// Fill out your copyright notice in the Description page of Project Settings.

#include "OKNavMeshGenerateComponent.h"
#include "AI/NavigationModifier.h"
#include "AI/NavigationSystemBase.h"
#include "Engine/CollisionProfile.h"
#include "AI/Navigation/NavAreaBase.h"
#include "AI/Navigation/NavigationRelevantData.h"
#include "NavAreas/NavArea_Default.h"

UOKNavMeshGenerateComponent::UOKNavMeshGenerateComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, NavAreaClass(UNavArea_Default::StaticClass())
{
	PrimaryComponentTick.bCanEverTick = false;
	SetGenerateOverlapEvents(false);
	CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	BodyInstance.SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
}

void UOKNavMeshGenerateComponent::BeginPlay()
{
	Super::BeginPlay();
}

bool UOKNavMeshGenerateComponent::IsNavigationRelevant() const
{
	return CanEverAffectNavigation();
}

void UOKNavMeshGenerateComponent::GetNavigationData(FNavigationRelevantData& Data) const
{
	if (NavAreaClass)
	{
		// No need to create modifiers if the AreaClass we want to set is the default one unless we want to replace a NavArea to default.
		if (NavAreaClass != FNavigationSystem::GetDefaultWalkableArea() || AreaClassToReplace)
		{
			Data.Modifiers.CreateAreaModifiers(this, NavAreaClass, AreaClassToReplace);
		}
	}
}

void UOKNavMeshGenerateComponent::PostInitProperties()
{
	Super::PostInitProperties();

#if WITH_EDITOR
	if (GIsEditor && !HasAnyFlags(RF_ClassDefaultObject))
	{
		OnNavAreaRegisteredDelegateHandle = UNavigationSystemBase::OnNavAreaRegisteredDelegate().AddUObject(this, &UOKNavMeshGenerateComponent::OnNavAreaRegistered);
		OnNavAreaUnregisteredDelegateHandle = UNavigationSystemBase::OnNavAreaUnregisteredDelegate().AddUObject(this, &UOKNavMeshGenerateComponent::OnNavAreaUnregistered);
	}
#endif // WITH_EDITOR
}

void UOKNavMeshGenerateComponent::BeginDestroy()
{
	Super::BeginDestroy();

#if WITH_EDITOR
	if (GIsEditor && !HasAnyFlags(RF_ClassDefaultObject))
	{
		UNavigationSystemBase::OnNavAreaRegisteredDelegate().Remove(OnNavAreaRegisteredDelegateHandle);
		UNavigationSystemBase::OnNavAreaUnregisteredDelegate().Remove(OnNavAreaUnregisteredDelegateHandle);
	}
#endif // WITH_EDITOR
}

#if WITH_EDITOR
// This function is only called if GIsEditor == true
void UOKNavMeshGenerateComponent::OnNavAreaRegistered(const UWorld& World, const UClass* InNavAreaClass)
{
	(void)World;
	(void)InNavAreaClass;
	FNavigationSystem::UpdateActorData(*this->GetOwner());
}

// This function is only called if GIsEditor == true
void UOKNavMeshGenerateComponent::OnNavAreaUnregistered(const UWorld& World, const UClass* InNavAreaClass)
{
	(void)World;
	(void)InNavAreaClass;
	FNavigationSystem::UpdateActorData(*this->GetOwner());
}
#endif // WITH_EDITOR
