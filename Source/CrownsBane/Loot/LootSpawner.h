#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Loot/ResourceTypes.h"
#include "LootSpawner.generated.h"

class ALootPickup;

UENUM(BlueprintType)
enum class EEnemyShipType : uint8
{
	Sloop   UMETA(DisplayName = "Sloop"),
	Brig    UMETA(DisplayName = "Brig"),
	Galleon UMETA(DisplayName = "Galleon")
};

UCLASS()
class CROWNSBANE_API ALootSpawner : public AActor
{
	GENERATED_BODY()

public:
	ALootSpawner();

protected:
	virtual void BeginPlay() override;

public:
	// Call this to spawn loot for a specific ship type at a given location
	UFUNCTION(BlueprintCallable, Category = "Loot")
	void SpawnLootForShipType(EEnemyShipType ShipType, FVector Location);

	// Spawn a specific resource pickup
	UFUNCTION(BlueprintCallable, Category = "Loot")
	ALootPickup* SpawnPickup(FVector Location, EResourceType ResourceType, int32 Amount);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	TSubclassOf<ALootPickup> LootPickupClass;

	// Loot tables per ship type
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Sloop")
	int32 SloopGoldMin = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Sloop")
	int32 SloopGoldMax = 80;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Sloop")
	int32 SloopWoodMin = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Sloop")
	int32 SloopWoodMax = 15;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Brig")
	int32 BrigGoldMin = 80;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Brig")
	int32 BrigGoldMax = 200;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Brig")
	int32 BrigWoodMin = 15;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Brig")
	int32 BrigWoodMax = 35;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Brig")
	int32 BrigMetalMin = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Brig")
	int32 BrigMetalMax = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Galleon")
	int32 GalleonGoldMin = 250;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Galleon")
	int32 GalleonGoldMax = 600;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Galleon")
	int32 GalleonWoodMin = 40;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Galleon")
	int32 GalleonWoodMax = 80;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Galleon")
	int32 GalleonMetalMin = 25;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Galleon")
	int32 GalleonMetalMax = 60;

private:
	void SpawnSloopLoot(FVector Location);
	void SpawnBrigLoot(FVector Location);
	void SpawnGalleonLoot(FVector Location);
	FVector GetRandomLootOffset() const;
};
