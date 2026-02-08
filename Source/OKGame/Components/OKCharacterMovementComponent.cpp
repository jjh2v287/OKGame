// Fill out your copyright notice in the Description page of Project Settings.

#include "OKCharacterMovementComponent.h"
#include "CollisionDebugDrawingPublic.h"
#include "NavigationSystem.h"
#include "GameFramework/Pawn.h"
#include "Components/SkeletalMeshComponent.h"
#include "NavMesh/RecastNavMesh.h"

UOKCharacterMovementComponent::UOKCharacterMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UOKCharacterMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	Mesh = PawnOwner->FindComponentByClass<USkeletalMeshComponent>();
}


void UOKCharacterMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(OKCharacterMovementComponent_Tick);
	FOKScopedMeshBoneUpdateOverride ScopedNoMeshBoneUpdate(Mesh, EKinematicBonesUpdateToPhysics::SkipAllBones);
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}