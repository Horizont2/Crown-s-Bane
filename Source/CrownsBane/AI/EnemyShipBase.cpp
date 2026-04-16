// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "AI/EnemyShipBase.h"
#include "Combat/CannonComponent.h"
#include "Components/HealthComponent.h"
#include "Loot/LootSpawner.h"
#include "Systems/WantedLevelManager.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

AEnemyShipBase::AEnemyShipBase()
{
	PrimaryActorTick.bCanEverTick = true;

	ShipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShipMesh"));
	RootComponent = ShipMesh;
	ShipMesh->SetCollisionProfileName(TEXT("Pawn"));
	ShipMesh->SetSimulatePhysics(false);

	CannonComponent = CreateDefaultSubobject<UCannonComponent>(TEXT("CannonComponent"));
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
}

void AEnemyShipBase::BeginPlay()
{
	Super::BeginPlay();

	SpawnLocation = GetActorLocation();
	PatrolTarget = SpawnLocation;

	if (HealthComponent)
	{
		HealthComponent->OnDeath.AddDynamic(this, &AEnemyShipBase::OnDeathDelegate);
	}

	// Pick initial patrol target
	FVector RandomDir = FVector(FMath::FRandRange(-1.0f, 1.0f), FMath::FRandRange(-1.0f, 1.0f), 0.0f).GetSafeNormal();
	PatrolTarget = SpawnLocation + RandomDir * PatrolRadius * FMath::FRandRange(0.3f, 1.0f);

	UE_LOG(LogTemp, Log, TEXT("EnemyShipBase: %s spawned. Initial state: Patrol."), *GetName());
}

void AEnemyShipBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CurrentState == EShipAIState::Sink)
	{
		HandleStateSink(DeltaTime);
		return;
	}

	// Tick fire cooldown
	if (FireCooldownTimer > 0.0f)
	{
		FireCooldownTimer -= DeltaTime;
	}

	// Check for state transitions based on player distance
	if (CurrentState != EShipAIState::Sink)
	{
		if (IsPlayerInRange(DetectionRange))
		{
			if (IsPlayerInRange(AttackRange))
			{
				if (CurrentState != EShipAIState::Attack)
				{
					TransitionToState(EShipAIState::Attack);
				}
			}
			else
			{
				if (CurrentState != EShipAIState::Chase)
				{
					TransitionToState(EShipAIState::Chase);
				}
			}
		}
		else
		{
			if (CurrentState != EShipAIState::Patrol)
			{
				TransitionToState(EShipAIState::Patrol);
			}
		}
	}

	switch (CurrentState)
	{
	case EShipAIState::Patrol:
		HandleStatePatrol(DeltaTime);
		break;
	case EShipAIState::Chase:
		HandleStateChase(DeltaTime);
		break;
	case EShipAIState::Attack:
		HandleStateAttack(DeltaTime);
		break;
	case EShipAIState::Sink:
		HandleStateSink(DeltaTime);
		break;
	}
}

float AEnemyShipBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	float Actual = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	// HealthComponent responds via OnTakeAnyDamage binding
	return Actual;
}

void AEnemyShipBase::TransitionToState(EShipAIState NewState)
{
	if (CurrentState == NewState) return;

	UE_LOG(LogTemp, Log, TEXT("EnemyShipBase: %s transitioning from %d to %d"),
		*GetName(), (int32)CurrentState, (int32)NewState);
	CurrentState = NewState;
}

bool AEnemyShipBase::IsPlayerInRange(float Range) const
{
	APawn* Player = GetPlayerPawn();
	if (!Player) return false;
	float DistSq = FVector::DistSquared(GetActorLocation(), Player->GetActorLocation());
	return DistSq <= (Range * Range);
}

APawn* AEnemyShipBase::GetPlayerPawn() const
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	return PC ? PC->GetPawn() : nullptr;
}

bool AEnemyShipBase::IsBroadsideAligned(ECannonSide& OutSide) const
{
	APawn* Player = GetPlayerPawn();
	if (!Player) return false;

	FVector ToPlayer = (Player->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	FVector RightVec = GetActorRightVector();
	float DotRight = FVector::DotProduct(ToPlayer, RightVec);
	float DotForward = FVector::DotProduct(ToPlayer, GetActorForwardVector());

	// Only fire broadside if player is more to the side than in front/behind
	if (FMath::Abs(DotRight) > FMath::Abs(DotForward))
	{
		OutSide = (DotRight > 0.0f) ? ECannonSide::Right : ECannonSide::Left;
		return true;
	}
	return false;
}

void AEnemyShipBase::TryFireAtPlayer()
{
	if (!CannonComponent) return;
	if (FireCooldownTimer > 0.0f) return;

	ECannonSide Side;
	if (IsBroadsideAligned(Side))
	{
		if (CannonComponent->CanFire(Side))
		{
			CannonComponent->FireBroadside(Side);
			FireCooldownTimer = FireCooldown;
		}
	}
}

void AEnemyShipBase::HandleStatePatrol(float DeltaTime)
{
	// Move toward patrol target
	float DistToTarget = FVector::Dist(GetActorLocation(), PatrolTarget);
	if (DistToTarget < 300.0f)
	{
		// Reached patrol target, pick a new one after a short wait
		PatrolWaitTimer += DeltaTime;
		if (PatrolWaitTimer >= 3.0f)
		{
			PatrolWaitTimer = 0.0f;
			FVector RandomDir = FVector(FMath::FRandRange(-1.0f, 1.0f), FMath::FRandRange(-1.0f, 1.0f), 0.0f).GetSafeNormal();
			PatrolTarget = SpawnLocation + RandomDir * PatrolRadius * FMath::FRandRange(0.3f, 1.0f);
		}
		CurrentSpeedActual = 0.0f;
	}
	else
	{
		PatrolWaitTimer = 0.0f;
		TurnToward(PatrolTarget, DeltaTime);
		MoveToward(PatrolTarget, PatrolSpeed, DeltaTime);
	}
}

void AEnemyShipBase::HandleStateChase(float DeltaTime)
{
	APawn* Player = GetPlayerPawn();
	if (!Player) return;

	TurnToward(Player->GetActorLocation(), DeltaTime);
	MoveToward(Player->GetActorLocation(), ChaseSpeed, DeltaTime);
}

void AEnemyShipBase::HandleStateAttack(float DeltaTime)
{
	APawn* Player = GetPlayerPawn();
	if (!Player) return;

	float Dist = FVector::Dist(GetActorLocation(), Player->GetActorLocation());

	// Default: maneuver to get broadside aligned, then maintain distance
	FVector ToPlayer = (Player->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	FVector RightVec = GetActorRightVector();
	float DotRight = FVector::DotProduct(ToPlayer, RightVec);

	// If player is mostly ahead, turn to put them to the side
	if (FMath::Abs(DotRight) < 0.5f)
	{
		// Turn perpendicular to face player with broadside
		FVector TurnTarget = GetActorLocation() + RightVec * (DotRight >= 0.0f ? 1.0f : -1.0f) * 2000.0f;
		TurnToward(Player->GetActorLocation(), DeltaTime);
	}
	else
	{
		// Maintain preferred engagement distance
		if (Dist < PreferredEngagementDistance - 200.0f)
		{
			// Too close, back off perpendicular
			FVector SideDir = (DotRight > 0.0f) ? RightVec : -RightVec;
			FVector BackTarget = GetActorLocation() - GetActorForwardVector() * 500.0f + SideDir * 300.0f;
			TurnToward(BackTarget, DeltaTime);
		}
		else
		{
			TurnToward(Player->GetActorLocation(), DeltaTime);
		}
	}

	MoveToward(Player->GetActorLocation(), AttackSpeed, DeltaTime);
	TryFireAtPlayer();
}

void AEnemyShipBase::HandleStateSink(float DeltaTime)
{
	SinkTimer += DeltaTime;
	// Slowly sink (move down) and rotate
	AddActorWorldOffset(FVector(0.0f, 0.0f, -30.0f * DeltaTime));
	AddActorLocalRotation(FRotator(1.5f * DeltaTime, 0.0f, 2.0f * DeltaTime));

	if (SinkTimer >= 3.0f)
	{
		Destroy();
	}
}

void AEnemyShipBase::MoveToward(FVector TargetLocation, float Speed, float DeltaTime)
{
	FVector ToTarget = (TargetLocation - GetActorLocation());
	ToTarget.Z = 0.0f;
	float Dist = ToTarget.Size();

	if (Dist < 1.0f)
	{
		CurrentSpeedActual = 0.0f;
		return;
	}

	// Slow down as we approach
	float EffectiveSpeed = FMath::Min(Speed, Dist / DeltaTime);
	CurrentSpeedActual = EffectiveSpeed;

	FVector MoveDir = GetActorForwardVector();
	MoveDir.Z = 0.0f;
	AddActorWorldOffset(MoveDir * EffectiveSpeed * DeltaTime, true);
}

void AEnemyShipBase::TurnToward(FVector TargetLocation, float DeltaTime)
{
	FVector ToTarget = (TargetLocation - GetActorLocation());
	ToTarget.Z = 0.0f;
	if (ToTarget.IsNearlyZero()) return;

	ToTarget.Normalize();
	FVector Forward = GetActorForwardVector();
	Forward.Z = 0.0f;
	Forward.Normalize();

	float DotForward = FVector::DotProduct(Forward, ToTarget);
	float DotRight = FVector::DotProduct(GetActorRightVector(), ToTarget);

	// Determine turn direction
	float TurnDir = (DotRight > 0.0f) ? 1.0f : -1.0f;
	float AngleToTarget = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(DotForward, -1.0f, 1.0f)));

	float YawDelta = TurnDir * FMath::Min(TurnRate * DeltaTime, AngleToTarget);
	AddActorLocalRotation(FRotator(0.0f, YawDelta, 0.0f));
}

void AEnemyShipBase::OnDeathDelegate()
{
	OnDeath();
}

void AEnemyShipBase::OnDeath()
{
	UE_LOG(LogTemp, Log, TEXT("EnemyShipBase: %s is sinking!"), *GetName());

	TransitionToState(EShipAIState::Sink);

	// Notify wanted level manager
	TArray<AActor*> Managers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AWantedLevelManager::StaticClass(), Managers);
	if (Managers.Num() > 0)
	{
		AWantedLevelManager* WLM = Cast<AWantedLevelManager>(Managers[0]);
		if (WLM)
		{
			WLM->OnEnemyKilled();
		}
	}

	// Spawn loot
	if (LootSpawnerClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		GetWorld()->SpawnActor<AActor>(LootSpawnerClass, GetActorLocation(), FRotator::ZeroRotator, SpawnParams);
	}
	else
	{
		// Spawn default loot spawner
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		GetWorld()->SpawnActor<ALootSpawner>(ALootSpawner::StaticClass(), GetActorLocation(), FRotator::ZeroRotator, SpawnParams);
	}

	// Disable cannon
	if (CannonComponent)
	{
		CannonComponent->Deactivate();
	}
}
