#include "SkillObject/Policy/OKSkillPolicyBase.h"

#include "Components/ShapeComponent.h"
#include "SkillObject/OKSkillObject.h"

void UOKSkillPolicyBase::InitializePolicy(AOKSkillObject* InOwner, const FOKSkillSpawnContext& InSpawnContext, const FOKSkillSpawnResult& InSpawnResult)
{
	OwnerSkillObject = InOwner;
	SpawnContext = InSpawnContext;
	SpawnResult = InSpawnResult;
}

void UOKSkillPolicyBase::CleanupPolicy()
{
	OwnerSkillObject = nullptr;
}

void UOKSkillPolicyBase::TickPolicy(const float DeltaTime)
{
	(void)DeltaTime;
}

void UOKMovementPolicy::NotifyWallHit(const FHitResult& HitResult)
{
	(void)HitResult;
}

void UOKDetectionPolicy::CollectTargets(AOKSkillObject* SkillObject, TArray<AActor*>& OutTargets) const
{
	if (!SkillObject)
	{
		return;
	}

	UShapeComponent* ActiveShape = SkillObject->GetActiveDetectionComponent();
	if (!ActiveShape)
	{
		return;
	}

	ActiveShape->GetOverlappingActors(OutTargets);
}

bool UOKFilterPolicy::PassesFilter(AActor* Candidate, const FOKSkillHitContext& HitContext) const
{
	(void)HitContext;
	return IsValid(Candidate);
}

void UOKEffectPolicy::ApplyEffect(AActor* TargetActor, const FOKSkillHitContext& HitContext)
{
	(void)TargetActor;
	(void)HitContext;
}

bool UOKTimingPolicy::OnTargetDetected(AActor* TargetActor, const FOKSkillHitContext& HitContext)
{
	(void)TargetActor;
	(void)HitContext;
	return true;
}

void UOKExpirePolicy::NotifyTargetHit(AActor* TargetActor, const FOKSkillHitContext& HitContext)
{
	(void)TargetActor;
	(void)HitContext;
}

void UOKExpirePolicy::NotifyWallHit(const FHitResult& HitResult)
{
	(void)HitResult;
}

void UOKExpirePolicy::TriggerManualExpire()
{
	MarkPendingExpire(EOKSkillExpireReason::Manual);
}

bool UOKExpirePolicy::ShouldExpire(EOKSkillExpireReason& OutReason) const
{
	if (!bWantsExpire)
	{
		OutReason = EOKSkillExpireReason::None;
		return false;
	}

	OutReason = PendingReason;
	return true;
}

void UOKExpirePolicy::MarkPendingExpire(const EOKSkillExpireReason Reason)
{
	bWantsExpire = true;
	PendingReason = Reason;
}

