#include "SkillObject/Debug/OKSkillObjectDebugSpawner.h"

#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "SkillObject/OKSkillObject.h"
#include "SkillObject/OKSkillObjectDataAsset.h"
#include "SkillObject/OKSkillObjectSampleFactory.h"

AOKSkillObjectDebugSpawner::AOKSkillObjectDebugSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AOKSkillObjectDebugSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (!bFireOnBeginPlay)
	{
		return;
	}

	FireSkill();

	if (FireIntervalSeconds > KINDA_SMALL_NUMBER)
	{
		GetWorldTimerManager().SetTimer(FireTimerHandle, this, &ThisClass::FireSkill, FireIntervalSeconds, true);
	}
}

void AOKSkillObjectDebugSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (FireTimerHandle.IsValid())
	{
		GetWorldTimerManager().ClearTimer(FireTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void AOKSkillObjectDebugSpawner::FireSkill()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UOKSkillObjectDataAsset* EffectiveSkillDataAsset = ResolveSkillDataAsset();
	if (!EffectiveSkillDataAsset)
	{
		return;
	}

	FOKSkillSpawnContext SpawnContext;
	SpawnContext.InstigatorActor = this;
	SpawnContext.InstigatorTransform = GetActorTransform();
	SpawnContext.AimDirection = GetActorForwardVector();
	SpawnContext.InstigatorASC = ResolveSourceASC();
	SpawnContext.RandomSeed = bUseFixedRandomSeed ? FixedRandomSeed : FMath::Rand();

	if (IsValid(TargetActor))
	{
		SpawnContext.TargetActor = TargetActor;
		SpawnContext.TargetLocation = TargetActor->GetActorLocation();
		SpawnContext.bHasTargetLocation = true;
	}

	TArray<FOKSkillSpawnResult> SpawnResults;
	if (EffectiveSkillDataAsset->SpawnPolicy)
	{
		EffectiveSkillDataAsset->SpawnPolicy->GenerateSpawnResults(SpawnContext, SpawnResults);
	}

	if (SpawnResults.IsEmpty())
	{
		FOKSkillSpawnResult Fallback;
		Fallback.SpawnTransform = GetActorTransform();
		SpawnResults.Add(Fallback);
	}

	TSubclassOf<AOKSkillObject> SpawnClass = EffectiveSkillDataAsset->SkillObjectClass;
	if (!SpawnClass)
	{
		SpawnClass = AOKSkillObject::StaticClass();
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = GetInstigator();
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	for (const FOKSkillSpawnResult& SpawnResult : SpawnResults)
	{
		AOKSkillObject* SpawnedSkillObject = World->SpawnActor<AOKSkillObject>(SpawnClass, SpawnResult.SpawnTransform, SpawnParameters);
		if (SpawnedSkillObject)
		{
			SpawnedSkillObject->InitializeSkillObject(EffectiveSkillDataAsset, SpawnContext, SpawnResult);
		}
	}
}

UOKSkillObjectDataAsset* AOKSkillObjectDebugSpawner::ResolveSkillDataAsset()
{
	if (SkillObjectDataAsset)
	{
		return SkillObjectDataAsset;
	}

	if (!bUseRuntimeSampleSkillIfMissing)
	{
		return nullptr;
	}

	if (!RuntimeGeneratedSkillObjectDataAsset)
	{
		RuntimeGeneratedSkillObjectDataAsset = UOKSkillObjectSampleFactory::CreateTransientFireballSkill(this);
	}

	return RuntimeGeneratedSkillObjectDataAsset;
}

UAbilitySystemComponent* AOKSkillObjectDebugSpawner::ResolveSourceASC() const
{
	return FindComponentByClass<UAbilitySystemComponent>();
}
