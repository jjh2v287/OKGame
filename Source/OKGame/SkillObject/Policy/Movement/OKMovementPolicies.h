#pragma once

#include "CoreMinimal.h"
#include "SkillObject/Policy/OKSkillPolicyBase.h"
#include "OKMovementPolicies.generated.h"

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKMovementPolicy_None : public UOKMovementPolicy
{
	GENERATED_BODY()

public:
	virtual void InitializePolicy(AOKSkillObject* InOwner, const FOKSkillSpawnContext& InSpawnContext, const FOKSkillSpawnResult& InSpawnResult) override;
};

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKMovementPolicy_Projectile : public UOKMovementPolicy
{
	GENERATED_BODY()

public:
	virtual void InitializePolicy(AOKSkillObject* InOwner, const FOKSkillSpawnContext& InSpawnContext, const FOKSkillSpawnResult& InSpawnResult) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float Speed = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float GravityScale = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bRotationFollowsVelocity = true;
};

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKMovementPolicy_Homing : public UOKMovementPolicy
{
	GENERATED_BODY()

public:
	virtual void InitializePolicy(AOKSkillObject* InOwner, const FOKSkillSpawnContext& InSpawnContext, const FOKSkillSpawnResult& InSpawnResult) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float Speed = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float HomingAcceleration = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float GravityScale = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bRequireTarget = false;
};

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKMovementPolicy_Orbit : public UOKMovementPolicy
{
	GENERATED_BODY()

public:
	virtual void InitializePolicy(AOKSkillObject* InOwner, const FOKSkillSpawnContext& InSpawnContext, const FOKSkillSpawnResult& InSpawnResult) override;
	virtual bool RequiresTick() const override { return true; }
	virtual void TickPolicy(float DeltaTime) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float Radius = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float DegreesPerSecond = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	EOKOrbitCenterLostBehavior CenterLostBehavior = EOKOrbitCenterLostBehavior::FixCenter;

private:
	float CurrentAngle = 0.0f;
	FVector CachedCenter = FVector::ZeroVector;
};

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKMovementPolicy_Follow : public UOKMovementPolicy
{
	GENERATED_BODY()

public:
	virtual bool RequiresTick() const override { return true; }
	virtual void TickPolicy(float DeltaTime) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	FVector Offset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float InterpSpeed = 8.0f;
};

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKMovementPolicy_Boomerang : public UOKMovementPolicy
{
	GENERATED_BODY()

public:
	virtual void InitializePolicy(AOKSkillObject* InOwner, const FOKSkillSpawnContext& InSpawnContext, const FOKSkillSpawnResult& InSpawnResult) override;
	virtual bool RequiresTick() const override { return true; }
	virtual void TickPolicy(float DeltaTime) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float ForwardSpeed = 1400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float MaxDistance = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float ReturnSpeed = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float ArrivalDistance = 80.0f;

private:
	bool bReturning = false;
	FVector StartLocation = FVector::ZeroVector;
	FVector ForwardDirection = FVector::ForwardVector;
};

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class OKGAME_API UOKMovementPolicy_Wave : public UOKMovementPolicy
{
	GENERATED_BODY()

public:
	virtual void InitializePolicy(AOKSkillObject* InOwner, const FOKSkillSpawnContext& InSpawnContext, const FOKSkillSpawnResult& InSpawnResult) override;
	virtual bool RequiresTick() const override { return true; }
	virtual void TickPolicy(float DeltaTime) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float Speed = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float Amplitude = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float Frequency = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	EOKWaveMode WaveMode = EOKWaveMode::Horizontal2D;

private:
	FVector StartLocation = FVector::ZeroVector;
	FVector ForwardDirection = FVector::ForwardVector;
	float ElapsedTime = 0.0f;
};

