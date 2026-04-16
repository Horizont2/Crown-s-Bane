#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Loot/ResourceTypes.h"
#include "LootPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UPlayerInventory;

UCLASS()
class CROWNSBANE_API ALootPickup : public AActor
{
	GENERATED_BODY()

public:
	ALootPickup();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* PickupSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	FResourceDrop ResourceDrop;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	float PickupRadius = 350.0f;

	// Bob up and down on water
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	float BobAmplitude = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	float BobFrequency = 1.5f;

	// Auto-despawn time
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	float DespawnTime = 60.0f;

	UFUNCTION(BlueprintCallable, Category = "Loot")
	void SetResourceDrop(EResourceType Type, int32 Amount);

protected:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

private:
	float SpawnZ = 0.0f;
	float BobTimer = 0.0f;
	bool bPickedUp = false;
};
