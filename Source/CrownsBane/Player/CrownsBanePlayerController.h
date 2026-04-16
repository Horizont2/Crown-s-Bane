#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CrownsBanePlayerController.generated.h"

class ADocksZone;
class AUpgradeManager;
class UPlayerInventory;
class AShipPawn;

UCLASS()
class CROWNSBANE_API ACrownsBanePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ACrownsBanePlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void Tick(float DeltaTime) override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	UPlayerInventory* PlayerInventory;

	UPROPERTY(BlueprintReadOnly, Category = "Docks")
	bool bIsInDocks = false;

	UPROPERTY(BlueprintReadOnly, Category = "Docks")
	ADocksZone* CurrentDocksZone = nullptr;

	// Called by DocksZone when player enters/exits
	UFUNCTION(BlueprintCallable, Category = "Docks")
	void OnEnterDocks(ADocksZone* DocksZone);

	UFUNCTION(BlueprintCallable, Category = "Docks")
	void OnExitDocks();

	// Show/hide the upgrade UI
	UFUNCTION(BlueprintCallable, Category = "UI")
	void OpenUpgradeUI();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void CloseUpgradeUI();

	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsUpgradeUIOpen() const { return bUpgradeUIOpen; }

	// Upgrade: buy a specific category (called from UI)
	UFUNCTION(BlueprintCallable, Category = "Upgrades")
	bool BuyUpgrade(uint8 CategoryByte);

private:
	void InputOpenUpgradeUI();
	void InputCloseUpgradeUI();

	bool bUpgradeUIOpen = false;

	AUpgradeManager* GetUpgradeManager() const;
	AShipPawn* GetShipPawn() const;
};
