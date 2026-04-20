// Copyright 2024 Crown's Bane. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TreasureChest.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UNiagaraSystem;
class USoundBase;
class UPlayerInventory;

/**
 * Buoyant chest spawned at a treasure quest location. When the player ship
 * overlaps, it rewards all three resources, plays open FX, and notifies the
 * TreasureQuestManager before destroying itself.
 */
UCLASS()
class CROWNSBANE_API ATreasureChest : public AActor
{
	GENERATED_BODY()

public:
	ATreasureChest();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* PickupSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeshComponent;

	// ---- Reward ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treasure|Reward")
	int32 GoldReward = 250;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treasure|Reward")
	int32 WoodReward = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treasure|Reward")
	int32 MetalReward = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treasure")
	float PickupRadius = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treasure")
	float BobAmplitude = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treasure")
	float BobFrequency = 0.6f;

	// ---- FX ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treasure|FX")
	UNiagaraSystem* BeaconFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treasure|FX")
	UNiagaraSystem* OpenFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treasure|FX")
	USoundBase* OpenSound = nullptr;

	// Quest this chest belongs to (set by TreasureQuestManager)
	UPROPERTY(BlueprintReadOnly, Category = "Treasure")
	FGuid QuestId;

protected:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	void GrantRewards(UPlayerInventory* Inventory);

private:
	float SpawnZ = 0.0f;
	float BobTimer = 0.0f;
	bool bLooted = false;
};
