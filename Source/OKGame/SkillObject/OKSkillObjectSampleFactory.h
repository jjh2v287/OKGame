#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "OKSkillObjectSampleFactory.generated.h"

class UOKSkillObjectDataAsset;

UCLASS()
class OKGAME_API UOKSkillObjectSampleFactory : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SkillObject|Sample")
	static UOKSkillObjectDataAsset* CreateTransientFireballSkill(UObject* Outer);
};
