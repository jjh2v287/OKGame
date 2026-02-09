// Fill out your copyright notice in the Description page of Project Settings.

#include "OKCombatTokenManager.h"
#include "AbilitySystemComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OKCombatTokenManager)

void UUKCombatTokenManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UUKCombatTokenManager::Deinitialize()
{
	Super::Deinitialize();
	for (const FUKCombatTokenRunTimeData& MonsterData : AggroMonsters)
	{
		if (MonsterData.MonsterPawn)
		{
			if (UAbilitySystemComponent* ASC = MonsterData.MonsterPawn->FindComponentByClass<UAbilitySystemComponent>())
			{
				ASC->GenericGameplayEventCallbacks.FindOrAdd(FGameplayTag::RequestGameplayTag(TEXT("Event.Monster.Death"))).Remove(MonsterData.DeathEventHandle);
			}
		}
	}
	AggroMonsters.Empty();
}

void UUKCombatTokenManager::RegisterAggroMonster(APawn* Register)
{
	if (AggroMonsters.FindByKey(Register))
	{
		return;
	}
	
	if (UAbilitySystemComponent* RegisterASC = Register->FindComponentByClass<UAbilitySystemComponent>())
	{
		FUKCombatTokenRunTimeData NewMonsterData;
		NewMonsterData.MonsterPawn = Register;
		NewMonsterData.DeathEventHandle = RegisterASC->GenericGameplayEventCallbacks.FindOrAdd(FGameplayTag::RequestGameplayTag(TEXT("Event.Monster.Death"))).AddUObject(this, &ThisClass::OnRegisterDeath);

		AggroMonsters.Add(NewMonsterData);
	}
	
	UpdateTokenPoolByMonsterCount();
}

void UUKCombatTokenManager::UnRegisterAggroMonster(APawn* Register)
{
	if (!Register)
	{
		return;
	}

	const int32 FoundIndex = AggroMonsters.IndexOfByKey(Register);
	if (FoundIndex != INDEX_NONE)
	{
		if (UAbilitySystemComponent* RegisterASC = Register->FindComponentByClass<UAbilitySystemComponent>())
		{
			RegisterASC->GenericGameplayEventCallbacks.FindOrAdd(FGameplayTag::RequestGameplayTag(TEXT("Event.Monster.Death"))).Remove(AggroMonsters[FoundIndex].DeathEventHandle);
		}

		AggroMonsters.RemoveAt(FoundIndex);
	}

	// 토큰 반납을 못하고 죽었을때 처리
	const int32 Index = TokenPool.IndexOfByPredicate([Register](const FUKCombatToken& Token)
	{
		return Token.TokenUser == Register;
	});
	if (Index != INDEX_NONE)
	{
		if (TokenPool[Index].State == EUKCombatTokenState::InUse)
		{
			ReleaseToken(Index);
		}
	}
	
	UpdateTokenPoolByMonsterCount();
}

void UUKCombatTokenManager::UpdateTokenPoolByMonsterCount()
{
	// 1. 새로운 최대 토큰 수 계산
	const int32 MonsterCount = AggroMonsters.Num(); 
	int32 NewMaxTokens = FMath::Max(FMath::FloorToInt(static_cast<float>(MonsterCount) * TokenControlRatio), 1);
	if (MonsterCount == 0)
	{
		NewMaxTokens = 0;
	}

	const int32 CurrentTokenCount = TokenPool.Num();
	if (NewMaxTokens == CurrentTokenCount)
	{
		// 토큰 수에 변화가 없으면 아무 작업도 하지 않음
		return;
	}

	if (NewMaxTokens > CurrentTokenCount)
	{
		// 2. 토큰 수가 늘어나는 경우
		const int32 TokensToAdd = NewMaxTokens - CurrentTokenCount;
		// 새로운 토큰을 기본 상태(Available)로 추가
		TokenPool.AddDefaulted(TokensToAdd);
		UE_LOG(LogTemp, Log, TEXT("Token Pool Increased. New Count: %d"), NewMaxTokens);
	}
	else
	{
		// 3. 토큰 수가 줄어드는 경우
		int32 TokensToRemove = CurrentTokenCount - NewMaxTokens;
		
		// 뒤에서부터 순회하며 'Available' 상태인 토큰을 우선적으로 제거
		for (int32 i = TokenPool.Num() - 1; i >= 0 && TokensToRemove > 0; --i)
		{
			if (TokenPool[i].State != EUKCombatTokenState::InUse)
			{
				if (TokenPool[i].InUseTimerHandle.IsValid())
				{
					GetWorld()->GetTimerManager().ClearTimer(TokenPool[i].InUseTimerHandle);
				}
				
				if (TokenPool[i].CooldownTimerHandle.IsValid())
				{
					GetWorld()->GetTimerManager().ClearTimer(TokenPool[i].CooldownTimerHandle);
				}
				TokenPool.RemoveAt(i);
				TokensToRemove--;  
			}
		}
		
		// 만약 'Available' 토큰이 부족하여 모두 제거하지 못했더라도,
		// 현재 'InUse' 또는 'OnCooldown'인 토큰을 강제로 제거하지 않고 유지하여 안정성을 높인다.
		UE_LOG(LogTemp, Log, TEXT("Token Pool Decreased. Target Count: %d, Current Count: %d"), NewMaxTokens, TokenPool.Num());
	}
}

	bool UUKCombatTokenManager::RequestThenRelease(APawn* Requester)
{
	if (!Requester)
	{
		UE_LOG(LogTemp, Warning, TEXT("Requester is null"));
		return false;
	}
	
	const int32 TokenIndex = RequestToken(Requester);
	if (TokenIndex != INDEX_NONE)
	{
		ReleaseToken(TokenIndex);
		return true;
	}

	UE_LOG(LogTemp, Warning, TEXT("Requester failed to receive token '%s'"), *Requester->GetName());
	return false;
}

void UUKCombatTokenManager::ReleaseTokenByUser(const APawn* Requester)
{
	if (!Requester)
	{
		UE_LOG(LogTemp, Warning, TEXT("Requester is null"));
		return;
	}

	for (int32 i = 0; i < TokenPool.Num(); ++i)
	{
		if (TokenPool[i].State == EUKCombatTokenState::InUse && TokenPool[i].TokenUser.Get() == Requester)
		{
			ReleaseToken(i);
			return; 
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("ReleaseTokenByUser: Controller '%s' tried to release a token but none was found in use by it."), *Requester->GetName());
}

bool UUKCombatTokenManager::IsTokenAvailable()
{
	bool bIsAvailable = false;
	
	for (int32 i = 0; i < TokenPool.Num(); ++i)
	{
		if (TokenPool[i].State == EUKCombatTokenState::Available)
		{
			bIsAvailable = true;
			break;
		}
	}
	
	return bIsAvailable;
}

int32 UUKCombatTokenManager::RequestToken(APawn* Requester)
{
	if (!Requester)
	{
		UE_LOG(LogTemp, Warning, TEXT("Requester is null"));
		return INDEX_NONE;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("World is invalid, cannot set cooldown timer"));
		return INDEX_NONE;
	}
	
	for (int32 i = 0; i < TokenPool.Num(); ++i)
	{
		if (TokenPool[i].State == EUKCombatTokenState::Available)
		{
			// 사용 중 상태 변경.
			TokenPool[i].State = EUKCombatTokenState::InUse;
			TokenPool[i].TokenUser = Requester;
			
			// 쿨타임 타이머 설정
			if (TokenPool[i].InUseTimerHandle.IsValid())
			{
				World->GetTimerManager().ClearTimer(TokenPool[i].InUseTimerHandle);
			}

			// FTimerDelegate TimerDelegate;
			// TimerDelegate.BindUObject(this, &UUKCombatTokenManager::OnInUseTimerFinished, Requester);
			// constexpr float Duration = 5.0f;//FMath::Max(TokenCoolDownDuration * 0.5f, 1.0f);
			// World->GetTimerManager().SetTimer(TokenPool[i].InUseTimerHandle, TimerDelegate, Duration, false);
			return i;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Requester failed to receive token '%s'"), *Requester->GetName());
	return INDEX_NONE;
}

void UUKCombatTokenManager::ReleaseToken(const int32 TokenIndex)
{
	if (!TokenPool.IsValidIndex(TokenIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid TokenIndex: %d"), TokenIndex);
		return;
	}
    
	if (TokenPool[TokenIndex].State == EUKCombatTokenState::Available)
	{
		UE_LOG(LogTemp, Warning, TEXT("Token %d is already available"), TokenIndex);
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("World is invalid, cannot set cooldown timer"));
		return;
	}
    
	if (TokenPool[TokenIndex].InUseTimerHandle.IsValid())
	{
		World->GetTimerManager().ClearTimer(TokenPool[TokenIndex].InUseTimerHandle);
	}
	
	if (TokenPool[TokenIndex].CooldownTimerHandle.IsValid())
	{
		World->GetTimerManager().ClearTimer(TokenPool[TokenIndex].CooldownTimerHandle);
	}

	// 쿨타임 상태 변경.
	TokenPool[TokenIndex].State = EUKCombatTokenState::OnCooldown;
	TokenPool[TokenIndex].TokenUser.Reset();

	// 쿨타임 타이머 설정
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindUObject(this, &UUKCombatTokenManager::OnTokenCooldownFinished, TokenIndex);
	World->GetTimerManager().SetTimer(TokenPool[TokenIndex].CooldownTimerHandle, TimerDelegate, TokenCoolDownDuration, false);
}

void UUKCombatTokenManager::OnTokenCooldownFinished(const int32 TokenIndex)
{
	if (TokenPool.IsValidIndex(TokenIndex))
	{
		// 사용 가능 상태 변경.
		TokenPool[TokenIndex].State = EUKCombatTokenState::Available;
		TokenPool[TokenIndex].CooldownTimerHandle.Invalidate();
	}
}

void UUKCombatTokenManager::OnInUseTimerFinished(APawn* Register)
{
	const int32 Index = TokenPool.IndexOfByPredicate([Register](const FUKCombatToken& Token)
	{
		return Token.TokenUser == Register;
	});
	
	if (Index != INDEX_NONE)
	{
		if (TokenPool[Index].State == EUKCombatTokenState::InUse)
		{
			ReleaseToken(Index);
		}
	}
}

void UUKCombatTokenManager::OnRegisterDeath([[maybe_unused]] const FGameplayEventData* GameplayEventData)
{
	if (GameplayEventData && GameplayEventData->Target)
	{
		if (APawn* DeadPawn = Cast<APawn>(const_cast<AActor*>(GameplayEventData->Target.Get())))
		{
			UnRegisterAggroMonster(DeadPawn);
		}
	}
}
