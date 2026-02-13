#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameplayEffect.h"
#include "SkillObject/Policy/OKSkillPolicyBase.h"
#include "OKEffectPolicies.generated.h"

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKEffectPolicy_Damage : public UOKEffectPolicy
{
	GENERATED_BODY()

public:
	virtual void ApplyEffect(AActor* TargetActor, const FOKSkillHitContext& HitContext) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	TSubclassOf<UGameplayEffect> GameplayEffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	FGameplayTag SetByCallerTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	float Magnitude = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	float Coefficient = 1.0f;
};

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKEffectPolicy_Heal : public UOKEffectPolicy
{
	GENERATED_BODY()

public:
	virtual void ApplyEffect(AActor* TargetActor, const FOKSkillHitContext& HitContext) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	TSubclassOf<UGameplayEffect> GameplayEffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	FGameplayTag SetByCallerTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	float Magnitude = 10.0f;
};

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKEffectPolicy_Buff : public UOKEffectPolicy
{
	GENERATED_BODY()

public:
	virtual void ApplyEffect(AActor* TargetActor, const FOKSkillHitContext& HitContext) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	TSubclassOf<UGameplayEffect> GameplayEffectClass;
};

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKEffectPolicy_Knockback : public UOKEffectPolicy
{
	GENERATED_BODY()

public:
	virtual void ApplyEffect(AActor* TargetActor, const FOKSkillHitContext& HitContext) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	EOKKnockbackOrigin KnockbackOrigin = EOKKnockbackOrigin::SkillObject;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	FVector CustomDirection = FVector::ForwardVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	float KnockbackForce = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	float UpForce = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	TSubclassOf<UGameplayEffect> GameplayEffectClass;
};

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKEffectPolicy_Multi : public UOKEffectPolicy
{
	GENERATED_BODY()

public:
	virtual void InitializePolicy(AOKSkillObject* InOwner, const FOKSkillSpawnContext& InSpawnContext, const FOKSkillSpawnResult& InSpawnResult) override;
	virtual void CleanupPolicy() override;
	virtual void ApplyEffect(AActor* TargetActor, const FOKSkillHitContext& HitContext) override;

protected:
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Effect")
	TArray<TObjectPtr<UOKEffectPolicy>> SubEffects;
};

