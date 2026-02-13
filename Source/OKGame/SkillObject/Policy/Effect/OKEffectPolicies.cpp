#include "SkillObject/Policy/Effect/OKEffectPolicies.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "SkillObject/Damage/OKDamageHelper.h"
#include "SkillObject/OKSkillObject.h"

namespace
{
	UAbilitySystemComponent* ResolveSourceASC(const FOKSkillSpawnContext& Context)
	{
		if (Context.InstigatorASC.IsValid())
		{
			return Context.InstigatorASC.Get();
		}

		if (AActor* InstigatorActor = Context.InstigatorActor.Get())
		{
			return UOKDamageHelper::GetASCFromActor(InstigatorActor);
		}

		return nullptr;
	}

	FVector ResolveKnockbackDirection(const EOKKnockbackOrigin OriginType, const FVector& CustomDirection, const AActor* SkillActor, const AActor* InstigatorActor, const AActor* TargetActor)
	{
		if (!IsValid(TargetActor))
		{
			return FVector::ForwardVector;
		}

		switch (OriginType)
		{
		case EOKKnockbackOrigin::SkillObject:
			if (IsValid(SkillActor))
			{
				return (TargetActor->GetActorLocation() - SkillActor->GetActorLocation()).GetSafeNormal();
			}
			break;
		case EOKKnockbackOrigin::Instigator:
			if (IsValid(InstigatorActor))
			{
				return (TargetActor->GetActorLocation() - InstigatorActor->GetActorLocation()).GetSafeNormal();
			}
			break;
		case EOKKnockbackOrigin::WorldUp:
			return FVector::UpVector;
		case EOKKnockbackOrigin::Custom:
		default:
			return CustomDirection.GetSafeNormal();
		}

		return FVector::ForwardVector;
	}
}

void UOKEffectPolicy_Damage::ApplyEffect(AActor* TargetActor, const FOKSkillHitContext& HitContext)
{
	if (!IsValid(TargetActor))
	{
		return;
	}

	const FOKSkillSpawnContext& Context = GetSpawnContext();
	UAbilitySystemComponent* SourceASC = ResolveSourceASC(Context);

	const float FinalMagnitude = Magnitude * Coefficient;
	const FGameplayTag EffectiveTag = SetByCallerTag.IsValid()
		? SetByCallerTag
		: FGameplayTag::RequestGameplayTag(FName(TEXT("Data.Damage.Base")), false);

	if (!UOKDamageHelper::ApplyGameplayEffect(SourceASC, TargetActor, GameplayEffectClass, EffectiveTag, FinalMagnitude, 1.0f))
	{
		AController* InstigatorController = nullptr;
		if (AActor* InstigatorActor = Context.InstigatorActor.Get())
		{
			InstigatorController = InstigatorActor->GetInstigatorController();
		}

		AActor* DamageCauser = HitContext.SkillObject.Get();
		UGameplayStatics::ApplyDamage(TargetActor, FinalMagnitude, InstigatorController, DamageCauser, nullptr);
	}
}

void UOKEffectPolicy_Heal::ApplyEffect(AActor* TargetActor, const FOKSkillHitContext& HitContext)
{
	(void)HitContext;

	if (!IsValid(TargetActor))
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = ResolveSourceASC(GetSpawnContext());
	const FGameplayTag EffectiveTag = SetByCallerTag.IsValid()
		? SetByCallerTag
		: FGameplayTag::RequestGameplayTag(FName(TEXT("Data.Heal.Base")), false);

	UOKDamageHelper::ApplyGameplayEffect(SourceASC, TargetActor, GameplayEffectClass, EffectiveTag, Magnitude, 1.0f);
}

void UOKEffectPolicy_Buff::ApplyEffect(AActor* TargetActor, const FOKSkillHitContext& HitContext)
{
	(void)HitContext;

	if (!IsValid(TargetActor))
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = ResolveSourceASC(GetSpawnContext());
	UOKDamageHelper::ApplyGameplayEffect(SourceASC, TargetActor, GameplayEffectClass, FGameplayTag(), 0.0f, 1.0f);
}

void UOKEffectPolicy_Knockback::ApplyEffect(AActor* TargetActor, const FOKSkillHitContext& HitContext)
{
	if (!IsValid(TargetActor))
	{
		return;
	}

	const FOKSkillSpawnContext& Context = GetSpawnContext();
	AActor* SkillActor = HitContext.SkillObject.Get();
	AActor* InstigatorActor = Context.InstigatorActor.Get();

	const FVector Direction = ResolveKnockbackDirection(KnockbackOrigin, CustomDirection, SkillActor, InstigatorActor, TargetActor);
	const FVector LaunchVelocity = (Direction * KnockbackForce) + (FVector::UpVector * UpForce);

	if (ACharacter* Character = Cast<ACharacter>(TargetActor))
	{
		Character->LaunchCharacter(LaunchVelocity, true, true);
	}

	if (GameplayEffectClass)
	{
		UAbilitySystemComponent* SourceASC = ResolveSourceASC(Context);
		UOKDamageHelper::ApplyGameplayEffect(SourceASC, TargetActor, GameplayEffectClass, FGameplayTag(), 0.0f, 1.0f);
	}
}

void UOKEffectPolicy_Multi::InitializePolicy(AOKSkillObject* InOwner, const FOKSkillSpawnContext& InSpawnContext, const FOKSkillSpawnResult& InSpawnResult)
{
	Super::InitializePolicy(InOwner, InSpawnContext, InSpawnResult);

	for (UOKEffectPolicy* SubEffect : SubEffects)
	{
		if (IsValid(SubEffect))
		{
			SubEffect->InitializePolicy(InOwner, InSpawnContext, InSpawnResult);
		}
	}
}

void UOKEffectPolicy_Multi::CleanupPolicy()
{
	for (UOKEffectPolicy* SubEffect : SubEffects)
	{
		if (IsValid(SubEffect))
		{
			SubEffect->CleanupPolicy();
		}
	}

	Super::CleanupPolicy();
}

void UOKEffectPolicy_Multi::ApplyEffect(AActor* TargetActor, const FOKSkillHitContext& HitContext)
{
	for (UOKEffectPolicy* SubEffect : SubEffects)
	{
		if (IsValid(SubEffect))
		{
			SubEffect->ApplyEffect(TargetActor, HitContext);
		}
	}
}

