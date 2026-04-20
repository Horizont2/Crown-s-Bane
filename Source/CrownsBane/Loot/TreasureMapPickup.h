// Copyright 2024 Crown's Bane. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TreasureMapPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UNiagaraSystem;
class USoundBase;

/**
 * A floating scroll. When the player ship overlaps it, the scroll notifies
 * the TreasureQuestManager to issue a new treasure quest (spawning a chest
 * at a random sea location), then destroys itself with a reveal FX / sound.
 */
UCLASS()
class CROWNSBANE_API ATreasureMapPickup : public AActor
{
	GENERATED_BODY()

public:
	ATreasureMapPickup();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* PickupSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treasure Map")
	float PickupRadius = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treasure Map")
	float BobAmplitude = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treasure Map")
	float BobFrequency = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treasure Map")
	float DespawnTime = 120.0f;

	// FX
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treasure Map|FX")
	UNiagaraSystem* RevealFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treasure Map|FX")
	USoundBase* RevealSound = nullptr;

protected:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

private:
	float SpawnZ = 0.0f;
	float BobTimer = 0.0f;
	bool bCollected = false;
};
