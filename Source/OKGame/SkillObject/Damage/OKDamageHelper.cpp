#include "SkillObject/Damage/OKDamageHelper.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

UAbilitySystemComponent* UOKDamageHelper::GetASCFromActor(const AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}

	return Actor->FindComponentByClass<UAbilitySystemComponent>();
}

bool UOKDamageHelper::ApplyGameplayEffect(
	UAbilitySystemComponent* SourceASC,
	AActor* TargetActor,
	TSubclassOf<UGameplayEffect> GameplayEffectClass,
	const FGameplayTag SetByCallerTag,
	const float SetByCallerMagnitude,
	const float AbilityLevel)
{
	if (!IsValid(SourceASC) || !IsValid(TargetActor) || !*GameplayEffectClass)
	{
		return false;
	}

	UAbilitySystemComponent* TargetASC = GetASCFromActor(TargetActor);
	if (!IsValid(TargetASC))
	{
		return false;
	}

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddSourceObject(SourceASC);

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(GameplayEffectClass, AbilityLevel, EffectContext);
	if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid())
	{
		return false;
	}

	if (SetByCallerTag.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(SetByCallerTag, SetByCallerMagnitude);
	}

	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	return true;
}

