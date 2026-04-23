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
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaSeconds) override;

	// In PIE the Slate-focus sometimes stays on the editor UI instead of
	// the game viewport, leaving the player unable to type W/A/S/D or move
	// the mouse. We re-claim focus aggressively for the first few seconds
	// and any time we detect we've lost it.
	void ForceFocusGameViewport();

	float FocusTimer = 0.0f;
	bool bForcedFocusOnce = false;

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
