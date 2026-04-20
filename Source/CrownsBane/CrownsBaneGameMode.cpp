// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "CrownsBaneGameMode.h"
#include "Systems/WantedLevelManager.h"
#include "Systems/WindSystem.h"
#include "Systems/EnemySpawner.h"
#include "Upgrades/UpgradeManager.h"
#include "Player/CrownsBanePlayerController.h"
#include "Ship/ShipPawn.h"
#include "UI/CrownsBaneHUD.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "TimerManager.h"

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

	// Spawn Upgrade Manager
	if (UpgradeManagerClass)
	{
		UpgradeManager = World->SpawnActor<AUpgradeManager>(UpgradeManagerClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	}
	else
	{
		UpgradeManager = World->SpawnActor<AUpgradeManager>(AUpgradeManager::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	}

	UE_LOG(LogTemp, Log, TEXT("CrownsBaneGameMode: All game systems initialized."));

	// Failsafe: retry possession a few times to cover BP configurations where
	// AutoPossessPlayer gets overridden or DefaultPawnClass points elsewhere.
	World->GetTimerManager().SetTimer(PossessRetryTimer, this,
		&ACrownsBaneGameMode::EnsurePlayerPossessesShip, 0.25f, true, 0.25f);
}

void ACrownsBaneGameMode::EnsurePlayerPossessesShip()
{
	UWorld* World = GetWorld();
	if (!World) return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (!PC)
	{
		if (++PossessRetryCount > 20)
			World->GetTimerManager().ClearTimer(PossessRetryTimer);
		return;
	}

	if (AShipPawn* Current = Cast<AShipPawn>(PC->GetPawn()))
	{
		UE_LOG(LogTemp, Log, TEXT("[GameMode] Player already possesses ShipPawn '%s'. Failsafe disarmed."),
			*Current->GetName());
		World->GetTimerManager().ClearTimer(PossessRetryTimer);
		return;
	}

	// Find the first ShipPawn in the level.
	AShipPawn* FoundShip = nullptr;
	for (TActorIterator<AShipPawn> It(World); It; ++It)
	{
		AShipPawn* Candidate = *It;
		if (!Candidate || Candidate->IsPendingKillPending()) continue;
		// Prefer one not controlled by AI or another PC.
		if (Candidate->GetController() == nullptr || Candidate->GetController() == PC)
		{
			FoundShip = Candidate;
			break;
		}
	}

	if (FoundShip)
	{
		if (APawn* OldPawn = PC->GetPawn())
		{
			PC->UnPossess();
		}
		PC->Possess(FoundShip);
		UE_LOG(LogTemp, Warning,
			TEXT("[GameMode] Failsafe: force-possessed ShipPawn '%s' for Player 0."),
			*FoundShip->GetName());

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan,
				FString::Printf(TEXT("GameMode: force-possessed %s"), *FoundShip->GetName()));
		}
		World->GetTimerManager().ClearTimer(PossessRetryTimer);
	}
	else if (++PossessRetryCount > 20)
	{
		UE_LOG(LogTemp, Error, TEXT("[GameMode] Failsafe: no AShipPawn found in level after retries."));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red,
				TEXT("No AShipPawn in level! Drag BP_PlayerShip into OceanMap."));
		}
		World->GetTimerManager().ClearTimer(PossessRetryTimer);
	}
}

void ACrownsBaneGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
