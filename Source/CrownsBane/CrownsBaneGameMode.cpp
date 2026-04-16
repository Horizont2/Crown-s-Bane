// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "CrownsBaneGameMode.h"
#include "Systems/WantedLevelManager.h"
#include "Systems/WindSystem.h"
#include "Systems/EnemySpawner.h"
#include "Player/CrownsBanePlayerController.h"
#include "Ship/ShipPawn.h"
#include "UI/CrownsBaneHUD.h"
#include "UObject/ConstructorHelpers.h"

ACrownsBaneGameMode::ACrownsBaneGameMode()
{
	PrimaryActorTick.bCanEverTick = true;

	DefaultPawnClass = AShipPawn::StaticClass();
	PlayerControllerClass = ACrownsBanePlayerController::StaticClass();
	HUDClass = ACrownsBaneHUD::StaticClass();
}

void ACrownsBaneGameMode::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Spawn Wind System
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;

	if (WindSystemClass)
	{
		WindSystem = World->SpawnActor<AWindSystem>(WindSystemClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	}
	else
	{
		WindSystem = World->SpawnActor<AWindSystem>(AWindSystem::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	}

	// Spawn Wanted Level Manager
	if (WantedLevelManagerClass)
	{
		WantedLevelManager = World->SpawnActor<AWantedLevelManager>(WantedLevelManagerClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	}
	else
	{
		WantedLevelManager = World->SpawnActor<AWantedLevelManager>(AWantedLevelManager::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	}

	// Spawn Enemy Spawner
	if (EnemySpawnerClass)
	{
		EnemySpawner = World->SpawnActor<AEnemySpawner>(EnemySpawnerClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	}
	else
	{
		EnemySpawner = World->SpawnActor<AEnemySpawner>(AEnemySpawner::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	}

	UE_LOG(LogTemp, Log, TEXT("CrownsBaneGameMode: All game systems initialized."));
}

void ACrownsBaneGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
