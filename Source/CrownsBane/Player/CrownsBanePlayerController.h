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

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ToggleUpgradeUI();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ToggleQuestLog();

	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsQuestLogOpen() const { return bQuestLogOpen; }

	UFUNCTION(BlueprintCallable, Category = "UI")
	void TogglePauseMenu();

	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsPauseMenuOpen() const { return bPauseMenuOpen; }

	UFUNCTION(BlueprintCallable, Category = "Upgrades")
	bool BuyUpgrade(uint8 CategoryByte);

	// ---- Trader (docks-only) ----
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ToggleTraderMenu();

	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsTraderMenuOpen() const { return bTraderMenuOpen; }

	UFUNCTION(BlueprintCallable, Category = "Trade")
	bool BuyAmmo(int32 Amount = 10);

	UFUNCTION(BlueprintCallable, Category = "Trade")
	bool BuyWood(int32 Amount = 10);

	UFUNCTION(BlueprintCallable, Category = "Trade")
	bool BuyMetal(int32 Amount = 5);

	UFUNCTION(BlueprintCallable, Category = "Trade")
	bool SellWood(int32 Amount = 10);

	UFUNCTION(BlueprintCallable, Category = "Trade")
	bool SellMetal(int32 Amount = 5);

	UFUNCTION(BlueprintCallable, Category = "Trade")
	bool PayForHeal();

	// Trader prices (gold per N units)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Trade")
	int32 BuyAmmoPrice  = 30;   // per 10 rounds
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Trade")
	int32 BuyWoodPrice  = 25;   // per 10 wood
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Trade")
	int32 BuyMetalPrice = 40;   // per 5 metal
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Trade")
	int32 SellWoodPrice = 12;   // gold gained per 10 wood
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Trade")
	int32 SellMetalPrice = 25;  // gold per 5 metal
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Trade")
	int32 HealCost = 80;        // gold to fully heal

	// ---- Lifetime stats ----
	UPROPERTY(BlueprintReadOnly, Category = "Stats")  int32 StatShipsSunk = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Stats")  int32 StatBoardingsWon = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Stats")  int32 StatCannonballsFired = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Stats")  int32 StatDamageDealt = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Stats")  int32 StatDamageTaken = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Stats")  int32 StatGoldEarned = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Stats")  float StatPlayTimeSeconds = 0.0f;

	UFUNCTION(BlueprintCallable, Category = "Stats")  void StatBumpShipsSunk();
	UFUNCTION(BlueprintCallable, Category = "Stats")  void StatBumpBoardingsWon();
	UFUNCTION(BlueprintCallable, Category = "Stats")  void StatBumpCannonballsFired(int32 N = 1);
	UFUNCTION(BlueprintCallable, Category = "Stats")  void StatBumpDamageDealt(float Dmg);
	UFUNCTION(BlueprintCallable, Category = "Stats")  void StatBumpDamageTaken(float Dmg);
	UFUNCTION(BlueprintCallable, Category = "Stats")  void StatBumpGoldEarned(int32 Amt);

private:

	bool bUpgradeUIOpen = false;
	bool bQuestLogOpen = false;
	bool bTraderMenuOpen = false;
	bool bPauseMenuOpen = false;

	AUpgradeManager* GetUpgradeManager() const;
	AShipPawn* GetShipPawn() const;
};
