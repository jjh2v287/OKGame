#include "SkillObject/Policy/Expire/OKExpirePolicies.h"

void UOKExpirePolicy_Lifetime::InitializePolicy(AOKSkillObject* InOwner, const FOKSkillSpawnContext& InSpawnContext, const FOKSkillSpawnResult& InSpawnResult)
{
	Super::InitializePolicy(InOwner, InSpawnContext, InSpawnResult);
	Elapsed = 0.0f;
}

void UOKExpirePolicy_Lifetime::TickPolicy(const float DeltaTime)
{
	Elapsed += DeltaTime;
	if (Elapsed >= Duration)
	{
		MarkPendingExpire(EOKSkillExpireReason::Lifetime);
	}
}

void UOKExpirePolicy_OnHit::NotifyTargetHit(AActor* TargetActor, const FOKSkillHitContext& HitContext)
{
	(void)TargetActor;
	(void)HitContext;
	MarkPendingExpire(EOKSkillExpireReason::Hit);
}

void UOKExpirePolicy_HitCount::InitializePolicy(AOKSkillObject* InOwner, const FOKSkillSpawnContext& InSpawnContext, const FOKSkillSpawnResult& InSpawnResult)
{
	Super::InitializePolicy(InOwner, InSpawnContext, InSpawnResult);
	CurrentHitCount = 0;
}

void UOKExpirePolicy_HitCount::NotifyTargetHit(AActor* TargetActor, const FOKSkillHitContext& HitContext)
{
	(void)TargetActor;
	(void)HitContext;

	++CurrentHitCount;
	if (CurrentHitCount >= MaxHitCount)
	{
		MarkPendingExpire(EOKSkillExpireReason::HitCount);
	}
}

void UOKExpirePolicy_WallHit::NotifyWallHit(const FHitResult& HitResult)
{
	(void)HitResult;
	MarkPendingExpire(EOKSkillExpireReason::WallHit);
}

void UOKExpirePolicy_Manual::RequestManualExpire()
{
	TriggerManualExpire();
}

void UOKExpirePolicy_Composite::InitializePolicy(AOKSkillObject* InOwner, const FOKSkillSpawnContext& InSpawnContext, const FOKSkillSpawnResult& InSpawnResult)
{
	Super::InitializePolicy(InOwner, InSpawnContext, InSpawnResult);

	for (UOKExpirePolicy* Policy : SubPolicies)
	{
		if (IsValid(Policy))
		{
			Policy->InitializePolicy(InOwner, InSpawnContext, InSpawnResult);
		}
	}
}

void UOKExpirePolicy_Composite::CleanupPolicy()
{
	for (UOKExpirePolicy* Policy : SubPolicies)
	{
		if (IsValid(Policy))
		{
			Policy->CleanupPolicy();
		}
	}

	Super::CleanupPolicy();
}

bool UOKExpirePolicy_Composite::RequiresTick() const
{
	for (const UOKExpirePolicy* Policy : SubPolicies)
	{
		if (IsValid(Policy) && Policy->RequiresTick())
		{
			return true;
		}
	}

	return false;
}

void UOKExpirePolicy_Composite::TickPolicy(const float DeltaTime)
{
	for (UOKExpirePolicy* Policy : SubPolicies)
	{
		if (IsValid(Policy) && Policy->RequiresTick())
		{
			Policy->TickPolicy(DeltaTime);
		}
	}
}

void UOKExpirePolicy_Composite::NotifyTargetHit(AActor* TargetActor, const FOKSkillHitContext& HitContext)
{
	for (UOKExpirePolicy* Policy : SubPolicies)
	{
		if (IsValid(Policy))
		{
			Policy->NotifyTargetHit(TargetActor, HitContext);
		}
	}
}

void UOKExpirePolicy_Composite::NotifyWallHit(const FHitResult& HitResult)
{
	for (UOKExpirePolicy* Policy : SubPolicies)
	{
		if (IsValid(Policy))
		{
			Policy->NotifyWallHit(HitResult);
		}
	}
}

bool UOKExpirePolicy_Composite::ShouldExpire(EOKSkillExpireReason& OutReason) const
{
	if (SubPolicies.IsEmpty())
	{
		OutReason = EOKSkillExpireReason::None;
		return false;
	}

	if (bRequireAllPolicies)
	{
		for (const UOKExpirePolicy* Policy : SubPolicies)
		{
			if (!IsValid(Policy))
			{
				continue;
			}

			EOKSkillExpireReason PolicyReason = EOKSkillExpireReason::None;
			if (!Policy->ShouldExpire(PolicyReason))
			{
				OutReason = EOKSkillExpireReason::None;
				return false;
			}
		}

		OutReason = EOKSkillExpireReason::PolicyCondition;
		return true;
	}

	for (const UOKExpirePolicy* Policy : SubPolicies)
	{
		if (!IsValid(Policy))
		{
			continue;
		}

		EOKSkillExpireReason PolicyReason = EOKSkillExpireReason::None;
		if (Policy->ShouldExpire(PolicyReason))
		{
			OutReason = PolicyReason;
			return true;
		}
	}

	OutReason = EOKSkillExpireReason::None;
	return false;
}
