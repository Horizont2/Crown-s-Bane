#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "CrownsBaneHUD.generated.h"

class UHealthComponent;
class UCannonComponent;
class UPlayerInventory;
class AWantedLevelManager;
class AWindSystem;
class AShipPawn;

UCLASS()
class CROWNSBANE_API ACrownsBaneHUD : public AHUD
{
	GENERATED_BODY()

public:
	ACrownsBaneHUD();

protected:
	virtual void BeginPlay() override;

public:
	virtual void DrawHUD() override;

	// Color settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Colors")
	FLinearColor HealthBarColor = FLinearColor(0.0f, 0.8f, 0.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Colors")
	FLinearColor HealthBarBGColor = FLinearColor(0.15f, 0.15f, 0.15f, 0.8f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Colors")
	FLinearColor ReloadReadyColor = FLinearColor(1.0f, 0.7f, 0.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Colors")
	FLinearColor ReloadEmptyColor = FLinearColor(0.3f, 0.3f, 0.3f, 0.8f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Colors")
	FLinearColor WantedStarColor = FLinearColor(1.0f, 0.9f, 0.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Colors")
	FLinearColor WantedStarEmptyColor = FLinearColor(0.2f, 0.2f, 0.2f, 0.6f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Colors")
	FLinearColor ResourceTextColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

	// HUD layout
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Layout")
	float HealthBarWidth = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Layout")
	float HealthBarHeight = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Layout")
	float HUDPaddingX = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Layout")
	float HUDPaddingY = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Layout")
	float ReloadBarWidth = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Layout")
	float ReloadBarHeight = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Layout")
	float StarSize = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Layout")
	float WindArrowRadius = 40.0f;

	// Show upgrade UI prompt when in docks
	UPROPERTY(BlueprintReadWrite, Category = "HUD")
	bool bShowDocksPrompt = false;

private:
	void DrawHealthBar(AShipPawn* Ship);
	void DrawReloadTimers(UCannonComponent* Cannons);
	void DrawWantedStars(AWantedLevelManager* WLM);
	void DrawResourceCounts(UPlayerInventory* Inventory);
	void DrawWindArrow(AWindSystem* Wind, AShipPawn* Ship);
	void DrawDocksPrompt();

	void DrawFilledRect(float X, float Y, float W, float H, FLinearColor Color);
	void DrawBorderedRect(float X, float Y, float W, float H, FLinearColor FillColor, FLinearColor BorderColor, float BorderThickness = 2.0f);
	void DrawStar(float CX, float CY, float Radius, FLinearColor Color);
	void DrawArrow(float CX, float CY, float Radius, float AngleDegrees, FLinearColor Color);

	// Cached references (refreshed each frame for safety)
	AShipPawn* GetPlayerShip() const;
	AWantedLevelManager* GetWantedLevelManager() const;
	AWindSystem* GetWindSystem() const;
	UPlayerInventory* GetPlayerInventory() const;
};
