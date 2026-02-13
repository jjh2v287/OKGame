#pragma once

#include "CoreMinimal.h"
#include "SkillObject/Policy/OKSkillPolicyBase.h"
#include "OKDetectionPolicies.generated.h"

class UBoxComponent;
class UCapsuleComponent;
class USphereComponent;

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKDetectionPolicy_Sphere : public UOKDetectionPolicy
{
	GENERATED_BODY()

public:
	virtual UShapeComponent* CreateDetectionShape(AOKSkillObject* SkillObject) const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Detection", meta = (ClampMin = "0.0"))
	float Radius = 30.0f;
};

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKDetectionPolicy_Box : public UOKDetectionPolicy
{
	GENERATED_BODY()

public:
	virtual UShapeComponent* CreateDetectionShape(AOKSkillObject* SkillObject) const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Detection")
	FVector Extent = FVector(50.0f, 50.0f, 50.0f);
};

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKDetectionPolicy_Capsule : public UOKDetectionPolicy
{
	GENERATED_BODY()

public:
	virtual UShapeComponent* CreateDetectionShape(AOKSkillObject* SkillObject) const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Detection", meta = (ClampMin = "0.0"))
	float Radius = 34.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Detection", meta = (ClampMin = "0.0"))
	float HalfHeight = 88.0f;
};

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKDetectionPolicy_Ray : public UOKDetectionPolicy
{
	GENERATED_BODY()

public:
	virtual UShapeComponent* CreateDetectionShape(AOKSkillObject* SkillObject) const override;
	virtual bool UsesOverlapEvents() const override { return false; }
	virtual void CollectTargets(AOKSkillObject* SkillObject, TArray<AActor*>& OutTargets) const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Detection", meta = (ClampMin = "0.0"))
	float Length = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Detection")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
};

