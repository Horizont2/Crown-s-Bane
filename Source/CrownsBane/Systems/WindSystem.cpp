// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "Systems/WindSystem.h"

AWindSystem::AWindSystem()
{
	PrimaryActorTick.bCanEverTick = true;

	// Start with a random wind direction
	float Angle = FMath::FRandRange(0.0f, 360.0f);
	CurrentWindDirection = FVector(FMath::Cos(FMath::DegreesToRadians(Angle)),
		FMath::Sin(FMath::DegreesToRadians(Angle)), 0.0f).GetSafeNormal();
	TargetWindDirection = CurrentWindDirection;
}

void AWindSystem::BeginPlay()
{
	Super::BeginPlay();

	// Pick an initial random target
	PickNewTargetWindDirection();

	UE_LOG(LogTemp, Log, TEXT("WindSystem: Initial wind direction: X=%.2f, Y=%.2f"),
		CurrentWindDirection.X, CurrentWindDirection.Y);
}

void AWindSystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Slowly rotate current wind direction toward target
	FVector PrevDir = CurrentWindDirection;

	// Spherical interpolation
	float AngleBetween = FMath::RadiansToDegrees(
		FMath::Acos(FMath::Clamp(FVector::DotProduct(CurrentWindDirection, TargetWindDirection), -1.0f, 1.0f)));

	if (AngleBetween > 0.5f)
	{
		float MaxRotation = WindChangeRate * DeltaTime;
		float AlphaThisFrame = FMath::Clamp(MaxRotation / AngleBetween, 0.0f, 1.0f);
		CurrentWindDirection = FMath::Lerp(CurrentWindDirection, TargetWindDirection, AlphaThisFrame);
		CurrentWindDirection.Z = 0.0f;
		CurrentWindDirection.Normalize();

		// Notify if direction changed significantly
		if (!CurrentWindDirection.Equals(PrevDir, 0.01f))
		{
			OnWindDirectionChanged.Broadcast(CurrentWindDirection);
		}
	}

	// Check if it's time to shift wind direction
	WindShiftTimer += DeltaTime;
	if (WindShiftTimer >= WindShiftInterval)
	{
		WindShiftTimer = 0.0f;
		PickNewTargetWindDirection();
	}
}

void AWindSystem::PickNewTargetWindDirection()
{
	// Pick a new random angle, but not too far from current (within 120 degrees)
	float CurrentAngle = FMath::Atan2(CurrentWindDirection.Y, CurrentWindDirection.X);
	float NewAngle = CurrentAngle + FMath::DegreesToRadians(FMath::FRandRange(-120.0f, 120.0f));

	TargetWindDirection = FVector(FMath::Cos(NewAngle), FMath::Sin(NewAngle), 0.0f).GetSafeNormal();

	UE_LOG(LogTemp, Log, TEXT("WindSystem: New target wind direction: X=%.2f, Y=%.2f"),
		TargetWindDirection.X, TargetWindDirection.Y);
}

float AWindSystem::GetWindSpeedMultiplier(FVector ShipForward) const
{
	// Dot product: +1 if ship faces same direction as wind (tailwind), -1 if facing into wind (headwind)
	FVector Forward2D = ShipForward;
	Forward2D.Z = 0.0f;
	Forward2D.Normalize();

	float Dot = FVector::DotProduct(Forward2D, CurrentWindDirection);
	// Return raw dot in [-1, 1], caller applies their WindInfluenceFactor
	return Dot * WindStrength;
}
