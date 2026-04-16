#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemySpawner.generated.h"

class AEnemyShipBase;
class ASloopShip;
class ABrigShip;
class AGalleonShip;
class ADocksZone;
class AWantedLevelManager;

UCLASS()
class CROWNSBANE_API AEnemySpawner : public AActor
{
	GENERATED_BODY()

public:
	AEnemySpawner();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// Spawn a pursuit Galleon near the player (called by WantedLevelManager at level 4+)
	UFUNCTION(BlueprintCallable, Category = "Spawner")
	void SpawnGalleonPursuit();

	// Force spawn a specific ship type at a location
	UFUNCTION(BlueprintCallable, Category = "Spawner")
	AEnemyShipBase* SpawnShipAt(TSubclassOf<AEnemyShipBase> ShipClass, FVector Location, FRotator Rotation);

	// Ship classes to spawn
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	TSubclassOf<ASloopShip> SloopClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	TSubclassOf<ABrigShip> BrigClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	TSubclassOf<AGalleonShip> GalleonClass;

	// Spawn timer interval (seconds)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	float SpawnInterval = 45.0f;

	// Max number of active enemies at once
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	int32 MaxEnemies = 8;

	// Minimum spawn distance from player
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	float MinSpawnDistance = 3000.0f;

	// Maximum spawn distance from player
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	float MaxSpawnDistance = 6000.0f;

	// At wanted level 0-1: mostly sloops. 2-3: sloops + brigs. 4+: all types
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	float SloopSpawnWeight = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	float BrigSpawnWeight = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	float GalleonSpawnWeight = 0.15f;

private:
	float SpawnTimer = 0.0f;

	TArray<TWeakObjectPtr<AEnemyShipBase>> ActiveEnemies;

	void TrySpawnEnemy();
	int32 GetActiveEnemyCount() const;
	FVector FindSpawnLocation() const;
	bool IsLocationSafe(FVector Location) const;
	TSubclassOf<AEnemyShipBase> PickShipClassByWantedLevel(int32 WantedLevel) const;
	AWantedLevelManager* GetWantedLevelManager() const;
};
