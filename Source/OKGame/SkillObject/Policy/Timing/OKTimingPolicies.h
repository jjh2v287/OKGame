#pragma once

#include "CoreMinimal.h"
#include "SkillObject/Policy/OKSkillPolicyBase.h"
#include "OKTimingPolicies.generated.h"

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKTimingPolicy_OnOverlap : public UOKTimingPolicy
{
	GENERATED_BODY()

public:
	virtual bool OnTargetDetected(AActor* TargetActor, const FOKSkillHitContext& HitContext) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0"))
	int32 MaxHitPerTarget = 1;
};

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKTimingPolicy_Periodic : public UOKTimingPolicy
{
	GENERATED_BODY()

public:
	virtual bool RequiresTick() const override { return true; }
	virtual bool OnTargetDetected(AActor* TargetActor, const FOKSkillHitContext& HitContext) override;
	virtual void TickPolicy(float DeltaTime) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0.01"))
	float Interval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing")
	bool bFireOnFirstTick = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0"))
	int32 MaxHitPerTarget = 0;

private:
	float AccumulatedTime = 0.0f;
};

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKTimingPolicy_OnExpire : public UOKTimingPolicy
{
	GENERATED_BODY()

public:
	virtual bool ShouldTriggerOnExpire() const override { return true; }
	virtual bool OnTargetDetected(AActor* TargetActor, const FOKSkillHitContext& HitContext) override;
};

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKTimingPolicy_Chain : public UOKTimingPolicy
{
	GENERATED_BODY()

public:
	virtual bool OnTargetDetected(AActor* TargetActor, const FOKSkillHitContext& HitContext) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "1"))
	int32 MaxChainCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0.0"))
	float SearchRadius = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing")
	bool bExpireAfterChain = true;
};

