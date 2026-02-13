#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Engine/HitResult.h"
#include "GameplayEffectTypes.h"
#include "OKSkillObjectTypes.generated.h"

class AActor;
class UAbilitySystemComponent;
class AOKSkillObject;

UENUM(BlueprintType)
enum class EOKDropPattern : uint8
{
	Random,
	Ring,
	Grid,
	Cross
};

UENUM(BlueprintType)
enum class EOKOrbitCenterLostBehavior : uint8
{
	Expire,
	Freefall,
	FixCenter
};

UENUM(BlueprintType)
enum class EOKKnockbackOrigin : uint8
{
	SkillObject,
	Instigator,
	WorldUp,
	Custom
};

UENUM(BlueprintType, meta=(Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EOKFilterTarget : uint8
{
	None = 0,
	Enemy = 1 << 0,
	Ally = 1 << 1,
	Neutral = 1 << 2,
	Destructible = 1 << 3,
	Self = 1 << 4,
	All = (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4)
};
ENUM_CLASS_FLAGS(EOKFilterTarget)

UENUM(BlueprintType)
enum class EOKWaveMode : uint8
{
	Horizontal2D,
	Vertical2D,
	Spiral3D
};

UENUM(BlueprintType)
enum class EOKSkillExpireReason : uint8
{
	None,
	Lifetime,
	Hit,
	HitCount,
	WallHit,
	Manual,
	PolicyCondition,
	ChainCompleted
};

USTRUCT(BlueprintType)
struct FOKSkillSpawnContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill")
	TWeakObjectPtr<AActor> InstigatorActor = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill")
	FTransform InstigatorTransform = FTransform::Identity;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill")
	FVector AimDirection = FVector::ForwardVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill")
	TWeakObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill")
	FVector TargetLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill")
	bool bHasTargetLocation = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill")
	TWeakObjectPtr<UAbilitySystemComponent> InstigatorASC = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill")
	int32 RandomSeed = 0;

	FGameplayEffectSpecHandle EffectSpecHandle;
};

USTRUCT(BlueprintType)
struct FOKSkillSpawnResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill")
	FTransform SpawnTransform = FTransform::Identity;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill")
	FVector InitialVelocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill")
	bool bHasInitialVelocity = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill")
	TWeakObjectPtr<AActor> HomingTargetActor = nullptr;
};

USTRUCT(BlueprintType)
struct FOKSkillHitContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill")
	TWeakObjectPtr<AOKSkillObject> SkillObject = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill")
	FHitResult HitResult;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill")
	bool bFromPeriodicTick = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill")
	bool bFromChainJump = false;
};
