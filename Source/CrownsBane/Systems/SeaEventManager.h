// Copyright 2024 Crown's Bane. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SeaEventManager.generated.h"

class ALootPickup;
class AEnemyShipBase;

UENUM(BlueprintType)
enum class ESeaEventType : uint8
{
	FloatingWreckage UMETA(DisplayName = "Floating Wreckage"),
	Fisherman        UMETA(DisplayName = "Lone Fisherman"),
	TradeConvoy      UMETA(DisplayName = "Trade Convoy"),
	BottleMessage    UMETA(DisplayName = "Bottle Message")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSeaEventSpawned, ESeaEventType, Type, FVector, Location);

/**
 * Periodically spawns random sea events around the player to keep the
 * world feeling alive between combat.  Drop a SeaEventManager actor in
 * the level and (optionally) set the spawn class TSubclassOf pointers in
 * the Details panel.  Triggers a toast on the HUD when an event spawns.
 */
UCLASS()
class CROWNSBANE_API ASeaEventManager : public AActor
{
	GENERATED_BODY()

public:
	ASeaEventManager();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// Interval range between random events (seconds).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sea Events")
	float MinInterval = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sea Events")
	float MaxInterval = 120.0f;

	// Distance range from player at which to spawn.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sea Events")
	float MinSpawnDistance = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sea Events")
	float MaxSpawnDistance = 10000.0f;

	// Sea-level Z for spawned actors.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sea Events")
	float SeaLevelZ = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sea Events|Classes")
	TSubclassOf<ALootPickup> WreckageLootClass;

	// Enemy ship classes the events can spawn as fishermen / convoy escorts /
	// the convoy flagship.  All optional — null entries are skipped.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sea Events|Classes")
	TSubclassOf<AEnemyShipBase> FishermanShipClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sea Events|Classes")
	TSubclassOf<AEnemyShipBase> ConvoyEscortClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sea Events|Classes")
	TSubclassOf<AEnemyShipBase> ConvoyFlagshipClass;

	UPROPERTY(BlueprintAssignable, Category = "Sea Events")
	FOnSeaEventSpawned OnSeaEventSpawned;

	// Force-spawn an event (for testing).
	UFUNCTION(BlueprintCallable, Category = "Sea Events")
	void ForceSpawn(ESeaEventType Type);

private:
	float TimeUntilNext = 30.0f;
	void SpawnRandomEvent();
	FVector PickSpawnLocation() const;
	void SpawnEvent_Wreckage(const FVector& Loc);
	void SpawnEvent_Fisherman(const FVector& Loc);
	void SpawnEvent_TradeConvoy(const FVector& Loc);
	void SpawnEvent_BottleMessage(const FVector& Loc);

	void Toast(ESeaEventType Type, const FVector& Loc) const;
};
