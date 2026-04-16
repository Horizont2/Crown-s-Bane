#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Upgrades/UpgradeTypes.h"
#include "UpgradeManager.generated.h"

class UPlayerInventory;
class AShipPawn;
class UCannonComponent;
class UHealthComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUpgradePurchased, EUpgradeCategory, Category, int32, NewTier);

UCLASS()
class CROWNSBANE_API AUpgradeManager : public AActor
{
	GENERATED_BODY()

public:
	AUpgradeManager();

protected:
	virtual void BeginPlay() override;

public:
	// Attempt to purchase an upgrade in the given category
	UFUNCTION(BlueprintCallable, Category = "Upgrades")
	bool PurchaseUpgrade(EUpgradeCategory Category, UPlayerInventory* Inventory, AShipPawn* Ship);

	// Get the current tier for a category (0 = not purchased, max 4)
	UFUNCTION(BlueprintPure, Category = "Upgrades")
	int32 GetCurrentTier(EUpgradeCategory Category) const;

	// Get the max tier for a category
	UFUNCTION(BlueprintPure, Category = "Upgrades")
	int32 GetMaxTier(EUpgradeCategory Category) const { return MaxTier; }

	// Get the next upgrade data for a category (the one that would be purchased)
	UFUNCTION(BlueprintPure, Category = "Upgrades")
	bool GetNextUpgradeData(EUpgradeCategory Category, FUpgradeLevel& OutData) const;

	// Get cost of the next upgrade for a category
	UFUNCTION(BlueprintPure, Category = "Upgrades")
	bool CanAffordNextUpgrade(EUpgradeCategory Category, const UPlayerInventory* Inventory) const;

	UPROPERTY(BlueprintAssignable, Category = "Upgrades")
	FOnUpgradePurchased OnUpgradePurchased;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrades")
	int32 MaxTier = 4;

private:
	// Current tier per category
	TMap<EUpgradeCategory, int32> UpgradeTiers;

	// Build the upgrade tables
	void InitUpgradeTables();

	// Upgrade data tables per category, 4 tiers each
	TMap<EUpgradeCategory, TArray<FUpgradeLevel>> UpgradeTables;

	void ApplyUpgradeEffect(EUpgradeCategory Category, int32 Tier, AShipPawn* Ship);
};
