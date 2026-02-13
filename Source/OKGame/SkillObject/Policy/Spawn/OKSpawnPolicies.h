#pragma once

#include "CoreMinimal.h"
#include "SkillObject/Policy/OKSkillPolicyBase.h"
#include "OKSpawnPolicies.generated.h"

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKSpawnPolicy_Forward : public UOKSpawnPolicy
{
	GENERATED_BODY()

public:
	virtual void GenerateSpawnResults(const FOKSkillSpawnContext& Context, TArray<FOKSkillSpawnResult>& OutResults) const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	float ForwardDistance = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	float HeightOffset = 0.0f;
};

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKSpawnPolicy_AtTarget : public UOKSpawnPolicy
{
	GENERATED_BODY()

public:
	virtual void GenerateSpawnResults(const FOKSkillSpawnContext& Context, TArray<FOKSkillSpawnResult>& OutResults) const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	bool bFallbackToInstigatorLocation = true;
};

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKSpawnPolicy_Spread : public UOKSpawnPolicy
{
	GENERATED_BODY()

public:
	virtual void GenerateSpawnResults(const FOKSkillSpawnContext& Context, TArray<FOKSkillSpawnResult>& OutResults) const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (ClampMin = "1"))
	int32 Count = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (ClampMin = "0.0"))
	float SpreadAngleDegrees = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	float Distance = 100.0f;
};

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKSpawnPolicy_Drop : public UOKSpawnPolicy
{
	GENERATED_BODY()

public:
	virtual void GenerateSpawnResults(const FOKSkillSpawnContext& Context, TArray<FOKSkillSpawnResult>& OutResults) const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (ClampMin = "1"))
	int32 Count = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	float DropHeight = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (ClampMin = "0.0"))
	float Radius = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	EOKDropPattern Pattern = EOKDropPattern::Random;
};

