// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "UI/CrownsBaneHUD.h"
#include "Ship/ShipPawn.h"
#include "Combat/CannonComponent.h"
#include "Components/HealthComponent.h"
#include "Player/PlayerInventory.h"
#include "Systems/WantedLevelManager.h"
#include "Systems/WindSystem.h"
#include "Systems/StormSystem.h"
#include "Systems/DayNightSystem.h"
#include "Loot/TreasureQuestManager.h"
#include "Loot/TreasureChest.h"
#include "Loot/LootPickup.h"
#include "Loot/TreasureMapPickup.h"
#include "Docks/DocksZone.h"
#include "AI/EnemyShipBase.h"
#include "Components/HealthComponent.h"
#include "EngineUtils.h"
#include "Camera/CameraComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "CanvasItem.h"

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

	if (Ship)
	{
		DrawEnemyHealthBars(Ship);
		DrawFiringArcs(Ship);
		DrawAimPredictor(Ship);
	}

	if (Ship)
	{
		DrawMinimap(Ship, TreasureMgr);
	}

	if (Inventory)
	{
		DrawAmmoCounter(Inventory);
	}

	DrawCrosshair();
	DrawTimeOfDay();

	// Compute dt for floating damage numbers
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	const float Dt = (LastDrawTime > 0.0) ? (float)(Now - LastDrawTime) : 0.016f;
	LastDrawTime = Now;
	DrawFloatingDamageNumbers(Dt);

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

void ACrownsBaneHUD::DrawEnemyHealthBars(AShipPawn* PlayerShip)
{
	if (!PlayerShip || !Canvas) return;

	const FVector PlayerLoc = PlayerShip->GetActorLocation();

	for (TActorIterator<AEnemyShipBase> It(GetWorld()); It; ++It)
	{
		AEnemyShipBase* Enemy = *It;
		if (!Enemy || !IsValid(Enemy)) continue;
		if (!Enemy->HealthComponent || !Enemy->HealthComponent->IsAlive()) continue;

		// Distance cull
		const float DistSq = FVector::DistSquared(PlayerLoc, Enemy->GetActorLocation());
		if (DistSq > FMath::Square(EnemyHPDrawRange)) continue;

		// Project world pos to screen (above the ship mesh)
		const FVector WorldBarPos = Enemy->GetActorLocation() + FVector(0.f, 0.f, 280.f);
		const FVector Screen = Canvas->Project(WorldBarPos);

		// Behind camera or off-screen
		if (Screen.Z <= 0.f) continue;
		if (Screen.X < 0.f || Screen.X > Canvas->ClipX) continue;
		if (Screen.Y < 0.f || Screen.Y > Canvas->ClipY) continue;

		const float Pct  = Enemy->HealthComponent->GetHealthPercent();
		const float BX   = Screen.X - EnemyHPBarWidth * 0.5f;
		const float BY   = Screen.Y;

		// Background
		DrawFilledRect(BX - 1.f, BY - 1.f, EnemyHPBarWidth + 2.f, EnemyHPBarHeight + 2.f, EnemyHPBarBGColor);

		// HP fill — green to red gradient
		const FLinearColor FillColor = FLinearColor::LerpUsingHSV(
			FLinearColor(0.85f, 0.1f, 0.1f, 1.f),
			FLinearColor(0.15f, 0.75f, 0.15f, 1.f),
			Pct);
		DrawFilledRect(BX, BY, EnemyHPBarWidth * Pct, EnemyHPBarHeight, FillColor);

		// AI state tag
		const EShipAIState State = Enemy->GetAIState();
		FString StateTag;
		FColor StateColor = FColor::White;
		switch (State)
		{
		case EShipAIState::Patrol:  StateTag = TEXT("PATROL");  StateColor = FColor::Silver;  break;
		case EShipAIState::Chase:   StateTag = TEXT("CHASING"); StateColor = FColor::Orange;  break;
		case EShipAIState::Attack:  StateTag = TEXT("ATTACK");  StateColor = FColor::Red;     break;
		case EShipAIState::Retreat: StateTag = TEXT("FLEEING"); StateColor = FColor::Yellow;  break;
		case EShipAIState::Sink:    StateTag = TEXT("SINKING"); StateColor = FColor::Silver;  break;
		}

		const FString Label = FString::Printf(TEXT("%s  %.0f%%  [%s]"),
			*Enemy->GetName(), Pct * 100.f, *StateTag);
		DrawText(Label, StateColor, BX, BY - 16.f, nullptr, 0.7f, false);
	}
}

// -------- MINIMAP --------

void ACrownsBaneHUD::DrawMinimap(AShipPawn* PlayerShip, ATreasureQuestManager* TreasureMgr)
{
	if (!PlayerShip || !Canvas) return;

	const float CX = Canvas->ClipX - MinimapPadding - MinimapRadius;
	const float CY = MinimapPadding + MinimapRadius;

	// Background + border (approximated as a filled square for simplicity; visual is clamped by drawing)
	DrawFilledRect(CX - MinimapRadius - 3.f, CY - MinimapRadius - 3.f,
		(MinimapRadius + 3.f) * 2.f, (MinimapRadius + 3.f) * 2.f, MinimapBorderColor);
	DrawFilledRect(CX - MinimapRadius, CY - MinimapRadius,
		MinimapRadius * 2.f, MinimapRadius * 2.f, MinimapBGColor);

	const float ShipYawDeg = PlayerShip->GetActorRotation().Yaw;
	const float Scale = MinimapRadius / MinimapWorldRadius;
	const FVector ShipLoc = PlayerShip->GetActorLocation();

	// Cache sin/cos of -ShipYaw so "world forward = map up"
	const float CosYaw = FMath::Cos(FMath::DegreesToRadians(-ShipYawDeg));
	const float SinYaw = FMath::Sin(FMath::DegreesToRadians(-ShipYawDeg));

	auto WorldToMap = [&](const FVector& WorldPos, float& OutMX, float& OutMY) -> bool
	{
		FVector Rel = WorldPos - ShipLoc;
		Rel.Z = 0.f;
		if (Rel.SizeSquared() > FMath::Square(MinimapWorldRadius)) return false;

		// Rotate so ship forward points up on the minimap
		const float RX = Rel.X * CosYaw - Rel.Y * SinYaw;
		const float RY = Rel.X * SinYaw + Rel.Y * CosYaw;

		// In UE, X = forward, Y = right.  On-screen: up = -Y, right = +X
		OutMX = CX + RY * Scale;
		OutMY = CY - RX * Scale;
		return true;
	};

	// Camera view cone (camera yaw relative to ship yaw)
	if (PlayerShip->Camera)
	{
		const float CamYawAbs = PlayerShip->Camera->GetComponentRotation().Yaw;
		// Relative to ship forward; minimap "up" corresponds to ship forward
		const float CamRelYaw = FRotator::NormalizeAxis(CamYawAbs - ShipYawDeg) - 90.0f;
		DrawMinimapViewCone(CX, CY, CamRelYaw, MinimapRadius * 0.95f, MinimapViewConeAngle, MinimapViewConeColor);
	}

	// Entity dots
	UWorld* World = GetWorld();
	if (World)
	{
		for (TActorIterator<AEnemyShipBase> It(World); It; ++It)
		{
			AEnemyShipBase* E = *It;
			if (!E || !IsValid(E) || !E->HealthComponent || !E->HealthComponent->IsAlive()) continue;
			float MX, MY;
			if (WorldToMap(E->GetActorLocation(), MX, MY))
				DrawMinimapDot(CX, CY, MX, MY, 4.f, MinimapEnemyColor);
		}
		for (TActorIterator<ALootPickup> It(World); It; ++It)
		{
			if (!*It || !IsValid(*It)) continue;
			float MX, MY;
			if (WorldToMap((*It)->GetActorLocation(), MX, MY))
				DrawMinimapDot(CX, CY, MX, MY, 3.f, MinimapLootColor);
		}
		for (TActorIterator<ATreasureChest> It(World); It; ++It)
		{
			if (!*It || !IsValid(*It)) continue;
			float MX, MY;
			if (WorldToMap((*It)->GetActorLocation(), MX, MY))
				DrawMinimapDot(CX, CY, MX, MY, 5.f, MinimapTreasureColor);
		}
		for (TActorIterator<ATreasureMapPickup> It(World); It; ++It)
		{
			if (!*It || !IsValid(*It)) continue;
			float MX, MY;
			if (WorldToMap((*It)->GetActorLocation(), MX, MY))
				DrawMinimapDot(CX, CY, MX, MY, 4.f, MinimapTreasureColor);
		}
		for (TActorIterator<ADocksZone> It(World); It; ++It)
		{
			if (!*It || !IsValid(*It)) continue;
			float MX, MY;
			if (WorldToMap((*It)->GetActorLocation(), MX, MY))
				DrawMinimapDot(CX, CY, MX, MY, 6.f, MinimapDockColor);
		}
	}

	// Player triangle at center pointing up (ship forward)
	DrawMinimapTriangle(CX, CY, -90.0f, 10.0f, MinimapPlayerColor);

	// Label
	DrawText(TEXT("MINIMAP"), FColor::White, CX - 28.f, CY - MinimapRadius - 16.f, nullptr, 0.75f, false);
	const FString ScaleLabel = FString::Printf(TEXT("%.0fm"), MinimapWorldRadius / 100.f);
	DrawText(ScaleLabel, FColor::Silver, CX - 12.f, CY + MinimapRadius + 4.f, nullptr, 0.7f, false);
}

void ACrownsBaneHUD::DrawMinimapDot(float CX, float CY, float DotX, float DotY, float DotSize, FLinearColor Color)
{
	// Clip to minimap circle
	const float DX = DotX - CX;
	const float DY = DotY - CY;
	if (DX * DX + DY * DY > MinimapRadius * MinimapRadius) return;

	const float Half = DotSize * 0.5f;
	DrawFilledRect(DotX - Half, DotY - Half, DotSize, DotSize, Color);
}

void ACrownsBaneHUD::DrawMinimapTriangle(float CX, float CY, float AngleDeg, float Size, FLinearColor Color)
{
	const float Rad = FMath::DegreesToRadians(AngleDeg);
	const float TipX = CX + FMath::Cos(Rad) * Size;
	const float TipY = CY + FMath::Sin(Rad) * Size;

	const float BackRad = Rad + PI;
	const float BackX = CX + FMath::Cos(BackRad) * (Size * 0.5f);
	const float BackY = CY + FMath::Sin(BackRad) * (Size * 0.5f);

	const float LeftRad = Rad + PI * 0.8f;
	const float LeftX = CX + FMath::Cos(LeftRad) * (Size * 0.7f);
	const float LeftY = CY + FMath::Sin(LeftRad) * (Size * 0.7f);

	const float RightRad = Rad - PI * 0.8f;
	const float RightX = CX + FMath::Cos(RightRad) * (Size * 0.7f);
	const float RightY = CY + FMath::Sin(RightRad) * (Size * 0.7f);

	// Three thick lines approximating a filled triangle
	auto DrawThick = [this, Color](float X1, float Y1, float X2, float Y2)
	{
		FCanvasLineItem L(FVector2D(X1, Y1), FVector2D(X2, Y2));
		L.SetColor(Color);
		L.LineThickness = 2.5f;
		Canvas->DrawItem(L);
	};
	DrawThick(TipX, TipY, LeftX, LeftY);
	DrawThick(TipX, TipY, RightX, RightY);
	DrawThick(LeftX, LeftY, RightX, RightY);
	// Filled center dot for body
	DrawFilledRect(CX - 2.f, CY - 2.f, 4.f, 4.f, Color);
}

void ACrownsBaneHUD::DrawMinimapViewCone(float CX, float CY, float AngleDeg, float Radius, float FOVDeg, FLinearColor Color)
{
	// Draw a fan of line segments approximating the FOV sector
	const float HalfFOV = FOVDeg * 0.5f;
	const int32 Segments = 12;
	const float Start = AngleDeg - HalfFOV;
	const float Step = FOVDeg / (float)Segments;

	for (int32 i = 0; i < Segments; ++i)
	{
		const float A1 = FMath::DegreesToRadians(Start + Step * i);
		const float A2 = FMath::DegreesToRadians(Start + Step * (i + 1));
		const FVector2D P1(CX + FMath::Cos(A1) * Radius, CY + FMath::Sin(A1) * Radius);
		const FVector2D P2(CX + FMath::Cos(A2) * Radius, CY + FMath::Sin(A2) * Radius);
		// Triangle fan edge
		FCanvasLineItem L(FVector2D(CX, CY), P1);
		L.SetColor(Color);
		L.LineThickness = 1.5f;
		Canvas->DrawItem(L);
		FCanvasLineItem LArc(P1, P2);
		LArc.SetColor(Color);
		LArc.LineThickness = 1.5f;
		Canvas->DrawItem(LArc);
	}
	// Final edge to close the sector
	const float FA = FMath::DegreesToRadians(AngleDeg + HalfFOV);
	FCanvasLineItem L(FVector2D(CX, CY),
		FVector2D(CX + FMath::Cos(FA) * Radius, CY + FMath::Sin(FA) * Radius));
	L.SetColor(Color);
	L.LineThickness = 1.5f;
	Canvas->DrawItem(L);
}

// -------- FIRING ARCS (camera-aim indicator) --------

void ACrownsBaneHUD::DrawFiringArcs(AShipPawn* PlayerShip)
{
	if (!PlayerShip || !PlayerShip->Camera || !PlayerShip->CannonComponent) return;

	// Determine which side the camera is aimed toward
	const FVector CamFwd = PlayerShip->Camera->GetForwardVector();
	const FVector ShipRight = PlayerShip->GetActorRightVector();
	const float DotR = FVector::DotProduct(CamFwd, ShipRight);

	FString AimLabel;
	FColor AimColor = FColor::Silver;
	if (DotR > 0.35f)       { AimLabel = TEXT("AIM: STARBOARD"); AimColor = FColor::Green; }
	else if (DotR < -0.35f) { AimLabel = TEXT("AIM: PORT");      AimColor = FColor::Green; }
	else                    { AimLabel = TEXT("AIM: CENTER (turn camera)"); AimColor = FColor::Orange; }

	const float ScreenW = Canvas->ClipX;
	const float ScreenH = Canvas->ClipY;
	DrawText(AimLabel, AimColor,
		ScreenW * 0.5f - 80.0f,
		ScreenH - HUDPaddingY - 80.0f,
		nullptr, 0.95f, false);

	// Reload-readiness pill per side
	const bool bPortReady = PlayerShip->CannonComponent->CanFire(ECannonSide::Left);
	const bool bStbdReady = PlayerShip->CannonComponent->CanFire(ECannonSide::Right);

	FString HintsLine;
	if (DotR > 0.35f)
		HintsLine = FString::Printf(TEXT("SPACE / LMB to fire STARBOARD broadside [%s]"),
			bStbdReady ? TEXT("READY") : TEXT("reloading..."));
	else if (DotR < -0.35f)
		HintsLine = FString::Printf(TEXT("SPACE / LMB to fire PORT broadside [%s]"),
			bPortReady ? TEXT("READY") : TEXT("reloading..."));
	else
		HintsLine = TEXT("Turn camera left/right to aim broadside");

	DrawText(HintsLine, FColor(220, 220, 220),
		ScreenW * 0.5f - 160.0f,
		ScreenH - HUDPaddingY - 62.0f,
		nullptr, 0.8f, false);
}

void ACrownsBaneHUD::DrawAmmoCounter(UPlayerInventory* Inventory)
{
	if (!Inventory) return;
	const int32 Cur = Inventory->GetAmmo();
	const int32 Max = Inventory->GetMaxAmmo();

	const float ScreenW = Canvas->ClipX;
	const float RightX = ScreenW - HUDPaddingX - 200.0f;
	const FString AmmoText = FString::Printf(TEXT("Ammo: %d / %d"), Cur, Max);
	const FColor Color = (Cur <= 4) ? FColor::Red : (Cur <= 10 ? FColor::Orange : FColor(220, 220, 180));
	DrawText(AmmoText, Color, RightX, HUDPaddingY + 72.0f, nullptr, 1.0f, false);
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

// -------- AIM PREDICTOR (AC4-style) --------

void ACrownsBaneHUD::DrawAimPredictor(AShipPawn* PlayerShip)
{
	if (!PlayerShip || !PlayerShip->Camera || !PlayerShip->CannonComponent || !Canvas) return;

	// Decide which side we're aiming toward.
	const FVector CamFwd   = PlayerShip->Camera->GetForwardVector();
	const FVector ShipRight = PlayerShip->GetActorRightVector();
	const float DotR = FVector::DotProduct(CamFwd, ShipRight);
	if (FMath::Abs(DotR) < 0.35f) return; // centered — no prediction

	const ECannonSide Side = (DotR > 0.f) ? ECannonSide::Right : ECannonSide::Left;
	const bool bReady = PlayerShip->CannonComponent->CanFire(Side);
	const FLinearColor ArcColor = bReady ? AimArcColorReady : AimArcColorReload;

	// Generate trajectories
	TArray<FVector> ImpactPoints, Starts, Ends;
	PlayerShip->CannonComponent->GetAimPrediction(Side, SeaLevelZ, ImpactPoints, Starts, Ends);

	// Draw each trajectory segment in screen space.
	const int32 N = FMath::Min(Starts.Num(), Ends.Num());
	for (int32 i = 0; i < N; ++i)
	{
		const FVector S3 = Canvas->Project(Starts[i]);
		const FVector E3 = Canvas->Project(Ends[i]);
		// Skip segments behind the camera
		if (S3.Z <= 0.f || E3.Z <= 0.f) continue;

		FCanvasLineItem L(FVector2D(S3.X, S3.Y), FVector2D(E3.X, E3.Y));
		L.SetColor(ArcColor);
		L.LineThickness = 2.0f;
		Canvas->DrawItem(L);
	}

	// Draw impact rings (rough ellipses) at landing points.
	for (const FVector& Impact : ImpactPoints)
	{
		const FVector P0 = Canvas->Project(Impact);
		if (P0.Z <= 0.f) continue;

		// Approximate ellipse by projecting a ring in world space.
		const int32 Segs = 20;
		const float R = AimImpactRingRadius;
		FVector2D PrevScreen;
		for (int32 s = 0; s <= Segs; ++s)
		{
			const float A = (float)s / (float)Segs * 2.0f * PI;
			const FVector Ring = Impact + FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, 0.0f);
			const FVector RS = Canvas->Project(Ring);
			if (RS.Z <= 0.f) { PrevScreen = FVector2D(-1, -1); continue; }

			if (s > 0 && PrevScreen.X >= 0.0f)
			{
				FCanvasLineItem Ln(PrevScreen, FVector2D(RS.X, RS.Y));
				Ln.SetColor(AimImpactRingColor);
				Ln.LineThickness = 2.0f;
				Canvas->DrawItem(Ln);
			}
			PrevScreen = FVector2D(RS.X, RS.Y);
		}

		// Small dot at the center
		DrawFilledRect(P0.X - 3.f, P0.Y - 3.f, 6.f, 6.f, AimImpactRingColor);
	}
}

// -------- CROSSHAIR --------

void ACrownsBaneHUD::DrawCrosshair()
{
	if (!Canvas) return;
	const float CX = Canvas->ClipX * 0.5f;
	const float CY = Canvas->ClipY * 0.5f;

	// Four tick marks
	DrawFilledRect(CX - CrosshairSize, CY - 1.f, CrosshairSize * 0.55f, 2.f, CrosshairColor);
	DrawFilledRect(CX + CrosshairSize * 0.45f, CY - 1.f, CrosshairSize * 0.55f, 2.f, CrosshairColor);
	DrawFilledRect(CX - 1.f, CY - CrosshairSize, 2.f, CrosshairSize * 0.55f, CrosshairColor);
	DrawFilledRect(CX - 1.f, CY + CrosshairSize * 0.45f, 2.f, CrosshairSize * 0.55f, CrosshairColor);
	// Centre dot
	DrawFilledRect(CX - 1.5f, CY - 1.5f, 3.f, 3.f, CrosshairColor);
}

// -------- FLOATING DAMAGE NUMBERS --------

void ACrownsBaneHUD::AddFloatingDamage(FVector WorldLocation, float Damage, bool bHitShip)
{
	if (Damage <= 0.0f) return;

	FFloatingDamageEntry Entry;
	Entry.WorldLocation = WorldLocation;
	Entry.Damage        = Damage;
	Entry.TimeRemaining = 1.4f;
	Entry.VerticalSpeed = 140.0f;
	Entry.Tint          = bHitShip ? FColor(255, 200, 80) : FColor(180, 220, 255);
	FloatingDamageEntries.Add(Entry);

	// Cap list size to avoid runaway growth
	if (FloatingDamageEntries.Num() > 32)
	{
		FloatingDamageEntries.RemoveAt(0, FloatingDamageEntries.Num() - 32);
	}
}

void ACrownsBaneHUD::DrawFloatingDamageNumbers(float DeltaTime)
{
	if (!Canvas) return;

	for (int32 i = FloatingDamageEntries.Num() - 1; i >= 0; --i)
	{
		FFloatingDamageEntry& E = FloatingDamageEntries[i];
		E.TimeRemaining -= DeltaTime;
		if (E.TimeRemaining <= 0.f)
		{
			FloatingDamageEntries.RemoveAtSwap(i);
			continue;
		}

		// Rise in world space
		E.WorldLocation.Z += E.VerticalSpeed * DeltaTime;

		const FVector Screen = Canvas->Project(E.WorldLocation);
		if (Screen.Z <= 0.f) continue;

		const float Alpha = FMath::Clamp(E.TimeRemaining / 1.4f, 0.f, 1.f);
		FColor Tint = E.Tint;
		Tint.A = (uint8)(Alpha * 255.f);

		const FString Text = FString::Printf(TEXT("-%.0f"), E.Damage);
		const float Scale = 1.1f + (1.0f - Alpha) * 0.4f;
		DrawText(Text, Tint, Screen.X - 14.f, Screen.Y, nullptr, Scale, false);
	}
}

// -------- TIME-OF-DAY CLOCK --------

void ACrownsBaneHUD::DrawTimeOfDay()
{
	if (!Canvas) return;
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADayNightSystem::StaticClass(), Actors);
	if (Actors.Num() == 0) return;

	ADayNightSystem* Sys = Cast<ADayNightSystem>(Actors[0]);
	if (!Sys) return;

	const float T = Sys->TimeOfDay;
	const int32 Hours = FMath::FloorToInt(T);
	const int32 Mins  = FMath::FloorToInt((T - Hours) * 60.f);

	const bool bNight = Sys->IsNight();
	const FColor Tint = bNight ? FColor(140, 170, 255) : FColor(255, 220, 120);
	const TCHAR* Icon = bNight ? TEXT("☽") : TEXT("☀"); // moon / sun

	const FString Clock = FString::Printf(TEXT("%s  %02d:%02d"), Icon, Hours, Mins);

	// Position: top center, slightly below wanted stars
	const float X = Canvas->ClipX * 0.5f - 55.f;
	const float Y = HUDPaddingY + StarSize + 56.f;
	DrawFilledRect(X - 6.f, Y - 4.f, 120.f, 22.f, FLinearColor(0.f, 0.f, 0.f, 0.55f));
	DrawText(Clock, Tint, X, Y, nullptr, 1.0f, false);
}
