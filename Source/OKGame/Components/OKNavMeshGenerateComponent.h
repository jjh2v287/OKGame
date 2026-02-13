// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "OKNavMeshGenerateComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class OKGAME_API UOKNavMeshGenerateComponent : public UBoxComponent
{
	GENERATED_BODY()

public:
	UOKNavMeshGenerateComponent(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual bool IsNavigationRelevant() const override;

	virtual void GetNavigationData(FNavigationRelevantData& Data) const override;

	UPROPERTY(EditAnywhere, Category = "UK Navigation")
	TSubclassOf<class UNavArea> NavAreaClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= "UK Navigation")
	TSubclassOf<class UNavArea> AreaClassToReplace;

protected:
	virtual void PostInitProperties() override;
	virtual void BeginDestroy() override;

#if WITH_EDITOR
	FDelegateHandle OnNavAreaRegisteredDelegateHandle;
	FDelegateHandle OnNavAreaUnregisteredDelegateHandle;

	void OnNavAreaRegistered(const UWorld& World, const UClass* InNavAreaClass);
	void OnNavAreaUnregistered(const UWorld& World, const UClass* InNavAreaClass);
#endif
};
