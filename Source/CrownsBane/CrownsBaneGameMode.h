#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CrownsBaneGameMode.generated.h"

class AWantedLevelManager;
class AWindSystem;
class AStormSystem;
class AEnemySpawner;
class AUpgradeManager;
class AShipPawn;

UCLASS(minimalapi)
class ACrownsBaneGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACrownsBaneGameMode();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Systems")
	TSubclassOf<AWantedLevelManager> WantedLevelManagerClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Systems")
	TSubclassOf<AWindSystem> WindSystemClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Systems")
	TSubclassOf<AStormSystem> StormSystemClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Systems")
	TSubclassOf<AEnemySpawner> EnemySpawnerClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Systems")
	TSubclassOf<AUpgradeManager> UpgradeManagerClass;

	UPROPERTY(BlueprintReadOnly, Category = "Game Systems")
	AWantedLevelManager* WantedLevelManager;

	UPROPERTY(BlueprintReadOnly, Category = "Game Systems")
	AWindSystem* WindSystem;

	UPROPERTY(BlueprintReadOnly, Category = "Game Systems")
	AStormSystem* StormSystem;

	UPROPERTY(BlueprintReadOnly, Category = "Game Systems")
	AEnemySpawner* EnemySpawner;

	UPROPERTY(BlueprintReadOnly, Category = "Game Systems")
	AUpgradeManager* UpgradeManager;

	UFUNCTION(BlueprintCallable, Category = "Game Systems")
	AWantedLevelManager* GetWantedLevelManager() const { return WantedLevelManager; }

	UFUNCTION(BlueprintCallable, Category = "Game Systems")
	AWindSystem* GetWindSystem() const { return WindSystem; }

	UFUNCTION(BlueprintCallable, Category = "Game Systems")
	AStormSystem* GetStormSystem() const { return StormSystem; }

	UFUNCTION(BlueprintCallable, Category = "Game Systems")
	AEnemySpawner* GetEnemySpawner() const { return EnemySpawner; }

private:
	// Failsafe: if Player 0 has no ShipPawn after startup, find one in level and possess it.
	// This handles the common case where BP_PlayerShip was dragged into the level but
	// GameMode's DefaultPawnClass or AutoPossessPlayer is misconfigured.
	void EnsurePlayerPossessesShip();
	FTimerHandle PossessRetryTimer;
	int32 PossessRetryCount = 0;
};
