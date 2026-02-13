#include "SkillObject/Policy/Movement/OKMovementPolicies.h"

#include "GameFramework/ProjectileMovementComponent.h"
#include "SkillObject/OKSkillObject.h"

namespace
{
	FVector ResolveAimDirection(const FOKSkillSpawnContext& SpawnContext)
	{
		if (!SpawnContext.AimDirection.IsNearlyZero())
		{
			return SpawnContext.AimDirection.GetSafeNormal();
		}

		return SpawnContext.InstigatorTransform.GetRotation().GetForwardVector();
	}

	void MoveActorWithSweep(AOKSkillObject* Owner, const FVector& NewLocation)
	{
		if (!Owner)
		{
			return;
		}

		FHitResult SweepHit;
		Owner->SetActorLocation(NewLocation, true, &SweepHit);
		if (SweepHit.bBlockingHit)
		{
			Owner->NotifyWallHit(SweepHit);
		}
	}
}

void UOKMovementPolicy_None::InitializePolicy(AOKSkillObject* InOwner, const FOKSkillSpawnContext& InSpawnContext, const FOKSkillSpawnResult& InSpawnResult)
{
	Super::InitializePolicy(InOwner, InSpawnContext, InSpawnResult);

	if (AOKSkillObject* Owner = GetOwnerSkillObject())
	{
		if (UProjectileMovementComponent* ProjectileMovement = Owner->GetProjectileMovementComponent())
		{
			ProjectileMovement->Deactivate();
			ProjectileMovement->SetComponentTickEnabled(false);
		}
	}
}

void UOKMovementPolicy_Projectile::InitializePolicy(AOKSkillObject* InOwner, const FOKSkillSpawnContext& InSpawnContext, const FOKSkillSpawnResult& InSpawnResult)
{
	Super::InitializePolicy(InOwner, InSpawnContext, InSpawnResult);

	AOKSkillObject* Owner = GetOwnerSkillObject();
	if (!Owner)
	{
		return;
	}

	UProjectileMovementComponent* ProjectileMovement = Owner->GetProjectileMovementComponent();
	if (!ProjectileMovement)
	{
		return;
	}

	ProjectileMovement->Deactivate();
	ProjectileMovement->SetUpdatedComponent(Owner->GetRootComponent());
	ProjectileMovement->InitialSpeed = Speed;
	ProjectileMovement->MaxSpeed = Speed;
	ProjectileMovement->ProjectileGravityScale = GravityScale;
	ProjectileMovement->bRotationFollowsVelocity = bRotationFollowsVelocity;
	ProjectileMovement->bIsHomingProjectile = false;

	const FVector Direction = InSpawnResult.bHasInitialVelocity
		? InSpawnResult.InitialVelocity.GetSafeNormal()
		: ResolveAimDirection(InSpawnContext);
	ProjectileMovement->Velocity = Direction * Speed;
	ProjectileMovement->SetComponentTickEnabled(true);
	ProjectileMovement->Activate(true);
}

void UOKMovementPolicy_Homing::InitializePolicy(AOKSkillObject* InOwner, const FOKSkillSpawnContext& InSpawnContext, const FOKSkillSpawnResult& InSpawnResult)
{
	Super::InitializePolicy(InOwner, InSpawnContext, InSpawnResult);

	AOKSkillObject* Owner = GetOwnerSkillObject();
	if (!Owner)
	{
		return;
	}

	UProjectileMovementComponent* ProjectileMovement = Owner->GetProjectileMovementComponent();
	if (!ProjectileMovement)
	{
		return;
	}

	AActor* HomingTarget = InSpawnResult.HomingTargetActor.Get();
	if (!HomingTarget)
	{
		HomingTarget = InSpawnContext.TargetActor.Get();
	}

	if (bRequireTarget && !IsValid(HomingTarget))
	{
		Owner->RequestExpire(EOKSkillExpireReason::PolicyCondition);
		return;
	}

	ProjectileMovement->Deactivate();
	ProjectileMovement->SetUpdatedComponent(Owner->GetRootComponent());
	ProjectileMovement->InitialSpeed = Speed;
	ProjectileMovement->MaxSpeed = Speed;
	ProjectileMovement->ProjectileGravityScale = GravityScale;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bIsHomingProjectile = IsValid(HomingTarget);
	ProjectileMovement->HomingAccelerationMagnitude = HomingAcceleration;
	ProjectileMovement->HomingTargetComponent = IsValid(HomingTarget) ? HomingTarget->GetRootComponent() : nullptr;

	const FVector Direction = InSpawnResult.bHasInitialVelocity
		? InSpawnResult.InitialVelocity.GetSafeNormal()
		: ResolveAimDirection(InSpawnContext);
	ProjectileMovement->Velocity = Direction * Speed;
	ProjectileMovement->SetComponentTickEnabled(true);
	ProjectileMovement->Activate(true);
}

void UOKMovementPolicy_Orbit::InitializePolicy(AOKSkillObject* InOwner, const FOKSkillSpawnContext& InSpawnContext, const FOKSkillSpawnResult& InSpawnResult)
{
	Super::InitializePolicy(InOwner, InSpawnContext, InSpawnResult);

	CurrentAngle = 0.0f;
	CachedCenter = InSpawnContext.InstigatorTransform.GetLocation();

	if (AOKSkillObject* Owner = GetOwnerSkillObject())
	{
		if (UProjectileMovementComponent* ProjectileMovement = Owner->GetProjectileMovementComponent())
		{
			ProjectileMovement->Deactivate();
			ProjectileMovement->SetComponentTickEnabled(false);
		}
	}
}

void UOKMovementPolicy_Orbit::TickPolicy(const float DeltaTime)
{
	AOKSkillObject* Owner = GetOwnerSkillObject();
	if (!Owner)
	{
		return;
	}

	AActor* CenterActor = SpawnContext.InstigatorActor.Get();
	if (IsValid(CenterActor))
	{
		CachedCenter = CenterActor->GetActorLocation();
	}
	else
	{
		switch (CenterLostBehavior)
		{
		case EOKOrbitCenterLostBehavior::Expire:
			Owner->RequestExpire(EOKSkillExpireReason::PolicyCondition);
			return;
		case EOKOrbitCenterLostBehavior::Freefall:
			MoveActorWithSweep(Owner, Owner->GetActorLocation() + (Owner->GetActorForwardVector() * Radius * DeltaTime));
			return;
		case EOKOrbitCenterLostBehavior::FixCenter:
		default:
			break;
		}
	}

	CurrentAngle += DegreesPerSecond * DeltaTime;
	const float Radians = FMath::DegreesToRadians(CurrentAngle);
	const FVector Offset = FVector(FMath::Cos(Radians) * Radius, FMath::Sin(Radians) * Radius, 0.0f);
	const FVector NewLocation = CachedCenter + Offset;

	MoveActorWithSweep(Owner, NewLocation);
	Owner->SetActorRotation((CachedCenter - NewLocation).Rotation());
}

void UOKMovementPolicy_Follow::TickPolicy(const float DeltaTime)
{
	AOKSkillObject* Owner = GetOwnerSkillObject();
	if (!Owner)
	{
		return;
	}

	AActor* FollowTarget = SpawnContext.InstigatorActor.Get();
	if (!IsValid(FollowTarget))
	{
		return;
	}

	const FVector TargetLocation = FollowTarget->GetActorLocation() + FollowTarget->GetActorTransform().TransformVectorNoScale(Offset);
	const FVector NewLocation = FMath::VInterpTo(Owner->GetActorLocation(), TargetLocation, DeltaTime, InterpSpeed);
	MoveActorWithSweep(Owner, NewLocation);
}

void UOKMovementPolicy_Boomerang::InitializePolicy(AOKSkillObject* InOwner, const FOKSkillSpawnContext& InSpawnContext, const FOKSkillSpawnResult& InSpawnResult)
{
	Super::InitializePolicy(InOwner, InSpawnContext, InSpawnResult);

	if (AOKSkillObject* Owner = GetOwnerSkillObject())
	{
		if (UProjectileMovementComponent* ProjectileMovement = Owner->GetProjectileMovementComponent())
		{
			ProjectileMovement->Deactivate();
			ProjectileMovement->SetComponentTickEnabled(false);
		}

		StartLocation = Owner->GetActorLocation();
	}

	ForwardDirection = InSpawnResult.bHasInitialVelocity
		? InSpawnResult.InitialVelocity.GetSafeNormal()
		: ResolveAimDirection(InSpawnContext);
	bReturning = false;
}

void UOKMovementPolicy_Boomerang::TickPolicy(const float DeltaTime)
{
	AOKSkillObject* Owner = GetOwnerSkillObject();
	if (!Owner)
	{
		return;
	}

	const FVector CurrentLocation = Owner->GetActorLocation();

	if (!bReturning)
	{
		const FVector NextLocation = CurrentLocation + (ForwardDirection * ForwardSpeed * DeltaTime);
		MoveActorWithSweep(Owner, NextLocation);

		if (FVector::DistSquared(StartLocation, NextLocation) >= FMath::Square(MaxDistance))
		{
			bReturning = true;
		}

		return;
	}

	AActor* ReturnTarget = SpawnContext.InstigatorActor.Get();
	if (!IsValid(ReturnTarget))
	{
		Owner->RequestExpire(EOKSkillExpireReason::PolicyCondition);
		return;
	}

	const FVector ToTarget = ReturnTarget->GetActorLocation() - CurrentLocation;
	if (ToTarget.SizeSquared() <= FMath::Square(ArrivalDistance))
	{
		Owner->RequestExpire(EOKSkillExpireReason::PolicyCondition);
		return;
	}

	const float DistanceScale = FMath::Clamp(ToTarget.Size() / FMath::Max(ArrivalDistance, 1.0f), 1.0f, 3.0f);
	const FVector ReturnDirection = ToTarget.GetSafeNormal();
	const FVector NextLocation = CurrentLocation + (ReturnDirection * ReturnSpeed * DistanceScale * DeltaTime);
	MoveActorWithSweep(Owner, NextLocation);
	Owner->SetActorRotation(ReturnDirection.Rotation());
}

void UOKMovementPolicy_Wave::InitializePolicy(AOKSkillObject* InOwner, const FOKSkillSpawnContext& InSpawnContext, const FOKSkillSpawnResult& InSpawnResult)
{
	Super::InitializePolicy(InOwner, InSpawnContext, InSpawnResult);

	if (AOKSkillObject* Owner = GetOwnerSkillObject())
	{
		if (UProjectileMovementComponent* ProjectileMovement = Owner->GetProjectileMovementComponent())
		{
			ProjectileMovement->Deactivate();
			ProjectileMovement->SetComponentTickEnabled(false);
		}

		StartLocation = Owner->GetActorLocation();
	}

	ForwardDirection = InSpawnResult.bHasInitialVelocity
		? InSpawnResult.InitialVelocity.GetSafeNormal()
		: ResolveAimDirection(InSpawnContext);

	ElapsedTime = 0.0f;
}

void UOKMovementPolicy_Wave::TickPolicy(const float DeltaTime)
{
	AOKSkillObject* Owner = GetOwnerSkillObject();
	if (!Owner)
	{
		return;
	}

	ElapsedTime += DeltaTime;

	const FVector ForwardOffset = ForwardDirection * Speed * ElapsedTime;
	const FVector Right = FVector::CrossProduct(FVector::UpVector, ForwardDirection).GetSafeNormal();
	const FVector Up = FVector::UpVector;
	const float Sine = FMath::Sin(ElapsedTime * Frequency);
	const float Cosine = FMath::Cos(ElapsedTime * Frequency);

	FVector WaveOffset = FVector::ZeroVector;
	switch (WaveMode)
	{
	case EOKWaveMode::Horizontal2D:
		WaveOffset = Right * (Amplitude * Sine);
		break;
	case EOKWaveMode::Vertical2D:
		WaveOffset = Up * (Amplitude * Sine);
		break;
	case EOKWaveMode::Spiral3D:
	default:
		WaveOffset = (Right * (Amplitude * Sine)) + (Up * (Amplitude * Cosine));
		break;
	}

	MoveActorWithSweep(Owner, StartLocation + ForwardOffset + WaveOffset);
	Owner->SetActorRotation(ForwardDirection.Rotation());
}

