// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "Systems/DynamicMusicManager.h"
#include "Systems/StormSystem.h"
#include "Ship/ShipPawn.h"
#include "AI/EnemyShipBase.h"
#include "Components/HealthComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

ADynamicMusicManager::ADynamicMusicManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ADynamicMusicManager::BeginPlay()
{
	Super::BeginPlay();
	CurrentState = EMusicState::Ambient;
}

void ADynamicMusicManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TimeSinceLastEval += DeltaTime;
	TimeSinceLastHit  += DeltaTime;
	TimeSinceLastKill += DeltaTime;

	if (TimeSinceLastEval >= EvaluationInterval)
	{
		TimeSinceLastEval = 0.0f;
		EvaluateState();
	}
}

void ADynamicMusicManager::NotifyCombatHit()
{
	TimeSinceLastHit = 0.0f;
}

void ADynamicMusicManager::NotifyEnemyKilled()
{
	TimeSinceLastKill = 0.0f;
}

void ADynamicMusicManager::EvaluateState()
{
	UWorld* W = GetWorld();
	if (!W) return;

	// Storm takes priority over combat — captains stop fighting when the sea decides.
	AStormSystem* Storm = nullptr;
	for (TActorIterator<AStormSystem> It(W); It; ++It) { Storm = *It; break; }
	const float StormIntensity = Storm ? Storm->GetStormIntensity() : 0.0f;

	// Player position for distance checks.
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(W, 0);
	const bool bHasPlayer = (PlayerPawn != nullptr);
	const FVector PlayerLoc = bHasPlayer ? PlayerPawn->GetActorLocation() : FVector::ZeroVector;

	// Closest hostile enemy distance.
	float ClosestEnemyDist2 = TNumericLimits<float>::Max();
	if (bHasPlayer)
	{
		for (TActorIterator<AEnemyShipBase> It(W); It; ++It)
		{
			AEnemyShipBase* E = *It;
			if (!E || !E->HealthComponent || !E->HealthComponent->IsAlive()) continue;
			const float D2 = FVector::DistSquared(PlayerLoc, E->GetActorLocation());
			if (D2 < ClosestEnemyDist2) ClosestEnemyDist2 = D2;
		}
	}

	EMusicState NewState = EMusicState::Ambient;
	if (StormIntensity >= StormThreshold)
	{
		NewState = EMusicState::Storm;
	}
	else if (TimeSinceLastHit < BattleCooldown
	      || ClosestEnemyDist2 < BattleRadius * BattleRadius)
	{
		NewState = EMusicState::Battle;
	}
	else if (TimeSinceLastKill < VictoryDuration)
	{
		NewState = EMusicState::Victory;
	}
	else if (ClosestEnemyDist2 < TensionRadius * TensionRadius)
	{
		NewState = EMusicState::Tension;
	}

	if (NewState != CurrentState)
	{
		const EMusicState Prev = CurrentState;
		CurrentState = NewState;
		OnMusicStateChanged.Broadcast(NewState, Prev);
	}
}
