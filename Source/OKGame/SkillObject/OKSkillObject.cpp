#include "SkillObject/OKSkillObject.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/ShapeComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "SkillObject/OKSkillObjectDataAsset.h"
#include "SkillObject/Policy/OKSkillPolicyBase.h"
#include "SkillObject/Policy/Detection/OKDetectionPolicies.h"
#include "SkillObject/Policy/Expire/OKExpirePolicies.h"
#include "SkillObject/Policy/Timing/OKTimingPolicies.h"

AOKSkillObject::AOKSkillObject()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	DefaultDetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DefaultDetectionSphere"));
	DefaultDetectionSphere->SetupAttachment(Root);
	DefaultDetectionSphere->SetSphereRadius(30.0f);
	DefaultDetectionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DefaultDetectionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	DefaultDetectionSphere->SetGenerateOverlapEvents(false);
	DefaultDetectionSphere->SetHiddenInGame(true);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovementComponent->bAutoActivate = false;
	ProjectileMovementComponent->SetComponentTickEnabled(false);
	ProjectileMovementComponent->bAutoRegisterUpdatedComponent = false;
	ProjectileMovementComponent->SetUpdatedComponent(Root);
	ProjectileMovementComponent->OnProjectileStop.AddDynamic(this, &ThisClass::HandleProjectileStop);

	ActiveDetectionComponent = DefaultDetectionSphere;
}

void AOKSkillObject::BeginPlay()
{
	Super::BeginPlay();
}

void AOKSkillObject::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsExpiring)
	{
		return;
	}

	if (MovementPolicy && MovementPolicy->RequiresTick())
	{
		MovementPolicy->TickPolicy(DeltaTime);
	}

	if (DetectionPolicy && !DetectionPolicy->UsesOverlapEvents())
	{
		TArray<AActor*> DetectedTargets;
		DetectionPolicy->CollectTargets(this, DetectedTargets);
		for (AActor* TargetActor : DetectedTargets)
		{
			if (!IsValid(TargetActor) || TargetActor == this)
			{
				continue;
			}

			FOKSkillHitContext HitContext;
			HitContext.SkillObject = this;
			if (!FilterTarget(TargetActor, HitContext))
			{
				continue;
			}

			bool bShouldApply = true;
			if (TimingPolicy)
			{
				bShouldApply = TimingPolicy->OnTargetDetected(TargetActor, HitContext);
			}

			if (bShouldApply)
			{
				ApplyEffectToTarget(TargetActor, nullptr, false);
			}
		}
	}

	if (TimingPolicy && TimingPolicy->RequiresTick())
	{
		TimingPolicy->TickPolicy(DeltaTime);
	}

	if (ExpirePolicy)
	{
		if (ExpirePolicy->RequiresTick())
		{
			ExpirePolicy->TickPolicy(DeltaTime);
		}

		EOKSkillExpireReason Reason = EOKSkillExpireReason::None;
		if (ExpirePolicy->ShouldExpire(Reason))
		{
			RequestExpire(Reason);
		}
	}
}

void AOKSkillObject::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CleanupPolicyInstances();
	Super::EndPlay(EndPlayReason);
}

void AOKSkillObject::InitializeSkillObject(UOKSkillObjectDataAsset* InDataAsset, const FOKSkillSpawnContext& InSpawnContext, const FOKSkillSpawnResult& InSpawnResult)
{
	ResetRuntimeState();

	SkillDataAsset = InDataAsset;
	SpawnContext = InSpawnContext;
	SpawnResult = InSpawnResult;

	SetActorTransform(InSpawnResult.SpawnTransform);

	if (SkillDataAsset)
	{
		MovementPolicy = DuplicatePolicy(SkillDataAsset->MovementPolicy.Get());
		DetectionPolicy = DuplicatePolicy(SkillDataAsset->DetectionPolicy.Get());
		FilterPolicy = DuplicatePolicy(SkillDataAsset->FilterPolicy.Get());
		EffectPolicy = DuplicatePolicy(SkillDataAsset->EffectPolicy.Get());
		TimingPolicy = DuplicatePolicy(SkillDataAsset->TimingPolicy.Get());
		ExpirePolicy = DuplicatePolicy(SkillDataAsset->ExpirePolicy.Get());

		const float SafeLifeTime = FMath::Max(0.0f, SkillDataAsset->MaxSafetyLifetime);
		if (SafeLifeTime > 0.0f)
		{
			SetLifeSpan(SafeLifeTime);
		}
	}

	if (MovementPolicy)
	{
		MovementPolicy->InitializePolicy(this, SpawnContext, SpawnResult);
	}
	if (DetectionPolicy)
	{
		DetectionPolicy->InitializePolicy(this, SpawnContext, SpawnResult);
	}
	if (FilterPolicy)
	{
		FilterPolicy->InitializePolicy(this, SpawnContext, SpawnResult);
	}
	if (EffectPolicy)
	{
		EffectPolicy->InitializePolicy(this, SpawnContext, SpawnResult);
	}
	if (TimingPolicy)
	{
		TimingPolicy->InitializePolicy(this, SpawnContext, SpawnResult);
	}
	if (ExpirePolicy)
	{
		ExpirePolicy->InitializePolicy(this, SpawnContext, SpawnResult);
	}

	SetupDetectionShape();

	const bool bNeedsTick =
		(DetectionPolicy && !DetectionPolicy->UsesOverlapEvents()) ||
		(MovementPolicy && MovementPolicy->RequiresTick()) ||
		(TimingPolicy && TimingPolicy->RequiresTick()) ||
		(ExpirePolicy && ExpirePolicy->RequiresTick());

	SetActorTickEnabled(bNeedsTick);
}

void AOKSkillObject::RequestExpire(const EOKSkillExpireReason Reason)
{
	if (bIsExpiring)
	{
		return;
	}

	BeginExpireSequence(Reason);
}

void AOKSkillObject::NotifyWallHit(const FHitResult& HitResult)
{
	if (MovementPolicy)
	{
		MovementPolicy->NotifyWallHit(HitResult);
	}

	if (ExpirePolicy)
	{
		ExpirePolicy->NotifyWallHit(HitResult);
		EOKSkillExpireReason Reason = EOKSkillExpireReason::None;
		if (ExpirePolicy->ShouldExpire(Reason))
		{
			RequestExpire(Reason);
		}
	}
}

void AOKSkillObject::ApplyPeriodicEffects(const int32 MaxHitPerTargetFromTimingPolicy)
{
	TArray<AActor*> Targets;
	CollectTargetsByCurrentDetection(Targets);

	for (AActor* TargetActor : Targets)
	{
		if (!IsValid(TargetActor) || TargetActor == this)
		{
			continue;
		}

		FOKSkillHitContext HitContext;
		HitContext.SkillObject = this;
		HitContext.bFromPeriodicTick = true;

		if (!FilterTarget(TargetActor, HitContext))
		{
			continue;
		}

		if (MaxHitPerTargetFromTimingPolicy > 0 && !CanTriggerForTarget(TargetActor, MaxHitPerTargetFromTimingPolicy))
		{
			continue;
		}

		ApplyEffectToTarget(TargetActor, nullptr, true);
	}
}

void AOKSkillObject::ApplyEffectToTarget(AActor* TargetActor, const FHitResult* OptionalHitResult, const bool bFromPeriodicTick, const bool bFromChainJump)
{
	if (!IsValid(TargetActor) || bIsExpiring)
	{
		return;
	}

	FOKSkillHitContext HitContext;
	HitContext.SkillObject = this;
	HitContext.bFromPeriodicTick = bFromPeriodicTick;
	HitContext.bFromChainJump = bFromChainJump;
	if (OptionalHitResult)
	{
		HitContext.HitResult = *OptionalHitResult;
	}

	if (!FilterTarget(TargetActor, HitContext))
	{
		return;
	}

	if (EffectPolicy)
	{
		EffectPolicy->ApplyEffect(TargetActor, HitContext);
	}

	AlreadyAffectedTargets.Add(TargetActor);
	TargetHitCountMap.FindOrAdd(TargetActor) += 1;

	if (ExpirePolicy)
	{
		ExpirePolicy->NotifyTargetHit(TargetActor, HitContext);

		EOKSkillExpireReason Reason = EOKSkillExpireReason::None;
		if (ExpirePolicy->ShouldExpire(Reason))
		{
			RequestExpire(Reason);
		}
	}
}

bool AOKSkillObject::CanTriggerForTarget(AActor* TargetActor, const int32 MaxHitPerTarget) const
{
	if (MaxHitPerTarget <= 0)
	{
		return true;
	}

	if (!IsValid(TargetActor))
	{
		return false;
	}

	const int32* HitCount = TargetHitCountMap.Find(TargetActor);
	return !HitCount || *HitCount < MaxHitPerTarget;
}

bool AOKSkillObject::FilterTarget(AActor* TargetActor, const FOKSkillHitContext& HitContext) const
{
	if (!IsValid(TargetActor) || TargetActor == this)
	{
		return false;
	}

	if (FilterPolicy)
	{
		return FilterPolicy->PassesFilter(TargetActor, HitContext);
	}

	return true;
}

void AOKSkillObject::CollectTargetsByCurrentDetection(TArray<AActor*>& OutTargets) const
{
	OutTargets.Reset();

	if (DetectionPolicy)
	{
		DetectionPolicy->CollectTargets(const_cast<AOKSkillObject*>(this), OutTargets);
		OutTargets.Remove(const_cast<AOKSkillObject*>(this));
		return;
	}

	if (ActiveDetectionComponent)
	{
		ActiveDetectionComponent->GetOverlappingActors(OutTargets);
		OutTargets.Remove(const_cast<AOKSkillObject*>(this));
	}
}

void AOKSkillObject::ResetRuntimeState()
{
	bIsExpiring = false;
	TargetHitCountMap.Reset();
	AlreadyAffectedTargets.Reset();

	if (DefaultDetectionSphere)
	{
		DefaultDetectionSphere->OnComponentBeginOverlap.RemoveAll(this);
		DefaultDetectionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		DefaultDetectionSphere->SetGenerateOverlapEvents(false);
	}

	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->Deactivate();
		ProjectileMovementComponent->SetComponentTickEnabled(false);
		ProjectileMovementComponent->bIsHomingProjectile = false;
		ProjectileMovementComponent->HomingTargetComponent = nullptr;
	}

	if (ActiveDetectionComponent && ActiveDetectionComponent != DefaultDetectionSphere)
	{
		ActiveDetectionComponent->OnComponentBeginOverlap.RemoveAll(this);
		ActiveDetectionComponent->DestroyComponent();
	}

	ActiveDetectionComponent = DefaultDetectionSphere;
}

void AOKSkillObject::SetupDetectionShape()
{
	UShapeComponent* DetectionShape = DefaultDetectionSphere;
	if (DetectionPolicy)
	{
		if (UShapeComponent* CreatedShape = DetectionPolicy->CreateDetectionShape(this))
		{
			DetectionShape = CreatedShape;
		}
	}

	ActiveDetectionComponent = DetectionShape;
	BindDetectionEvents(DetectionShape);
}

void AOKSkillObject::BindDetectionEvents(UShapeComponent* DetectionComponent)
{
	if (!DetectionComponent)
	{
		return;
	}

	DetectionComponent->OnComponentBeginOverlap.RemoveAll(this);

	const bool bUseOverlapEvents = !DetectionPolicy || DetectionPolicy->UsesOverlapEvents();
	if (bUseOverlapEvents)
	{
		DetectionComponent->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandleDetectionOverlap);
	}
}

void AOKSkillObject::BeginExpireSequence(const EOKSkillExpireReason Reason)
{
	if (bIsExpiring)
	{
		return;
	}

	bIsExpiring = true;

	if (TimingPolicy && TimingPolicy->ShouldTriggerOnExpire())
	{
		TArray<AActor*> Targets;
		CollectTargetsByCurrentDetection(Targets);

		for (AActor* TargetActor : Targets)
		{
			if (!IsValid(TargetActor))
			{
				continue;
			}

			FOKSkillHitContext HitContext;
			HitContext.SkillObject = this;

			if (!FilterTarget(TargetActor, HitContext))
			{
				continue;
			}

			ApplyEffectToTarget(TargetActor, nullptr, false);
		}
	}

	SpawnFollowUpSkillObjects();

	if (ActiveDetectionComponent)
	{
		ActiveDetectionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ActiveDetectionComponent->SetGenerateOverlapEvents(false);
	}

	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);
	SetLifeSpan(0.02f);

	UE_LOG(LogTemp, Verbose, TEXT("SkillObject expired. Reason=%d, Actor=%s"), static_cast<int32>(Reason), *GetName());
}

void AOKSkillObject::SpawnFollowUpSkillObjects()
{
	if (!SkillDataAsset || SkillDataAsset->OnExpireSpawnDataAsset.IsNull())
	{
		return;
	}

	UOKSkillObjectDataAsset* FollowUpDataAsset = SkillDataAsset->OnExpireSpawnDataAsset.LoadSynchronous();
	if (!IsValid(FollowUpDataAsset))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FOKSkillSpawnContext FollowUpContext = SpawnContext;
	FollowUpContext.InstigatorTransform = GetActorTransform();
	FollowUpContext.AimDirection = GetActorForwardVector();
	FollowUpContext.TargetLocation = GetActorLocation();
	FollowUpContext.bHasTargetLocation = true;
	FollowUpContext.TargetActor = nullptr;

	TArray<FOKSkillSpawnResult> SpawnResults;
	if (FollowUpDataAsset->SpawnPolicy)
	{
		FollowUpDataAsset->SpawnPolicy->GenerateSpawnResults(FollowUpContext, SpawnResults);
	}

	if (SpawnResults.IsEmpty())
	{
		FOKSkillSpawnResult FallbackResult;
		FallbackResult.SpawnTransform = GetActorTransform();
		SpawnResults.Add(FallbackResult);
	}

	TSubclassOf<AOKSkillObject> SpawnClass = FollowUpDataAsset->SkillObjectClass;
	if (!SpawnClass)
	{
		SpawnClass = StaticClass();
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = GetOwner();
	SpawnParameters.Instigator = GetInstigator();
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	for (const FOKSkillSpawnResult& Result : SpawnResults)
	{
		AOKSkillObject* SpawnedSkillObject = World->SpawnActor<AOKSkillObject>(SpawnClass, Result.SpawnTransform, SpawnParameters);
		if (SpawnedSkillObject)
		{
			SpawnedSkillObject->InitializeSkillObject(FollowUpDataAsset, FollowUpContext, Result);
		}
	}
}

void AOKSkillObject::CleanupPolicyInstances()
{
	if (MovementPolicy)
	{
		MovementPolicy->CleanupPolicy();
		MovementPolicy = nullptr;
	}
	if (DetectionPolicy)
	{
		DetectionPolicy->CleanupPolicy();
		DetectionPolicy = nullptr;
	}
	if (FilterPolicy)
	{
		FilterPolicy->CleanupPolicy();
		FilterPolicy = nullptr;
	}
	if (EffectPolicy)
	{
		EffectPolicy->CleanupPolicy();
		EffectPolicy = nullptr;
	}
	if (TimingPolicy)
	{
		TimingPolicy->CleanupPolicy();
		TimingPolicy = nullptr;
	}
	if (ExpirePolicy)
	{
		ExpirePolicy->CleanupPolicy();
		ExpirePolicy = nullptr;
	}
}

void AOKSkillObject::HandleDetectionOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	(void)OverlappedComponent;
	(void)OtherComp;
	(void)OtherBodyIndex;
	(void)bFromSweep;

	if (bIsExpiring || !IsValid(OtherActor) || OtherActor == this)
	{
		return;
	}

	FOKSkillHitContext HitContext;
	HitContext.SkillObject = this;
	HitContext.HitResult = SweepResult;

	if (!FilterTarget(OtherActor, HitContext))
	{
		return;
	}

	bool bShouldApply = true;
	if (TimingPolicy)
	{
		bShouldApply = TimingPolicy->OnTargetDetected(OtherActor, HitContext);
	}

	if (bShouldApply)
	{
		ApplyEffectToTarget(OtherActor, &SweepResult, false);
	}
}

void AOKSkillObject::HandleProjectileStop(const FHitResult& ImpactResult)
{
	NotifyWallHit(ImpactResult);
}

template <typename TPolicyType>
TObjectPtr<TPolicyType> AOKSkillObject::DuplicatePolicy(TPolicyType* TemplatePolicy)
{
	if (!TemplatePolicy)
	{
		return nullptr;
	}

	return DuplicateObject<TPolicyType>(TemplatePolicy, this);
}

