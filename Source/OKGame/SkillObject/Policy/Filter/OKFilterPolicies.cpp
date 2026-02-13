#include "SkillObject/Policy/Filter/OKFilterPolicies.h"

#include "AbilitySystemComponent.h"
#include "SkillObject/OKSkillObject.h"

namespace
{
	FGameplayTag GetTag(const TCHAR* TagName)
	{
		return FGameplayTag::RequestGameplayTag(FName(TagName), false);
	}

	FGameplayTag ResolveTeamTag(const AActor* Actor)
	{
		if (!IsValid(Actor))
		{
			return FGameplayTag();
		}

		if (const UAbilitySystemComponent* TargetASC = Actor->FindComponentByClass<UAbilitySystemComponent>())
		{
			FGameplayTagContainer OwnedTags;
			TargetASC->GetOwnedGameplayTags(OwnedTags);
			const FGameplayTag PlayerTag = GetTag(TEXT("Team.Player"));
			const FGameplayTag MonsterTag = GetTag(TEXT("Team.Monster"));
			const FGameplayTag NeutralTag = GetTag(TEXT("Team.Neutral"));
			const FGameplayTag DestructibleTag = GetTag(TEXT("Team.Destructible"));

			if (PlayerTag.IsValid() && OwnedTags.HasTag(PlayerTag)) { return PlayerTag; }
			if (MonsterTag.IsValid() && OwnedTags.HasTag(MonsterTag)) { return MonsterTag; }
			if (NeutralTag.IsValid() && OwnedTags.HasTag(NeutralTag)) { return NeutralTag; }
			if (DestructibleTag.IsValid() && OwnedTags.HasTag(DestructibleTag)) { return DestructibleTag; }
		}

		for (const FName& ActorTag : Actor->Tags)
		{
			const FString TagString = ActorTag.ToString();
			if (TagString.Equals(TEXT("Team.Player")) || TagString.Equals(TEXT("Team.Monster")) || TagString.Equals(TEXT("Team.Neutral")) || TagString.Equals(TEXT("Team.Destructible")))
			{
				const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(ActorTag, false);
				if (Tag.IsValid())
				{
					return Tag;
				}
			}
		}

		return FGameplayTag();
	}

	bool IsFlagEnabled(const int32 Mask, const EOKFilterTarget Flag)
	{
		return (Mask & static_cast<int32>(Flag)) != 0;
	}

	bool ActorHasGameplayTag(const AActor* Actor, const FGameplayTag& Tag)
	{
		if (!IsValid(Actor) || !Tag.IsValid())
		{
			return false;
		}

		if (const UAbilitySystemComponent* ASC = Actor->FindComponentByClass<UAbilitySystemComponent>())
		{
			return ASC->HasMatchingGameplayTag(Tag);
		}

		return Actor->ActorHasTag(Tag.GetTagName());
	}
}

bool UOKFilterPolicy_TeamBased::PassesFilter(AActor* Candidate, const FOKSkillHitContext& HitContext) const
{
	if (!IsValid(Candidate))
	{
		return false;
	}

	const AOKSkillObject* SkillObject = HitContext.SkillObject.Get();
	if (!SkillObject)
	{
		return false;
	}

	AActor* InstigatorActor = SkillObject->GetCurrentInstigator();
	if (!IsValid(InstigatorActor))
	{
		return false;
	}

	if (Candidate == InstigatorActor)
	{
		if (bIgnoreInstigator)
		{
			return false;
		}
		return IsFlagEnabled(AllowedTargetsMask, EOKFilterTarget::Self);
	}

	const FGameplayTag CandidateTeam = ResolveTeamTag(Candidate);
	const FGameplayTag InstigatorTeam = ResolveTeamTag(InstigatorActor);
	const FGameplayTag DestructibleTag = GetTag(TEXT("Team.Destructible"));
	const FGameplayTag NeutralTag = GetTag(TEXT("Team.Neutral"));

	if (DestructibleTag.IsValid() && CandidateTeam == DestructibleTag)
	{
		return IsFlagEnabled(AllowedTargetsMask, EOKFilterTarget::Destructible);
	}

	if (NeutralTag.IsValid() && CandidateTeam == NeutralTag)
	{
		return IsFlagEnabled(AllowedTargetsMask, EOKFilterTarget::Neutral);
	}

	if (InstigatorTeam.IsValid() && CandidateTeam.IsValid())
	{
		if (InstigatorTeam == CandidateTeam)
		{
			return IsFlagEnabled(AllowedTargetsMask, EOKFilterTarget::Ally);
		}

		return IsFlagEnabled(AllowedTargetsMask, EOKFilterTarget::Enemy);
	}

	return IsFlagEnabled(AllowedTargetsMask, EOKFilterTarget::Neutral);
}

bool UOKFilterPolicy_Tag::PassesFilter(AActor* Candidate, const FOKSkillHitContext& HitContext) const
{
	if (!IsValid(Candidate))
	{
		return false;
	}

	if (!Super::PassesFilter(Candidate, HitContext))
	{
		return false;
	}

	if (!BlockedTags.IsEmpty())
	{
		for (const FGameplayTag& BlockedTag : BlockedTags)
		{
			if (ActorHasGameplayTag(Candidate, BlockedTag))
			{
				return false;
			}
		}
	}

	if (RequiredTags.IsEmpty())
	{
		return true;
	}

	if (bRequireAllRequiredTags)
	{
		for (const FGameplayTag& RequiredTag : RequiredTags)
		{
			if (!ActorHasGameplayTag(Candidate, RequiredTag))
			{
				return false;
			}
		}
		return true;
	}

	for (const FGameplayTag& RequiredTag : RequiredTags)
	{
		if (ActorHasGameplayTag(Candidate, RequiredTag))
		{
			return true;
		}
	}

	return false;
}

