#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "OKDamageHelper.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;

UCLASS()
class OKGAME_API UOKDamageHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Skill|Damage")
	static UAbilitySystemComponent* GetASCFromActor(const AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "Skill|Damage")
	static bool ApplyGameplayEffect(
		UAbilitySystemComponent* SourceASC,
		AActor* TargetActor,
		TSubclassOf<UGameplayEffect> GameplayEffectClass,
		FGameplayTag SetByCallerTag,
		float SetByCallerMagnitude,
		float AbilityLevel);
};
