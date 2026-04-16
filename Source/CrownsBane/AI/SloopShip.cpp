// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "AI/SloopShip.h"
#include "Combat/CannonComponent.h"
#include "Components/HealthComponent.h"

ASloopShip::ASloopShip()
{
	// Small, fast, fragile
	HealthComponent->MaxHealth = 75.0f;

	// Fast movement
	PatrolSpeed = 550.0f;
	ChaseSpeed = 900.0f;
	AttackSpeed = 750.0f;

	// Agile turning
	TurnRate = 55.0f;

	// 2 cannons per side
	CannonComponent->CannonsPerSide = 2;
	CannonComponent->DamagePerCannon = 15.0f;
	CannonComponent->ReloadTime = 3.5f;

	// Wants to keep distance
	PreferredEngagementDistance = 1800.0f;
	AttackRange = 2200.0f;
	DetectionRange = 4500.0f;
	FireRange = 1800.0f;
	FireCooldown = 4.0f;
}

void ASloopShip::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Log, TEXT("SloopShip: Spawned - fast, 2 cannons per side."));
}

void ASloopShip::HandleStateAttack(float DeltaTime)
{
	APawn* Player = GetPlayerPawn();
	if (!Player) return;

	float Dist = FVector::Dist(GetActorLocation(), Player->GetActorLocation());
	FVector ToPlayer = (Player->GetActorLocation() - GetActorLocation());
	ToPlayer.Z = 0.0f;
	FVector ToPlayerDir = ToPlayer.GetSafeNormal();

	FVector RightVec = GetActorRightVector();
	float DotRight = FVector::DotProduct(ToPlayerDir, RightVec);

	// Sloop tries to keep at preferred distance
	if (Dist < PreferredEngagementDistance * 0.7f)
	{
		// Too close - flee perpendicular while maintaining broadside
		FVector FleeDir = -ToPlayerDir;
		FVector Target = GetActorLocation() + FleeDir * 2000.0f;
		TurnToward(Target, DeltaTime);
		MoveToward(Target, AttackSpeed * 1.2f, DeltaTime);
	}
	else if (Dist > PreferredEngagementDistance * 1.3f)
	{
		// Too far - close in while turning for broadside
		TurnToward(Player->GetActorLocation(), DeltaTime);
		MoveToward(Player->GetActorLocation(), AttackSpeed, DeltaTime);
	}
	else
	{
		// At preferred distance: circle and maintain broadside
		// Pick a tangential direction to orbit the player
		FVector TangentDir = FVector::CrossProduct(ToPlayerDir, FVector::UpVector).GetSafeNormal();
		// Orbit direction: if player is on our right, orbit right
		float OrbitSign = (DotRight >= 0.0f) ? 1.0f : -1.0f;
		FVector OrbitTarget = GetActorLocation() + TangentDir * OrbitSign * 1500.0f;

		TurnToward(Player->GetActorLocation(), DeltaTime * 0.5f);
		MoveToward(OrbitTarget, AttackSpeed * 0.8f, DeltaTime);
	}

	TryFireAtPlayer();
}
