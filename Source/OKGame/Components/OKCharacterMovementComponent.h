// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "OKCharacterMovementComponent.generated.h"

struct FOKScopedMeshBoneUpdateOverride
{
	FOKScopedMeshBoneUpdateOverride(USkeletalMeshComponent* Mesh, EKinematicBonesUpdateToPhysics::Type OverrideSetting)
		: MeshRef(Mesh)
	{
		if (MeshRef)
		{
			// 현재 상태 저장
			SavedUpdateSetting = MeshRef->KinematicBonesUpdateType;
			// 오버라이드 설정 적용
			MeshRef->KinematicBonesUpdateType = OverrideSetting;
		}
	}

	~FOKScopedMeshBoneUpdateOverride()
	{
		if (MeshRef)
		{
			// 원래 설정 복원
			MeshRef->KinematicBonesUpdateType = SavedUpdateSetting;
		}
	}

private:
	USkeletalMeshComponent* MeshRef = nullptr;
	EKinematicBonesUpdateToPhysics::Type SavedUpdateSetting;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class OKGAME_API UOKCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	explicit UOKCharacterMovementComponent(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Move|Simple")
	USkeletalMeshComponent* Mesh;
};
