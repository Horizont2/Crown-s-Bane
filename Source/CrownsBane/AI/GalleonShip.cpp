// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "AI/GalleonShip.h"
#include "Combat/CannonComponent.h"
#include "Components/HealthComponent.h"

AGalleonShip::AGalleonShip()
{
	// Massive, slow, tanky
	HealthComponent->MaxHealth = 600.0f;

	// Slow movement
	PatrolSpeed = 200.0f;
	ChaseSpeed = 350.0f;
	AttackSpeed = 280.0f;

	// Slow turning - big ship
	TurnRate = 18.0f;

	// 10 cannons per side - massive broadside
	CannonComponent->CannonsPerSide = 10;
	CannonComponent->DamagePerCannon = 30.0f;
	CannonComponent->ReloadTime = 7.0f;

	// Close to medium range engagement
	PreferredEngagementDistance = 1200.0f;
	AttackRange = 2500.0f;
	DetectionRange = 5000.0f;
	FireRange = 2000.0f;
	FireCooldown = 6.0f;

	BaseFireCooldown = 6.0f;
	BaseChaseSpeed = 350.0f;
}

void AGalleonShip::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Log, TEXT("GalleonShip: Spawned - massive, 10 cannons per side, boss tactics."));
}

void AGalleonShip::CheckEnrage()
{
	if (bIsEnraged) return;
	if (!HealthComponent) return;

	float HealthPct = HealthComponent->GetHealthPercent();
	if (HealthPct <= EnrageHealthThreshold)
	{
		bIsEnraged = true;
		ChaseSpeed = BaseChaseSpeed * EnragedSpeedMultiplier;
		AttackSpeed = AttackSpeed * EnragedSpeedMultiplier;
		FireCooldown = BaseFireCooldown * EnragedFireCooldownMultiplier;
		CannonComponent->ReloadTime = CannonComponent->ReloadTime * 0.6f;

		UE_LOG(LogTemp, Warning, TEXT("GalleonShip: %s is ENRAGED! Speed and fire rate increased!"), *GetName());
	}
}

void AGalleonShip::HandleStateChase(float DeltaTime)
{
	CheckEnrage();

	APawn* Player = GetPlayerPawn();
	if (!Player) return;

	// Galleon persistently chases - it never gives up
	// It also fires while chasing if broadside is aligned
	TurnToward(Player->GetActorLocation(), DeltaTime);
	MoveToward(Player->GetActorLocation(), ChaseSpeed, DeltaTime);

	// Fire if we have a broadside shot opportunity
	TryFireAtPlayer();
}

void AGalleonShip::HandleStateAttack(float DeltaTime)
{
	CheckEnrage();

	APawn* Player = GetPlayerPawn();
	if (!Player) return;

	float Dist = FVector::Dist(GetActorLocation(), Player->GetActorLocation());
	FVector ToPlayer = (Player->GetActorLocation() - GetActorLocation());
	ToPlayer.Z = 0.0f;
	FVector ToPlayerDir = ToPlayer.GetSafeNormal();

	FVector RightVec = GetActorRightVector();
	float DotRight = FVector::DotProduct(ToPlayerDir, RightVec);
	float DotForward = FVector::DotProduct(ToPlayerDir, GetActorForwardVector());

	// Galleon boss tactic: slowly sweep sideways to present broadsides
	// It tries to keep player at broadside range and dump all cannons

	if (FMath::Abs(DotRight) < 0.5f)
	{
		// Turn to bring player to the side
		// Determine which side is closer to being aligned
		FVector PerpendicularTarget;
		if (DotForward > 0.0f)
		{
			// Player is ahead - turn to either side based on DotRight
			FVector PerpendicularDir = FVector::CrossProduct(FVector::UpVector, ToPlayerDir).GetSafeNormal();
			PerpendicularTarget = GetActorLocation() + PerpendicularDir * 3000.0f;
		}
		else
		{
			// Player is behind - turn to face them first
			PerpendicularTarget = Player->GetActorLocation();
		}
		TurnToward(PerpendicularTarget, DeltaTime);
	}
	else
	{
		// Broadside mostly aligned - small corrections only
		TurnToward(Player->GetActorLocation(), DeltaTime * 0.2f);
	}

	// Move slowly to maintain engagement distance
	if (Dist > PreferredEngagementDistance * 1.2f)
	{
		MoveToward(Player->GetActorLocation(), AttackSpeed, DeltaTime);
	}
	else if (Dist < PreferredEngagementDistance * 0.8f)
	{
		// Back away slightly - keep the player in broadside range
		FVector BackTarget = GetActorLocation() - GetActorForwardVector() * 500.0f;
		MoveToward(BackTarget, AttackSpeed * 0.5f, DeltaTime);
	}
	else
	{
		MoveToward(GetActorLocation() + GetActorForwardVector() * 200.0f, AttackSpeed * 0.2f, DeltaTime);
	}

	// Fire relentlessly
	TryFireAtPlayer();

	// Enraged galleon also fires alternating broadsides
	if (bIsEnraged && FireCooldownTimer <= 0.0f)
	{
		// Also try the other side
		ECannonSide Side;
		if (!IsBroadsideAligned(Side))
		{
			// Try both sides
			if (CannonComponent->CanFire(ECannonSide::Left))
			{
				CannonComponent->FireBroadside(ECannonSide::Left);
			}
			if (CannonComponent->CanFire(ECannonSide::Right))
			{
				CannonComponent->FireBroadside(ECannonSide::Right);
			}
			FireCooldownTimer = FireCooldown;
		}
	}
}
