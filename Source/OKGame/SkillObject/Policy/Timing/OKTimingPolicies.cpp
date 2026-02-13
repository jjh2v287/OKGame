#include "SkillObject/Policy/Timing/OKTimingPolicies.h"

#include "Engine/EngineTypes.h"
#include "Kismet/KismetSystemLibrary.h"
#include "SkillObject/OKSkillObject.h"

bool UOKTimingPolicy_OnOverlap::OnTargetDetected(AActor* TargetActor, const FOKSkillHitContext& HitContext)
{
	if (!IsValid(TargetActor))
	{
		return false;
	}

	if (const AOKSkillObject* Owner = HitContext.SkillObject.Get())
	{
		if (MaxHitPerTarget > 0)
		{
			return Owner->CanTriggerForTarget(TargetActor, MaxHitPerTarget);
		}
	}

	return true;
}

bool UOKTimingPolicy_Periodic::OnTargetDetected(AActor* TargetActor, const FOKSkillHitContext& HitContext)
{
	if (!bFireOnFirstTick || !IsValid(TargetActor))
	{
		return false;
	}

	if (const AOKSkillObject* Owner = HitContext.SkillObject.Get())
	{
		if (MaxHitPerTarget > 0)
		{
			return Owner->CanTriggerForTarget(TargetActor, MaxHitPerTarget);
		}
	}

	return true;
}

void UOKTimingPolicy_Periodic::TickPolicy(const float DeltaTime)
{
	AOKSkillObject* Owner = GetOwnerSkillObject();
	if (!Owner)
	{
		return;
	}

	AccumulatedTime += DeltaTime;
	if (AccumulatedTime < FMath::Max(Interval, KINDA_SMALL_NUMBER))
	{
		return;
	}

	AccumulatedTime = 0.0f;
	Owner->ApplyPeriodicEffects(MaxHitPerTarget);
}

bool UOKTimingPolicy_OnExpire::OnTargetDetected(AActor* TargetActor, const FOKSkillHitContext& HitContext)
{
	(void)TargetActor;
	(void)HitContext;
	return false;
}

bool UOKTimingPolicy_Chain::OnTargetDetected(AActor* TargetActor, const FOKSkillHitContext& HitContext)
{
	if (!IsValid(TargetActor))
	{
		return false;
	}

	AOKSkillObject* Owner = HitContext.SkillObject.Get();
	if (!Owner)
	{
		return true;
	}

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(Owner);
	if (AActor* InstigatorActor = Owner->GetCurrentInstigator())
	{
		IgnoreActors.Add(InstigatorActor);
	}

	TArray<AActor*> OverlapActors;
	UKismetSystemLibrary::SphereOverlapActors(
		Owner,
		TargetActor->GetActorLocation(),
		SearchRadius,
		ObjectTypes,
		nullptr,
		IgnoreActors,
		OverlapActors);

	int32 AppliedChains = 1;
	for (AActor* Candidate : OverlapActors)
	{
		if (AppliedChains >= MaxChainCount)
		{
			break;
		}

		if (!IsValid(Candidate) || Candidate == TargetActor)
		{
			continue;
		}

		FOKSkillHitContext ChainContext = HitContext;
		ChainContext.bFromChainJump = true;

		if (!Owner->FilterTarget(Candidate, ChainContext))
		{
			continue;
		}

		if (!Owner->CanTriggerForTarget(Candidate, 1))
		{
			continue;
		}

		Owner->ApplyEffectToTarget(Candidate, nullptr, false, true);
		++AppliedChains;
	}

	if (bExpireAfterChain)
	{
		Owner->RequestExpire(EOKSkillExpireReason::ChainCompleted);
	}

	return true;
}

