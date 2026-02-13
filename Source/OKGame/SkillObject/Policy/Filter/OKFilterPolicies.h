#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SkillObject/Policy/OKSkillPolicyBase.h"
#include "OKFilterPolicies.generated.h"

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKFilterPolicy_TeamBased : public UOKFilterPolicy
{
	GENERATED_BODY()

public:
	virtual bool PassesFilter(AActor* Candidate, const FOKSkillHitContext& HitContext) const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Filter", meta = (Bitmask, BitmaskEnum = "/Script/OKGame.EOKFilterTarget"))
	int32 AllowedTargetsMask = static_cast<int32>(EOKFilterTarget::Enemy);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Filter")
	bool bIgnoreInstigator = true;
};

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKFilterPolicy_Tag : public UOKFilterPolicy
{
	GENERATED_BODY()

public:
	virtual bool PassesFilter(AActor* Candidate, const FOKSkillHitContext& HitContext) const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Filter")
	FGameplayTagContainer RequiredTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Filter")
	FGameplayTagContainer BlockedTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Filter")
	bool bRequireAllRequiredTags = true;
};

