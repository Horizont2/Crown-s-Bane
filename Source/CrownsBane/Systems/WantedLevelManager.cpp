// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "Systems/WantedLevelManager.h"
#include "Systems/EnemySpawner.h"
#include "Kismet/GameplayStatics.h"

AWantedLevelManager::AWantedLevelManager()
{
	PrimaryActorTick.bCanEverTick = false;

	// Default kill thresholds: 1 kill = 1 star, 3 = 2 stars, 6 = 3, 10 = 4, 15 = 5
	KillsPerWantedLevel.Add(1);   // 1 kill = star 1
	KillsPerWantedLevel.Add(3);   // 3 kills = star 2
	KillsPerWantedLevel.Add(6);   // 6 kills = star 3
	KillsPerWantedLevel.Add(10);  // 10 kills = star 4
	KillsPerWantedLevel.Add(15);  // 15 kills = star 5
}

void AWantedLevelManager::BeginPlay()
{
	Super::BeginPlay();

	CurrentWantedLevel = 0;
	KillsSinceLastDock = 0;
	bGalleonDispatched = false;

	UE_LOG(LogTemp, Log, TEXT("WantedLevelManager: Initialized. Max level: %d"), MaxWantedLevel);
}

void AWantedLevelManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AWantedLevelManager::OnEnemyKilled()
{
	KillsSinceLastDock++;
	UE_LOG(LogTemp, Log, TEXT("WantedLevelManager: Kill registered. Total kills this session: %d"), KillsSinceLastDock);

	int32 OldLevel = CurrentWantedLevel;
	UpdateWantedLevel();

	if (CurrentWantedLevel != OldLevel)
	{
		UE_LOG(LogTemp, Log, TEXT("WantedLevelManager: Wanted level increased to %d!"), CurrentWantedLevel);
		OnWantedLevelChanged.Broadcast(CurrentWantedLevel);

		// Dispatch Galleons if wanted level is high enough
		if (CurrentWantedLevel >= GalleonDispatchLevel && !bGalleonDispatched)
		{
			bGalleonDispatched = true;
			TArray<AActor*> Spawners;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemySpawner::StaticClass(), Spawners);
			if (Spawners.Num() > 0)
			{
				AEnemySpawner* Spawner = Cast<AEnemySpawner>(Spawners[0]);
				if (Spawner)
				{
					Spawner->SpawnGalleonPursuit();
				}
			}
		}
	}
}

void AWantedLevelManager::ResetWantedLevel()
{
	int32 OldLevel = CurrentWantedLevel;
	KillsSinceLastDock = 0;
	CurrentWantedLevel = 0;
	bGalleonDispatched = false;

	if (OldLevel != 0)
	{
		UE_LOG(LogTemp, Log, TEXT("WantedLevelManager: Wanted level reset to 0 (docked)."));
		OnWantedLevelChanged.Broadcast(0);
	}
}

void AWantedLevelManager::UpdateWantedLevel()
{
	int32 NewLevel = 0;

	for (int32 i = 0; i < KillsPerWantedLevel.Num() && i < MaxWantedLevel; ++i)
	{
		if (KillsSinceLastDock >= KillsPerWantedLevel[i])
		{
			NewLevel = i + 1;
		}
		else
		{
			break;
		}
	}

	NewLevel = FMath::Clamp(NewLevel, 0, MaxWantedLevel);
	CurrentWantedLevel = NewLevel;
}
