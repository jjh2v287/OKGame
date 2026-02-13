#include "SkillObject/OKSkillObjectSampleFactory.h"

#include "SkillObject/OKSkillObject.h"
#include "SkillObject/OKSkillObjectDataAsset.h"
#include "SkillObject/Policy/Detection/OKDetectionPolicies.h"
#include "SkillObject/Policy/Effect/OKEffectPolicies.h"
#include "SkillObject/Policy/Expire/OKExpirePolicies.h"
#include "SkillObject/Policy/Filter/OKFilterPolicies.h"
#include "SkillObject/Policy/Movement/OKMovementPolicies.h"
#include "SkillObject/Policy/Spawn/OKSpawnPolicies.h"
#include "SkillObject/Policy/Timing/OKTimingPolicies.h"
#include "UObject/Package.h"

UOKSkillObjectDataAsset* UOKSkillObjectSampleFactory::CreateTransientFireballSkill(UObject* Outer)
{
	UObject* EffectiveOuter = IsValid(Outer) ? Outer : static_cast<UObject*>(GetTransientPackage());

	UOKSkillObjectDataAsset* DataAsset = NewObject<UOKSkillObjectDataAsset>(EffectiveOuter, NAME_None, RF_Transient);
	if (!DataAsset)
	{
		return nullptr;
	}

	DataAsset->DisplayName = FText::FromString(TEXT("Runtime_Fireball"));
	DataAsset->SkillObjectClass = AOKSkillObject::StaticClass();
	DataAsset->MaxSafetyLifetime = 5.0f;

	DataAsset->SpawnPolicy = NewObject<UOKSpawnPolicy_Forward>(DataAsset, NAME_None, RF_Transient);
	DataAsset->MovementPolicy = NewObject<UOKMovementPolicy_Projectile>(DataAsset, NAME_None, RF_Transient);
	DataAsset->DetectionPolicy = NewObject<UOKDetectionPolicy_Sphere>(DataAsset, NAME_None, RF_Transient);
	DataAsset->FilterPolicy = NewObject<UOKFilterPolicy_Tag>(DataAsset, NAME_None, RF_Transient);
	DataAsset->EffectPolicy = NewObject<UOKEffectPolicy_Damage>(DataAsset, NAME_None, RF_Transient);
	DataAsset->TimingPolicy = NewObject<UOKTimingPolicy_OnOverlap>(DataAsset, NAME_None, RF_Transient);
	DataAsset->ExpirePolicy = NewObject<UOKExpirePolicy_OnHit>(DataAsset, NAME_None, RF_Transient);

	return DataAsset;
}
