#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "CrownsBaneHUD.generated.h"

USTRUCT()
struct FFloatingDamageEntry
{
	GENERATED_BODY()

	FVector WorldLocation = FVector::ZeroVector;
	float   Damage        = 0.0f;
	float   TimeRemaining = 1.2f;
	float   VerticalSpeed = 120.0f; // cm/s
	FColor  Tint          = FColor::White;
};

class UHealthComponent;
class UCannonComponent;
class UPlayerInventory;
class AWantedLevelManager;
class AWindSystem;
class AStormSystem;
class ATreasureQuestManager;
class AEnemyShipBase;
class AShipPawn;
class ADayNightSystem;

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

	// ---- Storm indicator ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Colors")
	FLinearColor StormBarColor = FLinearColor(0.25f, 0.45f, 0.9f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Colors")
	FLinearColor StormBarBGColor = FLinearColor(0.1f, 0.1f, 0.12f, 0.8f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Layout")
	float StormBarWidth = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Layout")
	float StormBarHeight = 14.0f;

	// ---- Treasure quest compass ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Colors")
	FLinearColor TreasureArrowColor = FLinearColor(1.0f, 0.8f, 0.1f, 1.0f);

	// ---- Enemy health bars ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Colors")
	FLinearColor EnemyHPBarColor = FLinearColor(0.85f, 0.1f, 0.1f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Colors")
	FLinearColor EnemyHPBarBGColor = FLinearColor(0.12f, 0.05f, 0.05f, 0.85f);

	// ---- Aim predictor (AC4-style) ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Aim")
	FLinearColor AimArcColorReady = FLinearColor(1.0f, 0.85f, 0.1f, 0.95f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Aim")
	FLinearColor AimArcColorReload = FLinearColor(0.55f, 0.55f, 0.55f, 0.6f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Aim")
	FLinearColor AimImpactRingColor = FLinearColor(1.0f, 0.25f, 0.15f, 0.85f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Aim")
	float AimImpactRingRadius = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Aim")
	float SeaLevelZ = 0.0f;

	// ---- Crosshair ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Reticle")
	FLinearColor CrosshairColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.45f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Reticle")
	float CrosshairSize = 14.0f;

	// ---- Minimap ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Minimap")
	float MinimapRadius = 110.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Minimap")
	float MinimapWorldRadius = 10000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Minimap")
	float MinimapPadding = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Minimap")
	FLinearColor MinimapBGColor = FLinearColor(0.02f, 0.04f, 0.08f, 0.75f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Minimap")
	FLinearColor MinimapBorderColor = FLinearColor(0.35f, 0.55f, 0.8f, 0.9f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Minimap")
	FLinearColor MinimapViewConeColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.18f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Minimap")
	FLinearColor MinimapPlayerColor = FLinearColor(0.2f, 0.9f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Minimap")
	FLinearColor MinimapEnemyColor = FLinearColor(0.95f, 0.2f, 0.15f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Minimap")
	FLinearColor MinimapLootColor = FLinearColor(1.0f, 0.85f, 0.15f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Minimap")
	FLinearColor MinimapTreasureColor = FLinearColor(1.0f, 0.55f, 0.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Minimap")
	FLinearColor MinimapDockColor = FLinearColor(0.25f, 0.9f, 0.4f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Minimap")
	float MinimapViewConeAngle = 70.0f;

	// Maximum world distance to draw enemy health bars (cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Layout")
	float EnemyHPDrawRange = 8000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Layout")
	float EnemyHPBarWidth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Layout")
	float EnemyHPBarHeight = 8.0f;

	// Show upgrade UI prompt when in docks
	UPROPERTY(BlueprintReadWrite, Category = "HUD")
	bool bShowDocksPrompt = false;

	// Toggled by PlayerController via J key — full-panel quest log overlay.
	UPROPERTY(BlueprintReadWrite, Category = "HUD")
	bool bShowQuestLog = false;

	// Called by Cannonball::OnHit to queue up a floating damage number.
	UFUNCTION(BlueprintCallable, Category = "HUD|Damage")
	void AddFloatingDamage(FVector WorldLocation, float Damage, bool bHitShip);

	// Trigger red full-screen vignette flash when the player takes damage.
	// Intensity = 0..1 scales the alpha (e.g. damage% of max HP).
	UFUNCTION(BlueprintCallable, Category = "HUD|Damage")
	void TriggerDamageFlash(float Intensity = 1.0f);

	// Trigger a brief hit marker at screen center when the player damages an enemy.
	UFUNCTION(BlueprintCallable, Category = "HUD|Damage")
	void TriggerHitMarker(bool bCritical = false);

private:
	void DrawHealthBar(AShipPawn* Ship);
	void DrawReloadTimers(UCannonComponent* Cannons);
	void DrawWantedStars(AWantedLevelManager* WLM);
	void DrawResourceCounts(UPlayerInventory* Inventory);
	void DrawWindArrow(AWindSystem* Wind, AShipPawn* Ship);
	void DrawStormIndicator(AStormSystem* Storm);
	void DrawTreasureCompass(ATreasureQuestManager* Manager, AShipPawn* Ship);
	void DrawEnemyHealthBars(AShipPawn* PlayerShip);
	void DrawMinimap(AShipPawn* PlayerShip, ATreasureQuestManager* TreasureMgr);
	void DrawFiringArcs(AShipPawn* PlayerShip);
	void DrawAmmoCounter(UPlayerInventory* Inventory);
	void DrawDocksPrompt();
	void DrawAimPredictor(AShipPawn* PlayerShip);
	void DrawCrosshair();
	void DrawTimeOfDay();
	void DrawFloatingDamageNumbers(float DeltaTime);
	void DrawDamageFlash(float DeltaTime);
	void DrawHitMarker(float DeltaTime);
	void DrawBoardingPrompt(AShipPawn* Ship);
	void DrawTraderMenu(class ACrownsBanePlayerController* PC, class UPlayerInventory* Inv);
	void DrawLockOnReticle(AShipPawn* Ship);
	void DrawLeadIndicator(AShipPawn* Ship);
	void DrawQuestLog(ATreasureQuestManager* Mgr, AShipPawn* Ship);
	void DrawActiveQuestTracker(ATreasureQuestManager* Mgr, AShipPawn* Ship);
	void DrawUpgradeMenu(class ACrownsBanePlayerController* PC, class AUpgradeManager* Mgr, class UPlayerInventory* Inv);

	// ������� �� �� ������ private:
private:
	// ������ ���� ������'�
	float DisplayedHealthPct = -1.0f;

	// ��� ������� ��� ��������� �������
	void DrawTextWithShadow(const FString& Text, FColor TextColor, float X, float Y, UFont* Font = nullptr, float Scale = 1.0f);
	void DrawMinimalistBar(float X, float Y, float W, float H, float Pct, FLinearColor FillColor);

	// Floating damage queue
	TArray<FFloatingDamageEntry> FloatingDamageEntries;
	double LastDrawTime = 0.0;

	// Damage flash + hit marker timers (seconds remaining)
	float DamageFlashTimer = 0.0f;
	float DamageFlashIntensity = 0.0f;
	static constexpr float DamageFlashDuration = 0.55f;

	float HitMarkerTimer = 0.0f;
	bool  bHitMarkerCritical = false;
	static constexpr float HitMarkerDuration = 0.32f;

	// ---- UI-B polish: resource toasts ----
	int32 LastGold = -1, LastWood = -1, LastMetal = -1;

	struct FResourceToast { FString Text; FLinearColor Tint; float TimeRemaining = 0.0f; };
	TArray<FResourceToast> ResourceToasts;

public:
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void PushResourceToast(const FString& Text, FLinearColor Tint);

private:
	void DrawResourceToasts(float DeltaTime);
	void DrawLowHealthFlash(AShipPawn* Ship);

	// ---- UI-D Combat HUD ----
	void DrawEnemyInfoCard(AShipPawn* Ship);
	void DrawDamageDirection(float DeltaTime);
	void DrawComboCounter(float DeltaTime);
	void DrawKillFeed(float DeltaTime);

	// Damage-direction state — Cannonball/ShipPawn poke this on player taking damage.
	struct FDamageMarker { float WorldYaw = 0.0f; float TimeRemaining = 0.0f; };
	TArray<FDamageMarker> DamageMarkers;

	// Combo / hit chain
	int32 CurrentCombo = 0;
	float ComboTimeRemaining = 0.0f;

	// Kill feed entries
	struct FKillFeedEntry { FString Text; float TimeRemaining = 0.0f; };
	TArray<FKillFeedEntry> KillFeed;

public:
	UFUNCTION(BlueprintCallable, Category = "HUD|Combat")
	void RegisterPlayerDamageFrom(float WorldYawDegrees);

	UFUNCTION(BlueprintCallable, Category = "HUD|Combat")
	void RegisterPlayerHit();

	UFUNCTION(BlueprintCallable, Category = "HUD|Combat")
	void PushKillFeed(const FString& Text);

	// ---- UI-G Cinematic banners ----
	UFUNCTION(BlueprintCallable, Category = "HUD|Cinematic")
	void ShowBanner(const FString& Title, const FString& Subtitle, FLinearColor Tint, float Duration = 3.5f);

	UFUNCTION(BlueprintCallable, Category = "HUD|Cinematic")
	void ShowMissionComplete(const FString& QuestName, int32 Gold, int32 Wood, int32 Metal);

private:
	FString BannerTitle, BannerSubtitle;
	FLinearColor BannerTint = FLinearColor::White;
	float BannerTimeRemaining = 0.0f;
	float BannerInitialDuration = 1.0f;

	FString MissionTitle;
	int32 MissionGold = 0, MissionWood = 0, MissionMetal = 0;
	float  MissionTimeRemaining = 0.0f;

	void DrawBanner(float DeltaTime);
	void DrawMissionComplete(float DeltaTime);
	void DrawPauseMenu(class ACrownsBanePlayerController* PC);

	// Helpers for minimap
	void DrawMinimapDot(float CX, float CY, float DotX, float DotY, float DotSize, FLinearColor Color);
	void DrawMinimapTriangle(float CX, float CY, float AngleDeg, float Size, FLinearColor Color);
	void DrawMinimapViewCone(float CX, float CY, float AngleDeg, float Radius, float FOVDeg, FLinearColor Color);

	void DrawFilledRect(float X, float Y, float W, float H, FLinearColor Color);
	void DrawBorderedRect(float X, float Y, float W, float H, FLinearColor FillColor, FLinearColor BorderColor, float BorderThickness = 2.0f);
	void DrawStar(float CX, float CY, float Radius, FLinearColor Color);
	void DrawArrow(float CX, float CY, float Radius, float AngleDegrees, FLinearColor Color);

	// ---- Unified style helpers (UICrownStyle) ----
	// Draws a CrownStyle::EPanelStyle-themed panel at (X,Y) with size (W,H).
	void DrawPanel(float X, float Y, float W, float H, uint8 Style /*CrownStyle::EPanelStyle*/);
	// Body text using CrownStyle::TextPrimary, font scale ScaleBody.
	void DrawBody(const FString& Text, float X, float Y, float Scale = 1.0f);
	void DrawHeading(const FString& Text, float X, float Y);
	void DrawCaption(const FString& Text, float X, float Y);
	// Smooth bar with optional pulse-highlight when bPulse is true.
	void DrawSmoothBar(float X, float Y, float W, float H, float Frac, FLinearColor Fill, bool bPulse = false);

	// Cached references (refreshed each frame for safety)
	AShipPawn* GetPlayerShip() const;
	AWantedLevelManager* GetWantedLevelManager() const;
	AWindSystem* GetWindSystem() const;
	AStormSystem* GetStormSystem() const;
	ATreasureQuestManager* GetTreasureQuestManager() const;
	UPlayerInventory* GetPlayerInventory() const;
};
