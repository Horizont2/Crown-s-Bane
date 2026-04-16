// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "AI/BrigShip.h"
#include "Combat/CannonComponent.h"
#include "Components/HealthComponent.h"

ABrigShip::ABrigShip()
{
	// Medium, balanced
	HealthComponent->MaxHealth = 150.0f;

	// Moderate speed
	PatrolSpeed = 400.0f;
	ChaseSpeed = 650.0f;
	AttackSpeed = 550.0f;

	// Moderate turning
	TurnRate = 40.0f;

	// 4 cannons per side
	CannonComponent->CannonsPerSide = 4;
	CannonComponent->DamagePerCannon = 20.0f;
	CannonComponent->ReloadTime = 4.5f;

	// Closes in for broadside
	PreferredEngagementDistance = 1000.0f;
	AttackRange = 2000.0f;
	DetectionRange = 4000.0f;
	FireRange = 1600.0f;
	FireCooldown = 5.0f;
}

void ABrigShip::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Log, TEXT("BrigShip: Spawned - balanced, 4 cannons per side."));
}

void ABrigShip::HandleStateAttack(float DeltaTime)
{
	APawn* Player = GetPlayerPawn();
	if (!Player) return;

	float Dist = FVector::Dist(GetActorLocation(), Player->GetActorLocation());
	FVector ToPlayer = (Player->GetActorLocation() - GetActorLocation());
	ToPlayer.Z = 0.0f;
	FVector ToPlayerDir = ToPlayer.GetSafeNormal();

	FVector RightVec = GetActorRightVector();
	float DotRight = FVector::DotProduct(ToPlayerDir, RightVec);
	float DotForward = FVector::DotProduct(ToPlayerDir, GetActorForwardVector());

	// Brig aggressively closes distance and then presents broadside
	if (Dist > PreferredEngagementDistance)
	{
		// Chase player to close distance
		TurnToward(Player->GetActorLocation(), DeltaTime);
		MoveToward(Player->GetActorLocation(), ChaseSpeed, DeltaTime);
	}
	else
	{
		// At close range: turn to present broadside
		if (FMath::Abs(DotRight) < 0.6f)
		{
			// Not broadside-aligned yet, turn sharply
			FVector TurnTarget = GetActorLocation() + RightVec * (DotRight >= 0.0f ? 1.0f : -1.0f) * 3000.0f;
			// Rotate 90 degrees relative to player direction
			FVector PerpendicularDir = FVector::CrossProduct(ToPlayerDir, FVector::UpVector).GetSafeNormal();
			FVector BroadsideTarget = GetActorLocation() + PerpendicularDir * 2000.0f;
			TurnToward(BroadsideTarget, DeltaTime * 1.5f);
		}
		else
		{
			// Hold position and fire
			TurnToward(Player->GetActorLocation(), DeltaTime * 0.3f);
		}

		// Slow move to stay close but not collide
		if (Dist > PreferredEngagementDistance * 0.5f)
		{
			MoveToward(Player->GetActorLocation(), AttackSpeed * 0.4f, DeltaTime);
		}
		else
		{
			// Hold position
			MoveToward(GetActorLocation() - GetActorForwardVector() * 100.0f, AttackSpeed * 0.2f, DeltaTime);
		}
	}

	TryFireAtPlayer();
}
