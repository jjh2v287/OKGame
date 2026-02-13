#include "SkillObject/Policy/Detection/OKDetectionPolicies.h"

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "SkillObject/OKSkillObject.h"

namespace
{
	void ConfigureDetectionCollision(UShapeComponent* ShapeComponent)
	{
		if (!ShapeComponent)
		{
			return;
		}

		ShapeComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		ShapeComponent->SetCollisionObjectType(ECC_WorldDynamic);
		ShapeComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
		ShapeComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		ShapeComponent->SetGenerateOverlapEvents(true);
	}
}

UShapeComponent* UOKDetectionPolicy_Sphere::CreateDetectionShape(AOKSkillObject* SkillObject) const
{
	if (!SkillObject)
	{
		return nullptr;
	}

	USphereComponent* SphereComponent = SkillObject->GetDefaultSphereComponent();
	if (!SphereComponent)
	{
		return nullptr;
	}

	SphereComponent->SetSphereRadius(Radius);
	ConfigureDetectionCollision(SphereComponent);
	SphereComponent->SetHiddenInGame(true);
	return SphereComponent;
}

UShapeComponent* UOKDetectionPolicy_Box::CreateDetectionShape(AOKSkillObject* SkillObject) const
{
	if (!SkillObject)
	{
		return nullptr;
	}

	UBoxComponent* BoxComponent = NewObject<UBoxComponent>(SkillObject, TEXT("SkillDetection_Box"));
	if (!BoxComponent)
	{
		return nullptr;
	}

	BoxComponent->SetBoxExtent(Extent);
	BoxComponent->SetupAttachment(SkillObject->GetRootComponent());
	ConfigureDetectionCollision(BoxComponent);
	BoxComponent->RegisterComponent();
	BoxComponent->SetHiddenInGame(true);
	return BoxComponent;
}

UShapeComponent* UOKDetectionPolicy_Capsule::CreateDetectionShape(AOKSkillObject* SkillObject) const
{
	if (!SkillObject)
	{
		return nullptr;
	}

	UCapsuleComponent* CapsuleComponent = NewObject<UCapsuleComponent>(SkillObject, TEXT("SkillDetection_Capsule"));
	if (!CapsuleComponent)
	{
		return nullptr;
	}

	CapsuleComponent->SetCapsuleRadius(Radius);
	CapsuleComponent->SetCapsuleHalfHeight(HalfHeight);
	CapsuleComponent->SetupAttachment(SkillObject->GetRootComponent());
	ConfigureDetectionCollision(CapsuleComponent);
	CapsuleComponent->RegisterComponent();
	CapsuleComponent->SetHiddenInGame(true);
	return CapsuleComponent;
}

UShapeComponent* UOKDetectionPolicy_Ray::CreateDetectionShape(AOKSkillObject* SkillObject) const
{
	if (!SkillObject)
	{
		return nullptr;
	}

	USphereComponent* SphereComponent = SkillObject->GetDefaultSphereComponent();
	if (!SphereComponent)
	{
		return nullptr;
	}

	SphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SphereComponent->SetGenerateOverlapEvents(false);
	return SphereComponent;
}

void UOKDetectionPolicy_Ray::CollectTargets(AOKSkillObject* SkillObject, TArray<AActor*>& OutTargets) const
{
	if (!SkillObject)
	{
		return;
	}

	UWorld* World = SkillObject->GetWorld();
	if (!World)
	{
		return;
	}

	const FVector Start = SkillObject->GetActorLocation();
	const FVector End = Start + (SkillObject->GetActorForwardVector() * Length);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(OKSkillRayDetection), false, SkillObject);
	if (AActor* Instigator = SkillObject->GetCurrentInstigator())
	{
		QueryParams.AddIgnoredActor(Instigator);
	}

	TArray<FHitResult> Hits;
	World->LineTraceMultiByChannel(Hits, Start, End, TraceChannel, QueryParams);

	for (const FHitResult& Hit : Hits)
	{
		if (AActor* HitActor = Hit.GetActor())
		{
			OutTargets.AddUnique(HitActor);
		}
	}
}

