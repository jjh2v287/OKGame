#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SkillObject/OKSkillObjectTypes.h"
#include "OKSkillObject.generated.h"

class UOKSkillObjectDataAsset;
class UOKMovementPolicy;
class UOKDetectionPolicy;
class UOKFilterPolicy;
class UOKEffectPolicy;
class UOKTimingPolicy;
class UOKExpirePolicy;
class UProjectileMovementComponent;
class USceneComponent;
class USphereComponent;
class UShapeComponent;

UCLASS(BlueprintType)
class OKGAME_API AOKSkillObject : public AActor
{
	GENERATED_BODY()

public:
	AOKSkillObject();

	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void InitializeSkillObject(UOKSkillObjectDataAsset* InDataAsset, const FOKSkillSpawnContext& InSpawnContext, const FOKSkillSpawnResult& InSpawnResult);

	void RequestExpire(EOKSkillExpireReason Reason);
	void NotifyWallHit(const FHitResult& HitResult);

	void ApplyPeriodicEffects(int32 MaxHitPerTargetFromTimingPolicy);
	void ApplyEffectToTarget(AActor* TargetActor, const FHitResult* OptionalHitResult, bool bFromPeriodicTick, bool bFromChainJump = false);
	bool CanTriggerForTarget(AActor* TargetActor, int32 MaxHitPerTarget) const;
	bool FilterTarget(AActor* TargetActor, const FOKSkillHitContext& HitContext) const;
	void CollectTargetsByCurrentDetection(TArray<AActor*>& OutTargets) const;

	AActor* GetCurrentInstigator() const { return SpawnContext.InstigatorActor.Get(); }
	const FOKSkillSpawnContext& GetCurrentSpawnContext() const { return SpawnContext; }
	const FOKSkillSpawnResult& GetCurrentSpawnResult() const { return SpawnResult; }
	UProjectileMovementComponent* GetProjectileMovementComponent() const { return ProjectileMovementComponent; }
	USphereComponent* GetDefaultSphereComponent() const { return DefaultDetectionSphere; }
	UShapeComponent* GetActiveDetectionComponent() const { return ActiveDetectionComponent; }
	UOKSkillObjectDataAsset* GetSkillDataAsset() const { return SkillDataAsset; }

protected:
	virtual void BeginPlay() override;

private:
	template <typename TPolicyType>
	TObjectPtr<TPolicyType> DuplicatePolicy(TPolicyType* TemplatePolicy);

	void ResetRuntimeState();
	void SetupDetectionShape();
	void BindDetectionEvents(UShapeComponent* DetectionComponent);
	void BeginExpireSequence(EOKSkillExpireReason Reason);
	void SpawnFollowUpSkillObjects();
	void CleanupPolicyInstances();

	UFUNCTION()
	void HandleDetectionOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleProjectileStop(const FHitResult& ImpactResult);

private:
	UPROPERTY(VisibleDefaultsOnly, Category = "Skill")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleDefaultsOnly, Category = "Skill")
	TObjectPtr<USphereComponent> DefaultDetectionSphere;

	UPROPERTY(VisibleDefaultsOnly, Category = "Skill")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

	UPROPERTY(Transient)
	TObjectPtr<UShapeComponent> ActiveDetectionComponent;

	UPROPERTY(Transient)
	TObjectPtr<UOKSkillObjectDataAsset> SkillDataAsset;

	UPROPERTY(Transient)
	TObjectPtr<UOKMovementPolicy> MovementPolicy;

	UPROPERTY(Transient)
	TObjectPtr<UOKDetectionPolicy> DetectionPolicy;

	UPROPERTY(Transient)
	TObjectPtr<UOKFilterPolicy> FilterPolicy;

	UPROPERTY(Transient)
	TObjectPtr<UOKEffectPolicy> EffectPolicy;

	UPROPERTY(Transient)
	TObjectPtr<UOKTimingPolicy> TimingPolicy;

	UPROPERTY(Transient)
	TObjectPtr<UOKExpirePolicy> ExpirePolicy;

	FOKSkillSpawnContext SpawnContext;
	FOKSkillSpawnResult SpawnResult;

	UPROPERTY(Transient)
	TMap<TWeakObjectPtr<AActor>, int32> TargetHitCountMap;

	UPROPERTY(Transient)
	TSet<TWeakObjectPtr<AActor>> AlreadyAffectedTargets;

	UPROPERTY(Transient)
	bool bIsExpiring = false;
};

