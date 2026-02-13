#pragma once

#include "CoreMinimal.h"
#include "SkillObject/Policy/OKSkillPolicyBase.h"
#include "OKExpirePolicies.generated.h"

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKExpirePolicy_Lifetime : public UOKExpirePolicy
{
	GENERATED_BODY()

public:
	virtual void InitializePolicy(AOKSkillObject* InOwner, const FOKSkillSpawnContext& InSpawnContext, const FOKSkillSpawnResult& InSpawnResult) override;
	virtual bool RequiresTick() const override { return true; }
	virtual void TickPolicy(float DeltaTime) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Expire", meta = (ClampMin = "0.01"))
	float Duration = 3.0f;

private:
	float Elapsed = 0.0f;
};

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKExpirePolicy_OnHit : public UOKExpirePolicy
{
	GENERATED_BODY()

public:
	virtual void NotifyTargetHit(AActor* TargetActor, const FOKSkillHitContext& HitContext) override;
};

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKExpirePolicy_HitCount : public UOKExpirePolicy
{
	GENERATED_BODY()

public:
	virtual void InitializePolicy(AOKSkillObject* InOwner, const FOKSkillSpawnContext& InSpawnContext, const FOKSkillSpawnResult& InSpawnResult) override;
	virtual void NotifyTargetHit(AActor* TargetActor, const FOKSkillHitContext& HitContext) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Expire", meta = (ClampMin = "1"))
	int32 MaxHitCount = 3;

private:
	int32 CurrentHitCount = 0;
};

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKExpirePolicy_WallHit : public UOKExpirePolicy
{
	GENERATED_BODY()

public:
	virtual void NotifyWallHit(const FHitResult& HitResult) override;
};

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKExpirePolicy_Manual : public UOKExpirePolicy
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Expire")
	void RequestManualExpire();
};

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKExpirePolicy_Composite : public UOKExpirePolicy
{
	GENERATED_BODY()

public:
	virtual void InitializePolicy(AOKSkillObject* InOwner, const FOKSkillSpawnContext& InSpawnContext, const FOKSkillSpawnResult& InSpawnResult) override;
	virtual void CleanupPolicy() override;
	virtual bool RequiresTick() const override;
	virtual void TickPolicy(float DeltaTime) override;
	virtual void NotifyTargetHit(AActor* TargetActor, const FOKSkillHitContext& HitContext) override;
	virtual void NotifyWallHit(const FHitResult& HitResult) override;
	virtual bool ShouldExpire(EOKSkillExpireReason& OutReason) const override;

protected:
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Expire")
	TArray<TObjectPtr<UOKExpirePolicy>> SubPolicies;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Expire")
	bool bRequireAllPolicies = false;
};

