// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Engine/TimerHandle.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OKCombatTokenManager.generated.h"

struct FAllOutAttackConditionQuery;
class UUKAllOutAttackSystem;
struct FGameplayEventData;

UENUM(BlueprintType)
enum class EUKCombatTokenState : uint8
{
	Available,
	InUse,
	OnCooldown
};

USTRUCT(Blueprintable)
struct FUKCombatToken
{
	GENERATED_BODY()
public:
	UPROPERTY(Transient)
	EUKCombatTokenState State = EUKCombatTokenState::Available;

	// 토큰 사용 후 해제를 안했을때 예외 처리용
	UPROPERTY(Transient)
	FTimerHandle InUseTimerHandle;
	
	// 토큰의 쿨타임 타이머 핸들
	UPROPERTY(Transient)
	FTimerHandle CooldownTimerHandle;

	// 토큰을 사용 중인 AI Pawn
	UPROPERTY(Transient)
	TWeakObjectPtr<APawn> TokenUser;
};

USTRUCT()
struct FUKCombatTokenRunTimeData
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<APawn> MonsterPawn;

	FDelegateHandle DeathEventHandle;

	bool operator==(const APawn* OtherPawn) const
	{
		return MonsterPawn == OtherPawn;
	}
};

UCLASS(Abstract, Blueprintable)
class OKGAME_API UUKCombatTokenManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	// USubsystem implementation Begin
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// USubsystem implementation End

	void RegisterAggroMonster(APawn* Register);
	void UnRegisterAggroMonster(APawn* Register);

	// 토큰 풀 크기 업데이트.
	void UpdateTokenPoolByMonsterCount();

	UFUNCTION(BlueprintPure, Category = "CombatToken")
	bool RequestThenRelease(APawn* Requester);
	
	UFUNCTION(BlueprintCallable, BlueprintPure=false)
	void ReleaseTokenByUser(const APawn* Requester);

	UFUNCTION(BlueprintPure, Category = "CombatToken")
	bool IsTokenAvailable();
	
	// 토큰을 요청하면 해당 토큰의 인덱스를 반환. 실패 시 INDEX_NONE.
	UFUNCTION(BlueprintPure, Category = "CombatToken")
	int32 RequestToken(APawn* Requester);

	// 사용이 끝난 토큰의 인덱스를 받아 쿨타임을 시작.
	UFUNCTION(BlueprintCallable, BlueprintPure=false)
	void ReleaseToken(const int32 TokenIndex);

	void OnTokenCooldownFinished(const int32 TokenIndex);

	void OnInUseTimerFinished(APawn* Register);

protected:
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TokenControlRatio {1.0f};

	UPROPERTY(EditAnywhere)
	float TokenCoolDownDuration {10.0f};

	// 현재 전투에서 사용 가능한 최대 토큰 수 (계산 결과 저장)
	int32 MaxAvailableTokens = 0;

private:
	UPROPERTY(Transient)
	TArray<FUKCombatToken> TokenPool;

	UPROPERTY(Transient)
	TArray<FUKCombatTokenRunTimeData> AggroMonsters;

	void OnRegisterDeath(const FGameplayEventData* GameplayEventData);
};
