#pragma once

#include "CoreMinimal.h"
#include "SkillObject/OKSkillObjectTypes.h"
#include "OKSkillPolicyBase.generated.h"

class AOKSkillObject;
class UShapeComponent;
class AActor;

UCLASS(Abstract, EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKSkillPolicyBase : public UObject
{
	GENERATED_BODY()

public:
	virtual void InitializePolicy(AOKSkillObject* InOwner, const FOKSkillSpawnContext& InSpawnContext, const FOKSkillSpawnResult& InSpawnResult);
	virtual void CleanupPolicy();
	virtual bool RequiresTick() const { return false; }
	virtual void TickPolicy(float DeltaTime);

	AOKSkillObject* GetOwnerSkillObject() const { return OwnerSkillObject.Get(); }
	const FOKSkillSpawnContext& GetSpawnContext() const { return SpawnContext; }
	const FOKSkillSpawnResult& GetSpawnResult() const { return SpawnResult; }

protected:
	UPROPERTY(Transient)
	TWeakObjectPtr<AOKSkillObject> OwnerSkillObject = nullptr;

	FOKSkillSpawnContext SpawnContext;
	FOKSkillSpawnResult SpawnResult;
};

UCLASS(Abstract, EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKSpawnPolicy : public UOKSkillPolicyBase
{
	GENERATED_BODY()

public:
	virtual void GenerateSpawnResults(const FOKSkillSpawnContext& Context, TArray<FOKSkillSpawnResult>& OutResults) const PURE_VIRTUAL(UOKSpawnPolicy::GenerateSpawnResults, );
};

UCLASS(Abstract, EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKMovementPolicy : public UOKSkillPolicyBase
{
	GENERATED_BODY()

public:
	virtual void NotifyWallHit(const FHitResult& HitResult);
};

UCLASS(Abstract, EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKDetectionPolicy : public UOKSkillPolicyBase
{
	GENERATED_BODY()

public:
	virtual UShapeComponent* CreateDetectionShape(AOKSkillObject* SkillObject) const PURE_VIRTUAL(UOKDetectionPolicy::CreateDetectionShape, return nullptr; );
	virtual bool UsesOverlapEvents() const { return true; }
	virtual void CollectTargets(AOKSkillObject* SkillObject, TArray<AActor*>& OutTargets) const;
};

UCLASS(Abstract, EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKFilterPolicy : public UOKSkillPolicyBase
{
	GENERATED_BODY()

public:
	virtual bool PassesFilter(AActor* Candidate, const FOKSkillHitContext& HitContext) const;
};

UCLASS(Abstract, EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKEffectPolicy : public UOKSkillPolicyBase
{
	GENERATED_BODY()

public:
	virtual void ApplyEffect(AActor* TargetActor, const FOKSkillHitContext& HitContext);
};

UCLASS(Abstract, EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKTimingPolicy : public UOKSkillPolicyBase
{
	GENERATED_BODY()

public:
	virtual bool OnTargetDetected(AActor* TargetActor, const FOKSkillHitContext& HitContext);
	virtual bool ShouldTriggerOnExpire() const { return false; }
};

UCLASS(Abstract, EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKExpirePolicy : public UOKSkillPolicyBase
{
	GENERATED_BODY()

public:
	virtual void NotifyTargetHit(AActor* TargetActor, const FOKSkillHitContext& HitContext);
	virtual void NotifyWallHit(const FHitResult& HitResult);
	virtual void TriggerManualExpire();
	virtual bool ShouldExpire(EOKSkillExpireReason& OutReason) const;

protected:
	void MarkPendingExpire(EOKSkillExpireReason Reason);

private:
	UPROPERTY(Transient)
	bool bWantsExpire = false;

	UPROPERTY(Transient)
	EOKSkillExpireReason PendingReason = EOKSkillExpireReason::None;
};

