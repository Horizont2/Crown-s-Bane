// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "UI/CrownsBaneHUD.h"
#include "Ship/ShipPawn.h"
#include "Combat/CannonComponent.h"
#include "Components/HealthComponent.h"
#include "Player/PlayerInventory.h"
#include "Systems/WantedLevelManager.h"
#include "Systems/WindSystem.h"
#include "Systems/StormSystem.h"
#include "Loot/TreasureQuestManager.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

ACrownsBaneHUD::ACrownsBaneHUD()
{
}

void ACrownsBaneHUD::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Log, TEXT("CrownsBaneHUD: Initialized."));
}

void ACrownsBaneHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas) return;

	AShipPawn* Ship = GetPlayerShip();
	AWantedLevelManager* WLM = GetWantedLevelManager();
	AWindSystem* Wind = GetWindSystem();
	AStormSystem* Storm = GetStormSystem();
	ATreasureQuestManager* TreasureMgr = GetTreasureQuestManager();
	UPlayerInventory* Inventory = GetPlayerInventory();

	if (Ship)
	{
		DrawHealthBar(Ship);

		if (Ship->CannonComponent)
		{
			DrawReloadTimers(Ship->CannonComponent);
		}
	}

	if (WLM)
	{
		DrawWantedStars(WLM);
	}

	if (Inventory)
	{
		DrawResourceCounts(Inventory);
	}

	if (Wind && Ship)
	{
		DrawWindArrow(Wind, Ship);
	}

	if (Storm)
	{
		DrawStormIndicator(Storm);
	}

	if (TreasureMgr && Ship)
	{
		DrawTreasureCompass(TreasureMgr, Ship);
	}

	if (bShowDocksPrompt)
	{
		DrawDocksPrompt();
	}
}

void ACrownsBaneHUD::DrawHealthBar(AShipPawn* Ship)
{
	if (!Ship || !Ship->HealthComponent) return;

	float HealthPct = Ship->HealthComponent->GetHealthPercent();
	float ScreenW = Canvas->ClipX;
	float ScreenH = Canvas->ClipY;

	float BarX = HUDPaddingX;
	float BarY = ScreenH - HUDPaddingY - HealthBarHeight - 30.0f;

	// Background
	DrawFilledRect(BarX - 2, BarY - 2, HealthBarWidth + 4, HealthBarHeight + 4, HealthBarBGColor);

	// Health fill - color transitions from green to red as health drops
	FLinearColor FillColor = FLinearColor::LerpUsingHSV(
		FLinearColor(0.8f, 0.1f, 0.1f, 1.0f),
		HealthBarColor,
		HealthPct);
	DrawFilledRect(BarX, BarY, HealthBarWidth * HealthPct, HealthBarHeight, FillColor);

	// Health text
	FString HealthText = FString::Printf(TEXT("Hull: %.0f / %.0f"),
		Ship->HealthComponent->GetCurrentHealth(),
		Ship->HealthComponent->GetMaxHealth());
	DrawText(HealthText, FColor::White, BarX, BarY - 22.0f, nullptr, 1.0f, false);

	// Speed indicator
	FString SpeedText = FString::Printf(TEXT("Speed: %.0f  Sail: %s"),
		Ship->GetCurrentSpeed(),
		Ship->GetSailLevel() == ESailLevel::Stop ? TEXT("STOP") :
		Ship->GetSailLevel() == ESailLevel::HalfSail ? TEXT("HALF") : TEXT("FULL"));
	DrawText(SpeedText, FColor::Cyan, BarX, BarY + HealthBarHeight + 4.0f, nullptr, 0.9f, false);
}

void ACrownsBaneHUD::DrawReloadTimers(UCannonComponent* Cannons)
{
	if (!Cannons) return;

	float ScreenW = Canvas->ClipX;
	float ScreenH = Canvas->ClipY;

	float CenterX = ScreenW * 0.5f;
	float BarY = ScreenH - HUDPaddingY - ReloadBarHeight;

	// Left cannon reload (Port)
	float LeftProgress = Cannons->GetReloadProgress(ECannonSide::Left);
	float LeftBarX = CenterX - ReloadBarWidth - 20.0f;

	DrawFilledRect(LeftBarX - 2, BarY - 2, ReloadBarWidth + 4, ReloadBarHeight + 4, FLinearColor(0.1f, 0.1f, 0.1f, 0.8f));
	FLinearColor LeftColor = (LeftProgress >= 1.0f) ? ReloadReadyColor : FLinearColor::LerpUsingHSV(ReloadEmptyColor, ReloadReadyColor, LeftProgress);
	DrawFilledRect(LeftBarX, BarY, ReloadBarWidth * LeftProgress, ReloadBarHeight, LeftColor);

	FString LeftLabel = Cannons->CanFire(ECannonSide::Left) ? TEXT("PORT [LMB] READY") : TEXT("PORT [LMB] RELOADING");
	DrawText(LeftLabel, LeftProgress >= 1.0f ? FColor::Yellow : FColor::Silver,
		LeftBarX, BarY - 20.0f, nullptr, 0.85f, false);

	// Right cannon reload (Starboard)
	float RightBarX = CenterX + 20.0f;
	float RightProgress = Cannons->GetReloadProgress(ECannonSide::Right);

	DrawFilledRect(RightBarX - 2, BarY - 2, ReloadBarWidth + 4, ReloadBarHeight + 4, FLinearColor(0.1f, 0.1f, 0.1f, 0.8f));
	FLinearColor RightColor = (RightProgress >= 1.0f) ? ReloadReadyColor : FLinearColor::LerpUsingHSV(ReloadEmptyColor, ReloadReadyColor, RightProgress);
	DrawFilledRect(RightBarX, BarY, ReloadBarWidth * RightProgress, ReloadBarHeight, RightColor);

	FString RightLabel = Cannons->CanFire(ECannonSide::Right) ? TEXT("STBD [RMB] READY") : TEXT("STBD [RMB] RELOADING");
	DrawText(RightLabel, RightProgress >= 1.0f ? FColor::Yellow : FColor::Silver,
		RightBarX, BarY - 20.0f, nullptr, 0.85f, false);

	// Cannon count info
	FString CannonInfo = FString::Printf(TEXT("%d cannons/side"), Cannons->GetCannonsPerSide());
	DrawText(CannonInfo, FColor::White, CenterX - 50.0f, BarY + ReloadBarHeight + 4.0f, nullptr, 0.8f, false);
}

void ACrownsBaneHUD::DrawWantedStars(AWantedLevelManager* WLM)
{
	if (!WLM) return;

	float ScreenW = Canvas->ClipX;
	int32 WantedLevel = WLM->GetWantedLevel();
	int32 MaxLevel = WLM->GetWantedLevel(); // Use current as reference for max display

	// Draw 5 stars at top center
	float TotalWidth = 5.0f * (StarSize + 8.0f);
	float StartX = (ScreenW - TotalWidth) * 0.5f;
	float StarY = HUDPaddingY + StarSize * 0.5f;

	for (int32 i = 0; i < 5; ++i)
	{
		float StarX = StartX + i * (StarSize + 8.0f) + StarSize * 0.5f;
		FLinearColor StarColor = (i < WLM->GetWantedLevel()) ? WantedStarColor : WantedStarEmptyColor;
		DrawStar(StarX, StarY, StarSize * 0.5f, StarColor);
	}

	// Wanted level label
	FString WantedText = (WantedLevel > 0)
		? FString::Printf(TEXT("WANTED - LEVEL %d"), WantedLevel)
		: TEXT("WANTED - NONE");
	FColor TextColor = (WantedLevel >= 4) ? FColor::Red : (WantedLevel >= 2 ? FColor::Orange : FColor::White);
	DrawText(WantedText, TextColor, StartX, StarY + StarSize + 4.0f, nullptr, 0.9f, false);
}

void ACrownsBaneHUD::DrawResourceCounts(UPlayerInventory* Inventory)
{
	if (!Inventory) return;

	float ScreenW = Canvas->ClipX;
	float RightX = ScreenW - HUDPaddingX - 200.0f;
	float TopY = HUDPaddingY;

	FString GoldText  = FString::Printf(TEXT("Gold:  %d"), Inventory->GetGold());
	FString WoodText  = FString::Printf(TEXT("Wood:  %d"), Inventory->GetWood());
	FString MetalText = FString::Printf(TEXT("Metal: %d"), Inventory->GetMetal());

	DrawText(GoldText,  FColor(255, 215, 0),   RightX, TopY,         nullptr, 1.0f, false);
	DrawText(WoodText,  FColor(160, 120, 60),   RightX, TopY + 24.0f, nullptr, 1.0f, false);
	DrawText(MetalText, FColor(180, 180, 200),  RightX, TopY + 48.0f, nullptr, 1.0f, false);
}

void ACrownsBaneHUD::DrawWindArrow(AWindSystem* Wind, AShipPawn* Ship)
{
	if (!Wind || !Ship) return;

	float ScreenW = Canvas->ClipX;
	float ScreenH = Canvas->ClipY;

	// Draw wind compass in bottom right
	float CX = ScreenW - HUDPaddingX - WindArrowRadius - 20.0f;
	float CY = ScreenH - HUDPaddingY - WindArrowRadius - 50.0f;

	// Background circle
	DrawFilledRect(CX - WindArrowRadius - 4, CY - WindArrowRadius - 4,
		(WindArrowRadius + 4) * 2, (WindArrowRadius + 4) * 2,
		FLinearColor(0.0f, 0.0f, 0.0f, 0.5f));

	FVector WindDir = Wind->GetWindDirection();
	FVector ShipForward = Ship->GetActorForwardVector();

	// Wind angle relative to screen (project to 2D)
	float WindAngle = FMath::Atan2(WindDir.Y, WindDir.X);
	float WindAngleDeg = FMath::RadiansToDegrees(WindAngle);

	// Draw wind direction arrow (where wind is blowing TO)
	DrawArrow(CX, CY, WindArrowRadius, WindAngleDeg, FLinearColor(0.4f, 0.7f, 1.0f, 1.0f));

	// Draw ship direction arrow (small, white)
	float ShipAngle = FMath::Atan2(ShipForward.Y, ShipForward.X);
	float ShipAngleDeg = FMath::RadiansToDegrees(ShipAngle);
	DrawArrow(CX, CY, WindArrowRadius * 0.5f, ShipAngleDeg, FLinearColor(1.0f, 1.0f, 1.0f, 0.7f));

	// Wind label
	float Multiplier = Wind->GetWindSpeedMultiplier(ShipForward);
	FString WindLabel;
	FColor WindColor;
	if (Multiplier > 0.1f)
	{
		WindLabel = FString::Printf(TEXT("TAILWIND +%.0f%%"), Multiplier * 20.0f);
		WindColor = FColor::Green;
	}
	else if (Multiplier < -0.1f)
	{
		WindLabel = FString::Printf(TEXT("HEADWIND -%.0f%%"), FMath::Abs(Multiplier) * 20.0f);
		WindColor = FColor::Orange;
	}
	else
	{
		WindLabel = TEXT("CROSSWIND");
		WindColor = FColor::White;
	}

	DrawText(WindLabel, WindColor, CX - WindArrowRadius, CY + WindArrowRadius + 8.0f, nullptr, 0.8f, false);
	DrawText(TEXT("WIND"), FColor::White, CX - 16.0f, CY - WindArrowRadius - 18.0f, nullptr, 0.8f, false);
}

void ACrownsBaneHUD::DrawStormIndicator(AStormSystem* Storm)
{
	if (!Storm) return;

	const float Intensity = Storm->GetStormIntensity();
	const EStormPhase Phase = Storm->GetStormPhase();

	// Hide during Clear weather with near-zero intensity
	if (Phase == EStormPhase::Clear && Intensity < 0.02f) return;

	const float ScreenW = Canvas->ClipX;
	const float BarX = ScreenW * 0.5f - StormBarWidth * 0.5f;
	const float BarY = HUDPaddingY + StarSize + 34.0f; // Below wanted stars row

	// Background
	DrawFilledRect(BarX - 2, BarY - 2, StormBarWidth + 4, StormBarHeight + 4, StormBarBGColor);

	// Intensity fill
	FLinearColor Fill = FLinearColor::LerpUsingHSV(StormBarColor, FLinearColor(0.9f, 0.2f, 0.1f, 1.f), Intensity);
	DrawFilledRect(BarX, BarY, StormBarWidth * Intensity, StormBarHeight, Fill);

	// Label
	FString PhaseLabel;
	FColor LabelColor = FColor::Cyan;
	switch (Phase)
	{
	case EStormPhase::Clear:       PhaseLabel = TEXT("CLEAR SKIES");          LabelColor = FColor(160, 200, 255); break;
	case EStormPhase::BuildingUp:  PhaseLabel = TEXT("STORM INCOMING!");      LabelColor = FColor::Yellow;        break;
	case EStormPhase::Storm:       PhaseLabel = TEXT("STORM - HANG ON!");     LabelColor = FColor::Red;           break;
	case EStormPhase::Dissipating: PhaseLabel = TEXT("STORM DISSIPATING");    LabelColor = FColor::Orange;        break;
	}
	DrawText(PhaseLabel, LabelColor, BarX, BarY - 18.0f, nullptr, 0.85f, false);
}

void ACrownsBaneHUD::DrawTreasureCompass(ATreasureQuestManager* Manager, AShipPawn* Ship)
{
	if (!Manager || !Ship) return;

	const TArray<FTreasureQuest>& Quests = Manager->GetActiveQuests();
	if (Quests.Num() == 0) return;

	FVector NearestLoc;
	float NearestDist = 0.0f;
	const FVector ShipLoc = Ship->GetActorLocation();
	if (!Manager->GetNearestActiveQuestLocation(ShipLoc, NearestLoc, NearestDist))
	{
		return;
	}

	const float ScreenW = Canvas->ClipX;
	const float ScreenH = Canvas->ClipY;
	const float CX = ScreenW * 0.5f;
	const float CY = ScreenH * 0.18f;
	const float Radius = 38.0f;

	// Compass background
	DrawFilledRect(CX - Radius - 4, CY - Radius - 4,
		(Radius + 4) * 2, (Radius + 4) * 2,
		FLinearColor(0.0f, 0.0f, 0.0f, 0.55f));

	// Direction from ship to chest, relative to world, then normalized to 2D
	FVector ToChest = NearestLoc - ShipLoc;
	ToChest.Z = 0.0f;
	ToChest.Normalize();
	const float ChestWorldAngle = FMath::RadiansToDegrees(FMath::Atan2(ToChest.Y, ToChest.X));

	// Rotate relative to ship heading so arrow points forward when aligned
	const FVector ShipFwd = Ship->GetActorForwardVector();
	const float ShipAngle = FMath::RadiansToDegrees(FMath::Atan2(ShipFwd.Y, ShipFwd.X));
	const float RelAngle = ChestWorldAngle - ShipAngle - 90.0f; // -90 so "up" = forward

	DrawArrow(CX, CY, Radius, RelAngle, TreasureArrowColor);

	const FString DistLabel = FString::Printf(TEXT("TREASURE %.0f m"), NearestDist / 100.0f);
	DrawText(DistLabel, FColor(255, 210, 60), CX - 60.0f, CY + Radius + 6.0f, nullptr, 0.9f, false);

	const FString CountLabel = (Quests.Num() > 1)
		? FString::Printf(TEXT("+%d more maps"), Quests.Num() - 1)
		: TEXT("Active Quest");
	DrawText(CountLabel, FColor::White, CX - 50.0f, CY - Radius - 18.0f, nullptr, 0.75f, false);
}

void ACrownsBaneHUD::DrawDocksPrompt()
{
	float ScreenW = Canvas->ClipX;
	float ScreenH = Canvas->ClipY;

	FString PromptLine1 = TEXT("=== DOCKS ===");
	FString PromptLine2 = TEXT("Ship repaired! Press [U] to upgrade.");

	float TextX = ScreenW * 0.5f - 150.0f;
	float TextY = ScreenH * 0.3f;

	DrawFilledRect(TextX - 10, TextY - 10, 320.0f, 60.0f, FLinearColor(0.0f, 0.0f, 0.0f, 0.7f));
	DrawText(PromptLine1, FColor::Yellow, TextX, TextY, nullptr, 1.2f, false);
	DrawText(PromptLine2, FColor::White, TextX, TextY + 28.0f, nullptr, 0.9f, false);
}

// ---- Helper draw primitives ----

void ACrownsBaneHUD::DrawFilledRect(float X, float Y, float W, float H, FLinearColor Color)
{
	if (!Canvas) return;
	FCanvasTileItem TileItem(FVector2D(X, Y), GWhiteTexture, FVector2D(W, H), Color);
	TileItem.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(TileItem);
}

void ACrownsBaneHUD::DrawBorderedRect(float X, float Y, float W, float H,
	FLinearColor FillColor, FLinearColor BorderColor, float BorderThickness)
{
	// Border (slightly larger)
	DrawFilledRect(X - BorderThickness, Y - BorderThickness,
		W + BorderThickness * 2, H + BorderThickness * 2, BorderColor);
	// Fill
	DrawFilledRect(X, Y, W, H, FillColor);
}

void ACrownsBaneHUD::DrawStar(float CX, float CY, float Radius, FLinearColor Color)
{
	// Approximate star using two overlapping triangles (5-pointed star approximation via diamond)
	// Draw a filled diamond/rhombus as a rough star shape using canvas lines
	// Since we don't have a star primitive, draw as a filled square rotated 45 degrees
	// using 4 triangles. For simplicity, we draw a slightly smaller filled quad + outline.

	float Half = Radius * 0.7f;
	DrawFilledRect(CX - Half, CY - Half, Half * 2.0f, Half * 2.0f, Color);

	// Overlay rotated square
	float SmallHalf = Radius * 0.5f;
	// Draw as filled lines to approximate star points
	FCanvasLineItem TopLine(FVector2D(CX, CY - Radius), FVector2D(CX - SmallHalf, CY));
	TopLine.SetColor(Color);
	Canvas->DrawItem(TopLine);

	FCanvasLineItem RightLine(FVector2D(CX, CY - Radius), FVector2D(CX + SmallHalf, CY));
	RightLine.SetColor(Color);
	Canvas->DrawItem(RightLine);

	FCanvasLineItem BottomLine(FVector2D(CX, CY + Radius * 0.5f), FVector2D(CX - SmallHalf, CY));
	BottomLine.SetColor(Color);
	Canvas->DrawItem(BottomLine);

	FCanvasLineItem BottomRight(FVector2D(CX, CY + Radius * 0.5f), FVector2D(CX + SmallHalf, CY));
	BottomRight.SetColor(Color);
	Canvas->DrawItem(BottomRight);
}

void ACrownsBaneHUD::DrawArrow(float CX, float CY, float Radius, float AngleDegrees, FLinearColor Color)
{
	float Rad = FMath::DegreesToRadians(AngleDegrees);
	float TipX = CX + FMath::Cos(Rad) * Radius;
	float TipY = CY + FMath::Sin(Rad) * Radius;
	float TailX = CX - FMath::Cos(Rad) * Radius * 0.5f;
	float TailY = CY - FMath::Sin(Rad) * Radius * 0.5f;

	FCanvasLineItem LineItem(FVector2D(TailX, TailY), FVector2D(TipX, TipY));
	LineItem.SetColor(Color);
	LineItem.LineThickness = 3.0f;
	Canvas->DrawItem(LineItem);

	// Arrowhead
	float HeadRad = Rad + PI * 0.85f;
	float HeadRad2 = Rad - PI * 0.85f;
	float HeadLen = Radius * 0.35f;

	FCanvasLineItem HeadLeft(FVector2D(TipX, TipY),
		FVector2D(TipX + FMath::Cos(HeadRad) * HeadLen, TipY + FMath::Sin(HeadRad) * HeadLen));
	HeadLeft.SetColor(Color);
	HeadLeft.LineThickness = 3.0f;
	Canvas->DrawItem(HeadLeft);

	FCanvasLineItem HeadRight(FVector2D(TipX, TipY),
		FVector2D(TipX + FMath::Cos(HeadRad2) * HeadLen, TipY + FMath::Sin(HeadRad2) * HeadLen));
	HeadRight.SetColor(Color);
	HeadRight.LineThickness = 3.0f;
	Canvas->DrawItem(HeadRight);
}

// ---- Cached getters ----

AShipPawn* ACrownsBaneHUD::GetPlayerShip() const
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	return PC ? Cast<AShipPawn>(PC->GetPawn()) : nullptr;
}

AWantedLevelManager* ACrownsBaneHUD::GetWantedLevelManager() const
{
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AWantedLevelManager::StaticClass(), Actors);
	return (Actors.Num() > 0) ? Cast<AWantedLevelManager>(Actors[0]) : nullptr;
}

AWindSystem* ACrownsBaneHUD::GetWindSystem() const
{
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AWindSystem::StaticClass(), Actors);
	return (Actors.Num() > 0) ? Cast<AWindSystem>(Actors[0]) : nullptr;
}

AStormSystem* ACrownsBaneHUD::GetStormSystem() const
{
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AStormSystem::StaticClass(), Actors);
	return (Actors.Num() > 0) ? Cast<AStormSystem>(Actors[0]) : nullptr;
}

ATreasureQuestManager* ACrownsBaneHUD::GetTreasureQuestManager() const
{
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATreasureQuestManager::StaticClass(), Actors);
	return (Actors.Num() > 0) ? Cast<ATreasureQuestManager>(Actors[0]) : nullptr;
}

UPlayerInventory* ACrownsBaneHUD::GetPlayerInventory() const
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC) return nullptr;

	APawn* Pawn = PC->GetPawn();
	if (Pawn)
	{
		UPlayerInventory* Inv = Pawn->FindComponentByClass<UPlayerInventory>();
		if (Inv) return Inv;
	}

	return PC->FindComponentByClass<UPlayerInventory>();
}
