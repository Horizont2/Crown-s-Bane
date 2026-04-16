// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "Systems/EnemySpawner.h"
#include "AI/EnemyShipBase.h"
#include "AI/SloopShip.h"
#include "AI/BrigShip.h"
#include "AI/GalleonShip.h"
#include "AI/ShipAIController.h"
#include "Systems/WantedLevelManager.h"
#include "Docks/DocksZone.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

AEnemySpawner::AEnemySpawner()
{
	PrimaryActorTick.bCanEverTick = true;
	SpawnTimer = SpawnInterval * 0.5f; // First spawn at half interval
}

void AEnemySpawner::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Log, TEXT("EnemySpawner: Initialized. Spawn interval: %.1fs, Max enemies: %d"),
		SpawnInterval, MaxEnemies);
}

void AEnemySpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SpawnTimer += DeltaTime;
	if (SpawnTimer >= SpawnInterval)
	{
		SpawnTimer = 0.0f;
		TrySpawnEnemy();
	}
}

AWantedLevelManager* AEnemySpawner::GetWantedLevelManager() const
{
	TArray<AActor*> Managers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AWantedLevelManager::StaticClass(), Managers);
	return (Managers.Num() > 0) ? Cast<AWantedLevelManager>(Managers[0]) : nullptr;
}

int32 AEnemySpawner::GetActiveEnemyCount() const
{
	int32 Count = 0;
	for (const auto& EnemyPtr : ActiveEnemies)
	{
		if (EnemyPtr.IsValid())
		{
			++Count;
		}
	}
	return Count;
}

void AEnemySpawner::TrySpawnEnemy()
{
	// Clean up invalid enemy references
	ActiveEnemies.RemoveAll([](const TWeakObjectPtr<AEnemyShipBase>& Ptr) {
		return !Ptr.IsValid();
	});

	if (GetActiveEnemyCount() >= MaxEnemies)
	{
		UE_LOG(LogTemp, Log, TEXT("EnemySpawner: Max enemies reached (%d), skipping spawn."), MaxEnemies);
		return;
	}

	FVector SpawnLoc = FindSpawnLocation();
	if (SpawnLoc.IsZero())
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemySpawner: Could not find valid spawn location."));
		return;
	}

	AWantedLevelManager* WLM = GetWantedLevelManager();
	int32 WantedLevel = WLM ? WLM->GetWantedLevel() : 0;

	TSubclassOf<AEnemyShipBase> ShipClass = PickShipClassByWantedLevel(WantedLevel);
	if (!ShipClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemySpawner: No valid ship class for wanted level %d"), WantedLevel);
		return;
	}

	FRotator SpawnRot = FRotator(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f);
	AEnemyShipBase* NewShip = SpawnShipAt(ShipClass, SpawnLoc, SpawnRot);
	if (NewShip)
	{
		ActiveEnemies.Add(NewShip);
		UE_LOG(LogTemp, Log, TEXT("EnemySpawner: Spawned %s at %s. Active enemies: %d"),
			*NewShip->GetName(), *SpawnLoc.ToString(), GetActiveEnemyCount());
	}
}

AEnemyShipBase* AEnemySpawner::SpawnShipAt(TSubclassOf<AEnemyShipBase> ShipClass, FVector Location, FRotator Rotation)
{
	if (!ShipClass) return nullptr;

	UWorld* World = GetWorld();
	if (!World) return nullptr;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AEnemyShipBase* Ship = World->SpawnActor<AEnemyShipBase>(ShipClass, Location, Rotation, SpawnParams);
	if (Ship)
	{
		// Spawn and attach an AI controller
		AShipAIController* AIController = World->SpawnActor<AShipAIController>(
			AShipAIController::StaticClass(), Location, Rotation, SpawnParams);
		if (AIController)
		{
			AIController->Possess(Ship);
		}
	}

	return Ship;
}

void AEnemySpawner::SpawnGalleonPursuit()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC || !PC->GetPawn()) return;

	FVector PlayerLoc = PC->GetPawn()->GetActorLocation();

	// Spawn 1-2 Galleons at max spawn distance, aimed at player
	int32 NumGalleons = FMath::RandRange(1, 2);
	for (int32 i = 0; i < NumGalleons; ++i)
	{
		float Angle = FMath::FRandRange(0.0f, 360.0f);
		FVector Offset = FVector(FMath::Cos(FMath::DegreesToRadians(Angle)),
			FMath::Sin(FMath::DegreesToRadians(Angle)), 0.0f) * MaxSpawnDistance;
		FVector SpawnLoc = PlayerLoc + Offset;

		// Look toward player
		FVector ToPlayer = (PlayerLoc - SpawnLoc).GetSafeNormal();
		FRotator SpawnRot = ToPlayer.Rotation();

		TSubclassOf<AEnemyShipBase> GClass = GalleonClass
			? TSubclassOf<AEnemyShipBase>(GalleonClass)
			: TSubclassOf<AEnemyShipBase>(AGalleonShip::StaticClass());

		AEnemyShipBase* Ship = SpawnShipAt(GClass, SpawnLoc, SpawnRot);
		if (Ship)
		{
			ActiveEnemies.Add(Ship);
			UE_LOG(LogTemp, Warning, TEXT("EnemySpawner: GALLEON DISPATCHED! Wanted level 4+."));
		}
	}
}

FVector AEnemySpawner::FindSpawnLocation() const
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC || !PC->GetPawn()) return FVector::ZeroVector;

	FVector PlayerLoc = PC->GetPawn()->GetActorLocation();

	// Try several random positions
	for (int32 Attempt = 0; Attempt < 10; ++Attempt)
	{
		float Angle = FMath::FRandRange(0.0f, 360.0f);
		float Dist = FMath::FRandRange(MinSpawnDistance, MaxSpawnDistance);
		FVector Offset = FVector(FMath::Cos(FMath::DegreesToRadians(Angle)),
			FMath::Sin(FMath::DegreesToRadians(Angle)), 0.0f) * Dist;
		FVector CandidateLoc = PlayerLoc + Offset;
		CandidateLoc.Z = PlayerLoc.Z; // Maintain water level

		if (IsLocationSafe(CandidateLoc))
		{
			return CandidateLoc;
		}
	}

	return FVector::ZeroVector;
}

bool AEnemySpawner::IsLocationSafe(FVector Location) const
{
	// Check that we're not spawning inside the safe zone of any docks
	TArray<AActor*> DocksActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADocksZone::StaticClass(), DocksActors);
	for (AActor* DocksActor : DocksActors)
	{
		ADocksZone* Docks = Cast<ADocksZone>(DocksActor);
		if (Docks && Docks->IsLocationInSafeZone(Location))
		{
			return false;
		}
	}
	return true;
}

TSubclassOf<AEnemyShipBase> AEnemySpawner::PickShipClassByWantedLevel(int32 WantedLevel) const
{
	float SloopW = SloopSpawnWeight;
	float BrigW = BrigSpawnWeight;
	float GalleonW = GalleonSpawnWeight;

	// At low wanted levels, only spawn sloops and brigs
	if (WantedLevel <= 1)
	{
		SloopW = 0.8f;
		BrigW = 0.2f;
		GalleonW = 0.0f;
	}
	else if (WantedLevel <= 3)
	{
		SloopW = 0.5f;
		BrigW = 0.4f;
		GalleonW = 0.1f;
	}

	float Total = SloopW + BrigW + GalleonW;
	float Roll = FMath::FRandRange(0.0f, Total);

	if (Roll < SloopW)
	{
		return SloopClass ? TSubclassOf<AEnemyShipBase>(SloopClass)
			: TSubclassOf<AEnemyShipBase>(ASloopShip::StaticClass());
	}
	else if (Roll < SloopW + BrigW)
	{
		return BrigClass ? TSubclassOf<AEnemyShipBase>(BrigClass)
			: TSubclassOf<AEnemyShipBase>(ABrigShip::StaticClass());
	}
	else
	{
		return GalleonClass ? TSubclassOf<AEnemyShipBase>(GalleonClass)
			: TSubclassOf<AEnemyShipBase>(AGalleonShip::StaticClass());
	}
}
