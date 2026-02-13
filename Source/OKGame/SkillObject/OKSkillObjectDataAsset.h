#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SkillObject/Policy/OKSkillPolicyBase.h"
#include "OKSkillObjectDataAsset.generated.h"

class AOKSkillObject;

UCLASS(BlueprintType)
class OKGAME_API UOKSkillObjectDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	TSubclassOf<AOKSkillObject> SkillObjectClass;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Skill|Policy")
	TObjectPtr<UOKSpawnPolicy> SpawnPolicy;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Skill|Policy")
	TObjectPtr<UOKMovementPolicy> MovementPolicy;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Skill|Policy")
	TObjectPtr<UOKDetectionPolicy> DetectionPolicy;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Skill|Policy")
	TObjectPtr<UOKFilterPolicy> FilterPolicy;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Skill|Policy")
	TObjectPtr<UOKEffectPolicy> EffectPolicy;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Skill|Policy")
	TObjectPtr<UOKTimingPolicy> TimingPolicy;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Skill|Policy")
	TObjectPtr<UOKExpirePolicy> ExpirePolicy;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|FollowUp")
	TSoftObjectPtr<UOKSkillObjectDataAsset> OnExpireSpawnDataAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	float MaxSafetyLifetime = 30.0f;
};

