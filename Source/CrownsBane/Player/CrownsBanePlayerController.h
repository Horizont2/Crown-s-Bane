#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CrownsBanePlayerController.generated.h"

class ADocksZone;
class AUpgradeManager;
class UPlayerInventory;
class AShipPawn;
class UInputMappingContext;

UCLASS()
class CROWNSBANE_API ACrownsBanePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ACrownsBanePlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	UPlayerInventory* PlayerInventory;

	UPROPERTY(BlueprintReadOnly, Category = "Docks")
	bool bIsInDocks = false;

	UPROPERTY(BlueprintReadOnly, Category = "Docks")
	ADocksZone* CurrentDocksZone = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Docks")
	void OnEnterDocks(ADocksZone* DocksZone);

	UFUNCTION(BlueprintCallable, Category = "Docks")
	void OnExitDocks();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void OpenUpgradeUI();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void CloseUpgradeUI();

	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsUpgradeUIOpen() const { return bUpgradeUIOpen; }

	UFUNCTION(BlueprintCallable, Category = "Upgrades")
	bool BuyUpgrade(uint8 CategoryByte);

private:
	void ToggleUpgradeUI();

	bool bUpgradeUIOpen = false;

	AUpgradeManager* GetUpgradeManager() const;
	AShipPawn* GetShipPawn() const;
};
