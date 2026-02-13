#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Engine/TimerHandle.h"
#include "GameFramework/Actor.h"
#include "OKSkillObjectDebugSpawner.generated.h"

class UAbilitySystemComponent;
class UOKSkillObjectDataAsset;

UCLASS(BlueprintType)
class OKGAME_API AOKSkillObjectDebugSpawner : public AActor
{
	GENERATED_BODY()

public:
	AOKSkillObjectDebugSpawner();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "SkillObject|Debug")
	void FireSkill();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillObject|Debug")
	bool bFireOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillObject|Debug", meta = (ClampMin = "0.0"))
	float FireIntervalSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillObject|Debug")
	TObjectPtr<UOKSkillObjectDataAsset> SkillObjectDataAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillObject|Debug")
	bool bUseRuntimeSampleSkillIfMissing = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillObject|Debug")
	TObjectPtr<AActor> TargetActor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillObject|Debug")
	bool bUseFixedRandomSeed = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillObject|Debug")
	int32 FixedRandomSeed = 1337;

private:
	UOKSkillObjectDataAsset* ResolveSkillDataAsset();
	UAbilitySystemComponent* ResolveSourceASC() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<UOKSkillObjectDataAsset> RuntimeGeneratedSkillObjectDataAsset;

	FTimerHandle FireTimerHandle;
};
