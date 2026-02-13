#include "SkillObject/OKGameplayAbility_SkillObject.h"

#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Pawn.h"
#include "SkillObject/OKSkillObject.h"
#include "SkillObject/OKSkillObjectDataAsset.h"
#include "SkillObject/OKSkillObjectSampleFactory.h"
#include "SkillObject/Policy/OKSkillPolicyBase.h"

UOKGameplayAbility_SkillObject::UOKGameplayAbility_SkillObject()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UOKGameplayAbility_SkillObject::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!ActorInfo || !IsValid(ActorInfo->AvatarActor.Get()))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	UOKSkillObjectDataAsset* EffectiveSkillDataAsset = SkillObjectDataAsset;
	if (!EffectiveSkillDataAsset && bUseRuntimeSampleSkillIfMissing)
	{
		if (!RuntimeGeneratedSkillObjectDataAsset)
		{
			RuntimeGeneratedSkillObjectDataAsset = UOKSkillObjectSampleFactory::CreateTransientFireballSkill(this);
		}
		EffectiveSkillDataAsset = RuntimeGeneratedSkillObjectDataAsset;
	}

	if (!EffectiveSkillDataAsset)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	UWorld* World = AvatarActor->GetWorld();
	if (!World)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	FOKSkillSpawnContext SpawnContext;
	SpawnContext.InstigatorActor = AvatarActor;
	SpawnContext.InstigatorTransform = AvatarActor->GetActorTransform();
	SpawnContext.AimDirection = AvatarActor->GetActorForwardVector();
	SpawnContext.InstigatorASC = ActorInfo->AbilitySystemComponent.Get();
	SpawnContext.RandomSeed = bUseFixedRandomSeed ? FixedRandomSeed : FMath::Rand();

	if (TriggerEventData)
	{
		if (IsValid(TriggerEventData->Target.Get()))
		{
			AActor* TargetActor = const_cast<AActor*>(TriggerEventData->Target.Get());
			SpawnContext.TargetActor = TargetActor;
			SpawnContext.TargetLocation = TargetActor->GetActorLocation();
			SpawnContext.bHasTargetLocation = true;
		}
	}

	TArray<FOKSkillSpawnResult> SpawnResults;
	if (EffectiveSkillDataAsset->SpawnPolicy)
	{
		EffectiveSkillDataAsset->SpawnPolicy->GenerateSpawnResults(SpawnContext, SpawnResults);
	}

	if (SpawnResults.IsEmpty())
	{
		FOKSkillSpawnResult Fallback;
		Fallback.SpawnTransform = SpawnContext.InstigatorTransform;
		SpawnResults.Add(Fallback);
	}

	TSubclassOf<AOKSkillObject> SpawnClass = EffectiveSkillDataAsset->SkillObjectClass;
	if (!SpawnClass)
	{
		SpawnClass = AOKSkillObject::StaticClass();
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = AvatarActor;
	SpawnParameters.Instigator = Cast<APawn>(AvatarActor);
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	for (const FOKSkillSpawnResult& SpawnResult : SpawnResults)
	{
		AOKSkillObject* SkillObject = World->SpawnActor<AOKSkillObject>(SpawnClass, SpawnResult.SpawnTransform, SpawnParameters);
		if (SkillObject)
		{
			SkillObject->InitializeSkillObject(EffectiveSkillDataAsset, SpawnContext, SpawnResult);
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

