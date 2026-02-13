#include "SkillObject/Policy/Spawn/OKSpawnPolicies.h"

namespace
{
	FVector ResolveAimDirection(const FOKSkillSpawnContext& Context)
	{
		if (!Context.AimDirection.IsNearlyZero())
		{
			return Context.AimDirection.GetSafeNormal();
		}

		return Context.InstigatorTransform.GetRotation().GetForwardVector();
	}
}

void UOKSpawnPolicy_Forward::GenerateSpawnResults(const FOKSkillSpawnContext& Context, TArray<FOKSkillSpawnResult>& OutResults) const
{
	OutResults.Reset();

	const FVector AimDirection = ResolveAimDirection(Context);
	FOKSkillSpawnResult Result;
	Result.SpawnTransform = FTransform(AimDirection.Rotation(), Context.InstigatorTransform.GetLocation() + (AimDirection * ForwardDistance) + FVector(0.0f, 0.0f, HeightOffset));
	OutResults.Add(Result);
}

void UOKSpawnPolicy_AtTarget::GenerateSpawnResults(const FOKSkillSpawnContext& Context, TArray<FOKSkillSpawnResult>& OutResults) const
{
	OutResults.Reset();

	FVector SpawnLocation = Context.InstigatorTransform.GetLocation();
	if (Context.TargetActor.IsValid())
	{
		SpawnLocation = Context.TargetActor->GetActorLocation();
	}
	else if (Context.bHasTargetLocation)
	{
		SpawnLocation = Context.TargetLocation;
	}
	else if (!bFallbackToInstigatorLocation)
	{
		return;
	}

	const FVector AimDirection = ResolveAimDirection(Context);
	FOKSkillSpawnResult Result;
	Result.SpawnTransform = FTransform(AimDirection.Rotation(), SpawnLocation);
	OutResults.Add(Result);
}

void UOKSpawnPolicy_Spread::GenerateSpawnResults(const FOKSkillSpawnContext& Context, TArray<FOKSkillSpawnResult>& OutResults) const
{
	OutResults.Reset();

	const int32 SafeCount = FMath::Max(1, Count);
	const FVector AimDirection = ResolveAimDirection(Context);
	const FRotator AimRotation = AimDirection.Rotation();
	const float StepAngle = SafeCount > 1 ? SpreadAngleDegrees / static_cast<float>(SafeCount - 1) : 0.0f;
	const float StartAngle = -SpreadAngleDegrees * 0.5f;

	for (int32 Index = 0; Index < SafeCount; ++Index)
	{
		const float Angle = StartAngle + (StepAngle * Index);
		const FRotator SpawnRotation = AimRotation + FRotator(0.0f, Angle, 0.0f);
		const FVector SpawnDirection = SpawnRotation.Vector();

		FOKSkillSpawnResult Result;
		Result.SpawnTransform = FTransform(SpawnRotation, Context.InstigatorTransform.GetLocation() + (SpawnDirection * Distance));
		OutResults.Add(Result);
	}
}

void UOKSpawnPolicy_Drop::GenerateSpawnResults(const FOKSkillSpawnContext& Context, TArray<FOKSkillSpawnResult>& OutResults) const
{
	OutResults.Reset();

	const int32 SafeCount = FMath::Max(1, Count);
	const FVector Center = Context.bHasTargetLocation
		? Context.TargetLocation
		: (Context.TargetActor.IsValid() ? Context.TargetActor->GetActorLocation() : Context.InstigatorTransform.GetLocation());

	FRandomStream RandomStream(Context.RandomSeed == 0 ? 1337 : Context.RandomSeed);

	for (int32 Index = 0; Index < SafeCount; ++Index)
	{
		FVector Offset = FVector::ZeroVector;

		switch (Pattern)
		{
		case EOKDropPattern::Random:
		{
			const float Angle = RandomStream.FRandRange(0.0f, 2.0f * PI);
			const float Dist = RandomStream.FRandRange(0.0f, Radius);
			Offset = FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 0.0f);
			break;
		}
		case EOKDropPattern::Ring:
		{
			const float Angle = (2.0f * PI * static_cast<float>(Index)) / static_cast<float>(SafeCount);
			Offset = FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0f);
			break;
		}
		case EOKDropPattern::Grid:
		{
			const int32 Side = FMath::CeilToInt(FMath::Sqrt(static_cast<float>(SafeCount)));
			const int32 X = Index % Side;
			const int32 Y = Index / Side;
			const float Spacing = Side > 1 ? (Radius * 2.0f) / static_cast<float>(Side - 1) : 0.0f;
			Offset = FVector((-Radius) + (X * Spacing), (-Radius) + (Y * Spacing), 0.0f);
			break;
		}
		case EOKDropPattern::Cross:
		default:
		{
			const int32 ArmIndex = Index % 4;
			const float Distance = Radius * (0.25f + (0.75f * static_cast<float>(Index + 1) / static_cast<float>(SafeCount)));
			switch (ArmIndex)
			{
			case 0: Offset = FVector(Distance, 0.0f, 0.0f); break;
			case 1: Offset = FVector(-Distance, 0.0f, 0.0f); break;
			case 2: Offset = FVector(0.0f, Distance, 0.0f); break;
			default: Offset = FVector(0.0f, -Distance, 0.0f); break;
			}
			break;
		}
		}

		FOKSkillSpawnResult Result;
		Result.SpawnTransform = FTransform(FRotator(-90.0f, 0.0f, 0.0f), Center + Offset + FVector(0.0f, 0.0f, DropHeight));
		OutResults.Add(Result);
	}
}

