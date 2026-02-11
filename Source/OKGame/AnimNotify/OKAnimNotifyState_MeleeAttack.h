// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "OKAnimNotifyState_MeleeAttack.generated.h"

/**
 * 
 */
UCLASS()
class OKGAME_API UOKAnimNotifyState_MeleeAttack : public UAnimNotifyState
{
	GENERATED_BODY()
	
public:
	UOKAnimNotifyState_MeleeAttack();

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;

	void TraceTest(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference);
	// void LegacyTrace(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZK")
	FName BoneName = NAME_None;
protected:
	double PreMontagePosition;
	double CurrentMontagePosition;
};
