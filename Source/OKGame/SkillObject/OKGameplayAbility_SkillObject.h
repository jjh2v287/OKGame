#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "OKGameplayAbility_SkillObject.generated.h"

class AOKSkillObject;
class UOKSkillObjectDataAsset;

UCLASS(BlueprintType)
class OKGAME_API UOKGameplayAbility_SkillObject : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UOKGameplayAbility_SkillObject();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	TObjectPtr<UOKSkillObjectDataAsset> SkillObjectDataAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	bool bUseRuntimeSampleSkillIfMissing = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	int32 FixedRandomSeed = 1337;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	bool bUseFixedRandomSeed = false;

	UPROPERTY(Transient)
	TObjectPtr<UOKSkillObjectDataAsset> RuntimeGeneratedSkillObjectDataAsset;
};
