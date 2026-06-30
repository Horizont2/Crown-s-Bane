// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "UI/CrownsBaneHUD.h"
#include "UI/UICrownStyle.h"
#include "Ship/ShipPawn.h"
#include "Combat/CannonComponent.h"
#include "Components/HealthComponent.h"
#include "Player/PlayerInventory.h"
#include "Systems/WantedLevelManager.h"
#include "Systems/WindSystem.h"
#include "Systems/StormSystem.h"
#include "Systems/DayNightSystem.h"
#include "Loot/TreasureQuestManager.h"
#include "Quests/BountyManager.h"
#include "Loot/TreasureChest.h"
#include "Loot/LootPickup.h"
#include "Loot/TreasureMapPickup.h"
#include "Docks/DocksZone.h"
#include "AI/EnemyShipBase.h"
#include "Upgrades/UpgradeManager.h"
#include "Upgrades/UpgradeTypes.h"
#include "Player/CrownsBanePlayerController.h"
#include "Player/ShipProgressionComponent.h"
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
	DrawDamageFlash(Dt);
	DrawHitMarker(Dt);
	DrawResourceToasts(Dt);
	if (Ship) DrawLowHealthFlash(Ship);

	if (TreasureMgr && Ship)
	{
		DrawActiveQuestTracker(TreasureMgr, Ship);
		if (bShowQuestLog) DrawQuestLog(TreasureMgr, Ship);
	}

	// Upgrade menu — drawn when PlayerController says it is open.
	if (APlayerController* RawPC = GetOwningPlayerController())
	{
		if (ACrownsBanePlayerController* CBPC = Cast<ACrownsBanePlayerController>(RawPC))
		{
			if (CBPC->IsUpgradeUIOpen())
			{
				AUpgradeManager* UM = nullptr;
				TArray<AActor*> Found;
				UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUpgradeManager::StaticClass(), Found);
				if (Found.Num() > 0) UM = Cast<AUpgradeManager>(Found[0]);
				DrawUpgradeMenu(CBPC, UM, Inventory);
			}
			if (CBPC->IsTraderMenuOpen())
			{
				DrawTraderMenu(CBPC, Inventory);
			}
		}
	}

	if (bShowDocksPrompt)
	{
		DrawDocksPrompt();
	}

	if (Ship)
	{
		DrawBoardingPrompt(Ship);
		DrawLockOnReticle(Ship);
		DrawLeadIndicator(Ship);
		DrawEnemyInfoCard(Ship);
	}
	DrawDamageDirection(Dt);
	DrawComboCounter(Dt);
	DrawKillFeed(Dt);
	DrawBanner(Dt);
	DrawMissionComplete(Dt);

	if (APlayerController* RawPC2 = GetOwningPlayerController())
	{
		if (ACrownsBanePlayerController* CBPC2 = Cast<ACrownsBanePlayerController>(RawPC2))
		{
			if (CBPC2->IsPauseMenuOpen()) DrawPauseMenu(CBPC2);
		}
	}

	if (bShowHelpOverlay) DrawHelpOverlay();

	if (APlayerController* RawPC3 = GetOwningPlayerController())
	{
		if (ACrownsBanePlayerController* CBPC3 = Cast<ACrownsBanePlayerController>(RawPC3))
		{
			DrawXPBar(CBPC3);
			DrawPerkChoiceOverlay(CBPC3);
		}
	}
}

void ACrownsBaneHUD::DrawTextWithShadow(const FString& Text, FColor TextColor, float X, float Y, UFont* Font, float Scale)
{
	if (!Canvas) return;
	// Тінь (зміщена на 2 пікселі вниз і вправо)
	DrawText(Text, FColor(0, 0, 0, 180), X + 2.0f, Y + 2.0f, Font, Scale, false);
	// Основний текст
	DrawText(Text, TextColor, X, Y, Font, Scale, false);
}

void ACrownsBaneHUD::DrawMinimalistBar(float X, float Y, float W, float H, float Pct, FLinearColor FillColor)
{
	// Тонка темна основа (стиль KCD2)
	DrawFilledRect(X, Y, W, H, FLinearColor(0.05f, 0.05f, 0.05f, 0.85f));
	// Заповнення
	DrawFilledRect(X + 1.0f, Y + 1.0f, (W - 2.0f) * FMath::Clamp(Pct, 0.0f, 1.0f), H - 2.0f, FillColor);
	// Тонка золота/срібна рамка
	DrawBorderedRect(X, Y, W, H, FLinearColor::Transparent, FLinearColor(0.6f, 0.5f, 0.3f, 0.4f), 1.0f);
}

void ACrownsBaneHUD::DrawHealthBar(AShipPawn* Ship)
{
	if (!Ship || !Ship->HealthComponent) return;

	const float TargetPct = Ship->HealthComponent->GetHealthPercent();
	const float DeltaTime = GetWorld()->GetDeltaSeconds();
	if (DisplayedHealthPct < 0.0f) DisplayedHealthPct = TargetPct;
	DisplayedHealthPct = FMath::FInterpTo(DisplayedHealthPct, TargetPct, DeltaTime, 5.0f);

	const float ScreenH = Canvas->ClipY;
	const float BarX = HUDPaddingX + 10.f;
	const float BarY = ScreenH - HUDPaddingY - HealthBarHeight - 50.f;

	// Outer panel
	DrawPanel(BarX - 6.f, BarY - 26.f, HealthBarWidth + 12.f, HealthBarHeight + 56.f,
		(uint8)CrownStyle::EPanelStyle::Subtle);

	// Header
	DrawText(FString::Printf(TEXT("HULL  %.0f / %.0f"),
			Ship->HealthComponent->GetCurrentHealth(), Ship->HealthComponent->GetMaxHealth()),
		CrownStyle::TextPrimary, BarX, BarY - 22.f, nullptr, 0.95f, false);

	// Segmented bar — 20 chunks separated by 1-px gutters.  Empty chunks dim,
	// full chunks tinted by overall pct (green->red).
	const int32 Segments = 20;
	const float SegW = (HealthBarWidth - (Segments - 1)) / (float)Segments;
	const FLinearColor FillColor = FLinearColor::LerpUsingHSV(
		FLinearColor(0.85f, 0.1f, 0.1f, 1.0f), HealthBarColor, DisplayedHealthPct);

	for (int32 i = 0; i < Segments; ++i)
	{
		const float SegX = BarX + i * (SegW + 1.f);
		const float ThisFrac = FMath::Clamp(DisplayedHealthPct * Segments - i, 0.f, 1.f);
		// Background segment
		DrawFilledRect(SegX, BarY, SegW, HealthBarHeight, FLinearColor(0.08f, 0.06f, 0.04f, 0.85f));
		if (ThisFrac > 0.0f)
		{
			DrawFilledRect(SegX, BarY, SegW * ThisFrac, HealthBarHeight, FillColor);
			// Highlight top edge
			DrawFilledRect(SegX, BarY, SegW * ThisFrac, 1.f, FLinearColor(1.0f, 1.0f, 1.0f, 0.35f));
		}
	}

	// Armor indicator: if ArmorReduction > 0, draw a thin gold strip on top of the bar
	// indicating mitigation coverage.
	if (Ship->ArmorReduction > 0.0f)
	{
		DrawFilledRect(BarX, BarY - 4.f, HealthBarWidth * Ship->ArmorReduction, 3.f, CrownStyle::AccentGold);
	}

	// Speed line below.
	const TCHAR* SailLabel = Ship->GetSailLevel() == ESailLevel::Stop ? TEXT("ANCHORED")
		: Ship->GetSailLevel() == ESailLevel::HalfSail ? TEXT("HALF SAIL") : TEXT("FULL SAIL");
	DrawText(FString::Printf(TEXT("%.0f knots   |   %s"), Ship->GetCurrentSpeed() / 50.f, SailLabel),
		FColor(180, 200, 220), BarX, BarY + HealthBarHeight + 6.f, nullptr, 0.85f, false);

	// Sail integrity warning when torn.
	if (Ship->SailIntegrity < 0.99f)
	{
		const FColor SailCol = (Ship->SailIntegrity < 0.5f) ? FColor(255, 120, 120) : FColor(255, 200, 100);
		DrawText(FString::Printf(TEXT("SAILS  %.0f%%"), Ship->SailIntegrity * 100.f),
			SailCol, BarX + HealthBarWidth - 90.f, BarY + HealthBarHeight + 6.f,
			nullptr, 0.85f, false);
	}
}

void ACrownsBaneHUD::DrawReloadTimers(UCannonComponent* Cannons)
{
	if (!Cannons) return;
	float CenterX = Canvas->ClipX * 0.5f;
	float BarY = Canvas->ClipY - HUDPaddingY - ReloadBarHeight - 20.0f;

	// Пульсація для готових гармат
	float PulseAlpha = FMath::Sin(GetWorld()->GetTimeSeconds() * 6.0f) * 0.5f + 0.5f;
	FLinearColor PulsingReadyColor = FLinearColor::LerpUsingHSV(ReloadReadyColor, FLinearColor::White, PulseAlpha * 0.3f);

	auto DrawSide = [&](ECannonSide Side, float XOffset, const FString& LabelKey) {
		float Progress = Cannons->GetReloadProgress(Side);
		float BarX = CenterX + XOffset;
		FLinearColor Color = (Progress >= 1.0f) ? PulsingReadyColor : FLinearColor::LerpUsingHSV(ReloadEmptyColor, ReloadReadyColor, Progress);

		DrawMinimalistBar(BarX, BarY, ReloadBarWidth, ReloadBarHeight * 0.4f, Progress, Color);

		FString Label = Cannons->CanFire(Side) ? FString::Printf(TEXT("%s READY"), *LabelKey) : FString::Printf(TEXT("RELOADING..."));
		FColor TextColor = (Progress >= 1.0f) ? FColor(255, 215, 0, 200 + (int)(PulseAlpha * 55)) : FColor(150, 150, 150, 200);
		DrawTextWithShadow(Label, TextColor, BarX, BarY - 18.0f, nullptr, 0.8f);
		};

	DrawSide(ECannonSide::Left, -ReloadBarWidth - 30.0f, TEXT("PORT [Q]"));
	DrawSide(ECannonSide::Right, 30.0f, TEXT("STBD [E]"));
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
	if (!Inventory || !Canvas) return;

	const float ScreenW = Canvas->ClipX;
	const float PW = 200.f;
	const float PH = 84.f;
	const float PX = ScreenW - HUDPaddingX - PW;
	const float PY = HUDPaddingY + 40.f; // below minimap header

	DrawPanel(PX, PY, PW, PH, (uint8)CrownStyle::EPanelStyle::Subtle);

	struct FRow { FLinearColor Tint; const TCHAR* Label; int32 Val; };
	const FRow Rows[3] = {
		{ CrownStyle::AccentGold,                                   TEXT("GOLD"),  Inventory->GetGold()  },
		{ FLinearColor(0.65f, 0.45f, 0.25f, 1.f),                   TEXT("WOOD"),  Inventory->GetWood()  },
		{ FLinearColor(0.75f, 0.78f, 0.85f, 1.f),                   TEXT("METAL"), Inventory->GetMetal() },
	};

	for (int32 i = 0; i < 3; ++i)
	{
		const float RY = PY + 10.f + i * 24.f;
		// Colored badge
		DrawFilledRect(PX + 10.f, RY + 2.f, 14.f, 14.f, Rows[i].Tint);
		// Label + value
		DrawText(Rows[i].Label, CrownStyle::TextSecondary,
			PX + 32.f, RY, nullptr, 0.95f, false);
		DrawText(FString::Printf(TEXT("%d"), Rows[i].Val),
			Rows[i].Tint.ToFColor(true),
			PX + PW - 80.f, RY, nullptr, 1.0f, false);
	}
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

	// Cardinal directions (N/S/E/W) at the minimap rim, rotated with ship heading.
	// World "N" is +X, rotated -ShipYaw so it sits at the correct map angle.
	auto DrawCardinal = [&](const TCHAR* L, float WorldAngleDeg, FColor Tint)
	{
		const float Ang = FMath::DegreesToRadians(WorldAngleDeg - ShipYawDeg - 90.f);
		const float Rim = MinimapRadius - 12.f;
		const float LX = CX + FMath::Cos(Ang) * Rim - 5.f;
		const float LY = CY + FMath::Sin(Ang) * Rim - 7.f;
		DrawText(L, Tint, LX, LY, nullptr, 0.9f, false);
	};
	DrawCardinal(TEXT("N"), 90.f,  FColor(255, 200, 80));  // N highlighted
	DrawCardinal(TEXT("E"),  0.f,  FColor(180, 180, 180));
	DrawCardinal(TEXT("S"), -90.f, FColor(180, 180, 180));
	DrawCardinal(TEXT("W"), 180.f, FColor(180, 180, 180));

	// Label
	DrawText(TEXT("MINIMAP"), FColor::White, CX - 28.f, CY - MinimapRadius - 16.f, nullptr, 0.75f, false);
	const FString ScaleLabel = FString::Printf(TEXT("%.0fm"), MinimapWorldRadius / 100.f);
	DrawText(ScaleLabel, FColor::Silver, CX - 12.f, CY + MinimapRadius + 4.f, nullptr, 0.7f, false);

	// Heading degrees (ship yaw in compass form: 0=N, 90=E, etc.)
	const float CompassYaw = FMath::Fmod(450.f - ShipYawDeg, 360.f);
	const FString Heading = FString::Printf(TEXT("HDG  %.0f°"), CompassYaw);
	DrawText(Heading, FColor(220, 220, 220), CX - 30.f, CY + MinimapRadius + 16.f, nullptr, 0.85f, false);
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

	// Show a compact AIM-side label only when actually aimed at a broadside (so
	// it doesn't clutter the centre when the player is just sailing).  Spammy
	// "Turn camera..." hint removed — F1 documents it once.
	if (FMath::Abs(DotR) < 0.35f) return;

	const FString AimLabel = (DotR > 0.f) ? TEXT("AIM ▶ STARBOARD") : TEXT("◀ AIM PORT");
	const FColor  AimColor = FColor(120, 220, 120);

	const float ScreenW = Canvas->ClipX;
	const float ScreenH = Canvas->ClipY;
	DrawText(AimLabel, AimColor,
		ScreenW * 0.5f - 70.0f,
		ScreenH - HUDPaddingY - 80.0f,
		nullptr, 1.0f, false);
}

void ACrownsBaneHUD::DrawAmmoCounter(UPlayerInventory* Inventory)
{
	if (!Inventory || !Canvas) return;
	const int32 Cur = Inventory->GetAmmo();
	const int32 Max = Inventory->GetMaxAmmo();

	const float ScreenW = Canvas->ClipX;
	const float ScreenH = Canvas->ClipY;
	const float PW = 220.f;
	const float PH = 64.f;
	const float PX = ScreenW - HUDPaddingX - PW;
	const float PY = ScreenH - HUDPaddingY - PH - 110.f;

	DrawPanel(PX, PY, PW, PH, (uint8)CrownStyle::EPanelStyle::Subtle);

	// Active ammo type from the player's CannonComponent.
	const TCHAR* TypeNames[5]  = { TEXT("Round"), TEXT("Chain"), TEXT("Grape"), TEXT("Heavy"), TEXT("Explosive") };
	const FLinearColor TypeCol[5] = {
		FLinearColor(0.85f, 0.85f, 0.85f, 1.f),     // Round - silver
		FLinearColor(0.85f, 0.65f, 0.30f, 1.f),     // Chain - copper
		FLinearColor(0.7f,  1.0f,  0.4f,  1.f),     // Grape - green
		FLinearColor(0.6f,  0.6f,  0.85f, 1.f),     // Heavy - blue
		FLinearColor(1.0f,  0.45f, 0.15f, 1.f),     // Explosive - orange
	};
	int32 TypeIdx = 0;
	if (AShipPawn* Ship = GetPlayerShip())
	{
		if (Ship->CannonComponent)
		{
			switch (Ship->CannonComponent->ActiveCannonballType)
			{
			case ECannonballType::Standard:  TypeIdx = 0; break;
			case ECannonballType::Chain:     TypeIdx = 1; break;
			case ECannonballType::Grape:     TypeIdx = 2; break;
			case ECannonballType::Heavy:     TypeIdx = 3; break;
			case ECannonballType::Explosive: TypeIdx = 4; break;
			default:                         TypeIdx = 0; break;
			}
		}
	}

	// Colored badge for ammo type + name
	DrawFilledRect(PX + 10.f, PY + 10.f, 14.f, 18.f, TypeCol[TypeIdx]);
	DrawText(TypeNames[TypeIdx], TypeCol[TypeIdx].ToFColor(true),
		PX + 32.f, PY + 10.f, nullptr, 1.05f, false);

	// Ammo count
	const FColor CountCol = (Cur <= 4) ? FColor::Red : (Cur <= 10 ? FColor::Orange : FColor(255, 230, 140));
	DrawText(FString::Printf(TEXT("%d / %d"), Cur, Max), CountCol,
		PX + PW - 90.f, PY + 10.f, nullptr, 1.05f, false);

	// 1-5 hint
	DrawText(TEXT("[1-5] switch ammo"), CrownStyle::TextDim,
		PX + 10.f, PY + 36.f, nullptr, 0.85f, false);
}

void ACrownsBaneHUD::DrawDocksPrompt()
{
	const float SW = Canvas->ClipX;
	const float SH = Canvas->ClipY;

	// Pull dock context from the player's controller if available.
	FString DockTitle = TEXT("DOCKED");
	FLinearColor TitleTint = CrownStyle::AccentGold;
	if (APlayerController* RawPC = GetOwningPlayerController())
	{
		if (ACrownsBanePlayerController* CBPC = Cast<ACrownsBanePlayerController>(RawPC))
		{
			if (CBPC->CurrentDocksZone)
			{
				const ADocksZone* DZ = CBPC->CurrentDocksZone;
				const TCHAR* T = (DZ->DockType == EDockType::Naval) ? TEXT("NAVAL")
					: (DZ->DockType == EDockType::PirateHaven) ? TEXT("PIRATE HAVEN") : TEXT("MERCHANT PORT");
				DockTitle = FString::Printf(TEXT("%s — %s"), T, *DZ->DockName);
				TitleTint = (DZ->DockType == EDockType::Naval) ? FLinearColor(0.5f, 0.7f, 1.0f, 1.0f)
					: (DZ->DockType == EDockType::PirateHaven) ? FLinearColor(1.0f, 0.5f, 0.3f, 1.0f) : CrownStyle::AccentGold;
			}
		}
	}

	const float W = 360.f;
	const float H = 76.f;
	const float X = (SW - W) * 0.5f;
	const float Y = SH * 0.30f;
	DrawPanel(X, Y, W, H, (uint8)CrownStyle::EPanelStyle::Highlight);

	DrawText(DockTitle, TitleTint.ToFColor(true), X + CrownStyle::Sp3, Y + 8.f, nullptr, 1.3f, false);
	DrawText(TEXT("[U] Upgrades    [T] Trader    [J] Quest log"),
		CrownStyle::TextPrimary, X + CrownStyle::Sp3, Y + 42.f, nullptr, 0.95f, false);
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
	if (!Canvas) return;

	// 10-vertex polygon (5 outer points + 5 inner) — proper five-pointed star
	// drawn as 5 filled triangles emanating from center.  Plus a centre core for solidity.
	const float Inner = Radius * 0.4f;
	FVector2D Pts[10];
	for (int32 i = 0; i < 10; ++i)
	{
		const float A = FMath::DegreesToRadians(-90.f + i * 36.f);
		const float R = (i % 2 == 0) ? Radius : Inner;
		Pts[i] = FVector2D(CX + FMath::Cos(A) * R, CY + FMath::Sin(A) * R);
	}

	// Solid centre to fill the polygon
	const float Core = Radius * 0.55f;
	DrawFilledRect(CX - Core * 0.5f, CY - Core * 0.5f, Core, Core, Color);

	// Outline edges — gives star its silhouette.
	for (int32 i = 0; i < 10; ++i)
	{
		FCanvasLineItem L(Pts[i], Pts[(i + 1) % 10]);
		L.SetColor(Color);
		L.LineThickness = 2.0f;
		Canvas->DrawItem(L);
		// Spoke from center to outer point (adds the "rays")
		if (i % 2 == 0)
		{
			FCanvasLineItem S(FVector2D(CX, CY), Pts[i]);
			S.SetColor(Color);
			S.LineThickness = 2.0f;
			Canvas->DrawItem(S);
		}
	}

	// Inner glow halo when bright (alpha>0.6 = filled star)
	if (Color.A > 0.5f)
	{
		const FLinearColor Halo(Color.R, Color.G, Color.B, 0.15f);
		DrawFilledRect(CX - Radius * 0.95f, CY - Radius * 0.95f,
			Radius * 1.9f, Radius * 1.9f, Halo);
	}
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

	const FVector CamFwd = PlayerShip->Camera->GetForwardVector();
	const FVector ShipRight = PlayerShip->GetActorRightVector();
	const float DotR = FVector::DotProduct(CamFwd, ShipRight);
	if (FMath::Abs(DotR) < 0.35f) return;

	const ECannonSide Side = (DotR > 0.f) ? ECannonSide::Right : ECannonSide::Left;
	const bool bReady = PlayerShip->CannonComponent->CanFire(Side);

	// Пульсація прицілу
	float Pulse = FMath::Sin(GetWorld()->GetTimeSeconds() * 8.0f) * 0.15f + 0.85f;
	FLinearColor ArcColor = bReady ? AimArcColorReady : AimArcColorReload;
	ArcColor.A *= (bReady ? Pulse : 0.5f); // Блимає тільки коли готово

	TArray<FVector> ImpactPoints, Starts, Ends;
	PlayerShip->CannonComponent->GetAimPrediction(Side, SeaLevelZ, ImpactPoints, Starts, Ends);

	const int32 N = FMath::Min(Starts.Num(), Ends.Num());
	for (int32 i = 0; i < N; ++i)
	{
		const FVector S3 = Canvas->Project(Starts[i]);
		const FVector E3 = Canvas->Project(Ends[i]);
		if (S3.Z <= 0.f || E3.Z <= 0.f) continue;

		// Робимо лінії прицілу пунктирними/прозорими ближче до корабля
		FCanvasLineItem L(FVector2D(S3.X, S3.Y), FVector2D(E3.X, E3.Y));
		L.SetColor(ArcColor);
		L.LineThickness = 1.5f;
		Canvas->DrawItem(L);
	}

	for (const FVector& Impact : ImpactPoints)
	{
		const FVector P0 = Canvas->Project(Impact);
		if (P0.Z <= 0.f) continue;

		float CurrentRingRadius = AimImpactRingRadius * (bReady ? Pulse : 1.0f);
		DrawFilledRect(P0.X - 3.f, P0.Y - 3.f, 6.f, 6.f, AimImpactRingColor); // Точка падіння
	}
}

// -------- CROSSHAIR --------

void ACrownsBaneHUD::DrawCrosshair()
{
	if (!Canvas) return;
	const float CX = Canvas->ClipX * 0.5f;
	const float CY = Canvas->ClipY * 0.5f;

	// Dynamic crosshair: ticks spread wider when sailing, tighten when aiming.
	AShipPawn* Ship = GetPlayerShip();
	const bool bAiming = Ship && Ship->bIsAiming;

	const float OuterOffset = bAiming ? 6.f  : CrosshairSize;        // tighter when aiming
	const float TickLength  = bAiming ? 10.f : CrosshairSize * 0.55f;
	const float Thickness   = bAiming ? 2.f  : 2.f;

	FLinearColor Col = CrosshairColor;
	if (bAiming) { Col.R = 1.f; Col.G = 0.95f; Col.B = 0.3f; Col.A = 1.f; } // gold reticle in aim mode

	// 4 tick marks
	DrawFilledRect(CX - OuterOffset - TickLength, CY - Thickness * 0.5f, TickLength, Thickness, Col);
	DrawFilledRect(CX + OuterOffset,              CY - Thickness * 0.5f, TickLength, Thickness, Col);
	DrawFilledRect(CX - Thickness * 0.5f, CY - OuterOffset - TickLength, Thickness, TickLength, Col);
	DrawFilledRect(CX - Thickness * 0.5f, CY + OuterOffset,              Thickness, TickLength, Col);

	// Centre dot only in aim mode
	if (bAiming)
	{
		DrawFilledRect(CX - 2.f, CY - 2.f, 4.f, 4.f, Col);
	}
}

// -------- DAMAGE FLASH (player took hit) --------

void ACrownsBaneHUD::TriggerDamageFlash(float Intensity)
{
	DamageFlashIntensity = FMath::Max(DamageFlashIntensity, FMath::Clamp(Intensity, 0.0f, 1.0f));
	DamageFlashTimer = DamageFlashDuration;
}

void ACrownsBaneHUD::DrawDamageFlash(float DeltaTime)
{
	if (DamageFlashTimer <= 0.0f || !Canvas) return;

	DamageFlashTimer -= DeltaTime;
	if (DamageFlashTimer <= 0.0f)
	{
		DamageFlashTimer = 0.0f;
		DamageFlashIntensity = 0.0f;
		return;
	}

	const float Alpha = (DamageFlashTimer / DamageFlashDuration) * DamageFlashIntensity * 0.65f;
	const float SW = Canvas->SizeX;
	const float SH = Canvas->SizeY;

	// Edge vignette — heavy on the borders, transparent in the center.
	// Implement as four trapezoidal rectangles on the screen edges.
	const float BorderThickness = SH * 0.18f;
	const FLinearColor Edge(0.85f, 0.05f, 0.05f, Alpha);

	// Top
	DrawFilledRect(0, 0, SW, BorderThickness * 0.7f,         FLinearColor(Edge.R, Edge.G, Edge.B, Alpha * 0.55f));
	// Bottom
	DrawFilledRect(0, SH - BorderThickness * 0.7f, SW, BorderThickness * 0.7f, FLinearColor(Edge.R, Edge.G, Edge.B, Alpha * 0.55f));
	// Left
	DrawFilledRect(0, 0, BorderThickness, SH,                FLinearColor(Edge.R, Edge.G, Edge.B, Alpha * 0.75f));
	// Right
	DrawFilledRect(SW - BorderThickness, 0, BorderThickness, SH, FLinearColor(Edge.R, Edge.G, Edge.B, Alpha * 0.75f));
}

// -------- HIT MARKER (player damaged enemy) --------

void ACrownsBaneHUD::TriggerHitMarker(bool bCritical)
{
	HitMarkerTimer = HitMarkerDuration;
	bHitMarkerCritical = bCritical;
}

void ACrownsBaneHUD::DrawHitMarker(float DeltaTime)
{
	if (HitMarkerTimer <= 0.0f || !Canvas) return;

	HitMarkerTimer -= DeltaTime;
	if (HitMarkerTimer <= 0.0f) { HitMarkerTimer = 0.0f; return; }

	const float Frac  = HitMarkerTimer / HitMarkerDuration;
	const float Alpha = FMath::Clamp(Frac * 1.5f, 0.0f, 1.0f);

	const float CX = Canvas->SizeX * 0.5f;
	const float CY = Canvas->SizeY * 0.5f;

	const FLinearColor Tint = bHitMarkerCritical
		? FLinearColor(1.0f, 0.85f, 0.1f, Alpha)        // gold for critical
		: FLinearColor(1.0f, 1.0f, 1.0f, Alpha);        // white for normal

	// "X" mark drawn as four short rects offset diagonally from center.
	const float Inner = 8.0f;
	const float Len   = 14.0f;
	const float Thick = 2.0f;

	// Top-left diagonal
	DrawFilledRect(CX - Inner - Len, CY - Inner - Len, Len, Thick, Tint);
	DrawFilledRect(CX - Inner - Len, CY - Inner - Len, Thick, Len, Tint);
	// Top-right
	DrawFilledRect(CX + Inner,        CY - Inner - Len, Len, Thick, Tint);
	DrawFilledRect(CX + Inner + Len - Thick, CY - Inner - Len, Thick, Len, Tint);
	// Bottom-left
	DrawFilledRect(CX - Inner - Len, CY + Inner + Len - Thick, Len, Thick, Tint);
	DrawFilledRect(CX - Inner - Len, CY + Inner, Thick, Len, Tint);
	// Bottom-right
	DrawFilledRect(CX + Inner,        CY + Inner + Len - Thick, Len, Thick, Tint);
	DrawFilledRect(CX + Inner + Len - Thick, CY + Inner, Thick, Len, Tint);
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

		E.WorldLocation.Z += E.VerticalSpeed * DeltaTime;
		const FVector Screen = Canvas->Project(E.WorldLocation);
		if (Screen.Z <= 0.f) continue;

		// Ефект "вибуху" (pop-up) в перші 0.2 секунди
		float LifeTime = 1.4f - E.TimeRemaining;
		float PopScale = (LifeTime < 0.15f) ? FMath::Lerp(0.5f, 1.5f, LifeTime / 0.15f) : FMath::Lerp(1.5f, 0.9f, (LifeTime - 0.15f) / 1.25f);

		const float Alpha = FMath::Clamp(E.TimeRemaining / 0.8f, 0.f, 1.f);
		FColor Tint = E.Tint;
		Tint.A = (uint8)(Alpha * 255.f);

		const FString Text = FString::Printf(TEXT("-%.0f"), E.Damage);
		DrawTextWithShadow(Text, Tint, Screen.X - 14.f, Screen.Y, nullptr, PopScale);
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

	// Moved to top-left so it doesn't fight the wanted stars + treasure compass for the
	// top-center band.  Sits above the gold/wood/metal resource panel.
	const float X = HUDPaddingX;
	const float Y = HUDPaddingY;
	DrawPanel(X, Y, 130.f, 28.f, (uint8)CrownStyle::EPanelStyle::Subtle);
	DrawText(Clock, Tint, X + CrownStyle::Sp2, Y + 6.f, nullptr, 1.05f, false);
}

// -------- QUEST LOG OVERLAY (J key) --------

void ACrownsBaneHUD::DrawActiveQuestTracker(ATreasureQuestManager* Mgr, AShipPawn* Ship)
{
	if (!Mgr || !Ship || !Canvas) return;

	const TArray<FTreasureQuest>& Quests = Mgr->GetActiveQuests();
	if (Quests.Num() == 0) return;

	FVector Closest; float Dist;
	if (!Mgr->GetNearestActiveQuestLocation(Ship->GetActorLocation(), Closest, Dist)) return;

	// Find the matching quest name for the closest chest.
	FString QuestName = TEXT("Treasure Hunt");
	float Best = TNumericLimits<float>::Max();
	for (const FTreasureQuest& Q : Quests)
	{
		const float D = FVector::Dist(Q.ChestLocation, Ship->GetActorLocation());
		if (D < Best) { Best = D; QuestName = Q.QuestName.IsEmpty() ? TEXT("Treasure Hunt") : Q.QuestName.ToString(); }
	}

	const float W = 320.f;
	const float H = 78.f;
	const float X = HUDPaddingX;
	const float Y = Canvas->ClipY - H - 220.f; // sits above the wind/storm panels

	// Solid styled panel using UI-A helpers — opaque background so text is always legible.
	DrawFilledRect(X, Y, W, H, FLinearColor(0.04f, 0.05f, 0.08f, 0.92f));
	DrawBorderedRect(X, Y, W, H, FLinearColor(0,0,0,0), CrownStyle::AccentGold, 2.0f);
	// Left accent stripe — gold band as a visual anchor.
	DrawFilledRect(X, Y, 4.f, H, CrownStyle::AccentGold);

	// Bearing arrow indicator on the right.
	const FVector ToTarget = (Closest - Ship->GetActorLocation()).GetSafeNormal2D();
	const FVector ShipFwd = Ship->GetActorForwardVector();
	float RelYaw = FMath::RadiansToDegrees(FMath::Atan2(
		FVector::CrossProduct(ShipFwd, ToTarget).Z,
		FVector::DotProduct(ShipFwd, ToTarget)));
	DrawArrow(X + W - 30.f, Y + 28.f, 14.f, RelYaw - 90.f, CrownStyle::AccentGold);

	const FString DistStr = FString::Printf(TEXT("%.0f m"), Dist * 0.01f);

	// Header label with subtle dim, then big quest name, then distance + counter row.
	DrawText(TEXT("ACTIVE QUEST"), CrownStyle::TextDim,
		X + 14.f, Y + 8.f, nullptr, CrownStyle::ScaleCaption, false);
	DrawText(QuestName, CrownStyle::AccentGold.ToFColor(true),
		X + 14.f, Y + 26.f, nullptr, CrownStyle::ScaleHeading, false);

	DrawText(DistStr, CrownStyle::TextPrimary,
		X + 14.f, Y + 54.f, nullptr, CrownStyle::ScaleBody, false);

	if (Quests.Num() > 1)
	{
		const FString More = FString::Printf(TEXT("+%d more — J"), Quests.Num() - 1);
		DrawText(More, CrownStyle::TextSecondary, X + 100.f, Y + 56.f, nullptr, CrownStyle::ScaleCaption, false);
	}
}

void ACrownsBaneHUD::DrawQuestLog(ATreasureQuestManager* Mgr, AShipPawn* Ship)
{
	if (!Mgr || !Ship || !Canvas) return;

	const float SW = Canvas->ClipX;
	const float SH = Canvas->ClipY;

	const float PanelW = FMath::Min(640.f, SW * 0.55f);
	const float PanelH = FMath::Min(420.f, SH * 0.6f);
	const float PanelX = (SW - PanelW) * 0.5f;
	const float PanelY = (SH - PanelH) * 0.5f;

	// Background dimmer for the full screen behind the panel
	DrawFilledRect(0, 0, SW, SH, FLinearColor(0.0f, 0.0f, 0.0f, 0.45f));
	DrawFilledRect(PanelX, PanelY, PanelW, PanelH, FLinearColor(0.06f, 0.05f, 0.04f, 0.92f));
	DrawBorderedRect(PanelX, PanelY, PanelW, PanelH, FLinearColor(0,0,0,0),
		FLinearColor(0.9f, 0.75f, 0.35f, 1.0f), 2.0f);

	DrawText(TEXT("✦  QUEST LOG  ✦"), FColor(255, 230, 140), PanelX + 16.f, PanelY + 12.f, nullptr, 1.4f, false);
	DrawText(TEXT("Press J to close"), FColor(170, 170, 170), PanelX + PanelW - 150.f, PanelY + 18.f, nullptr, 0.9f, false);

	// Horizontal separator
	DrawFilledRect(PanelX + 16.f, PanelY + 44.f, PanelW - 32.f, 1.f, FLinearColor(0.6f, 0.5f, 0.25f, 0.7f));

	const TArray<FTreasureQuest>& Quests = Mgr->GetActiveQuests();
	if (Quests.Num() == 0)
	{
		DrawText(TEXT("No active quests."), FColor(200, 200, 200), PanelX + 24.f, PanelY + 70.f, nullptr, 1.1f, false);
		DrawText(TEXT("Loot Treasure Maps from sunken enemies to start a hunt."),
			FColor(160, 160, 160), PanelX + 24.f, PanelY + 96.f, nullptr, 0.95f, false);
		return;
	}

	const FVector PlayerLoc = Ship->GetActorLocation();
	float Y = PanelY + 60.f;
	const float RowH = 64.f;

	for (int32 i = 0; i < Quests.Num(); ++i)
	{
		const FTreasureQuest& Q = Quests[i];
		const float Dist = FVector::Dist(Q.ChestLocation, PlayerLoc);
		const FString Name = Q.QuestName.IsEmpty() ? TEXT("Treasure Hunt") : Q.QuestName.ToString();
		const FString DistStr = FString::Printf(TEXT("%.0f m"), Dist * 0.01f);

		// Bearing for cardinal hint
		const FVector Dir2D = (Q.ChestLocation - PlayerLoc).GetSafeNormal2D();
		const float Bearing = FMath::RadiansToDegrees(FMath::Atan2(Dir2D.Y, Dir2D.X));
		FString CardinalStr;
		const float B = FMath::Fmod(Bearing + 360.f, 360.f);
		if      (B < 22.5f  || B >= 337.5f) CardinalStr = TEXT("E");
		else if (B < 67.5f)                 CardinalStr = TEXT("SE");
		else if (B < 112.5f)                CardinalStr = TEXT("S");
		else if (B < 157.5f)                CardinalStr = TEXT("SW");
		else if (B < 202.5f)                CardinalStr = TEXT("W");
		else if (B < 247.5f)                CardinalStr = TEXT("NW");
		else if (B < 292.5f)                CardinalStr = TEXT("N");
		else                                CardinalStr = TEXT("NE");

		DrawFilledRect(PanelX + 16.f, Y, PanelW - 32.f, RowH - 6.f, FLinearColor(0.10f, 0.08f, 0.04f, 0.55f));
		DrawText(FString::Printf(TEXT("%d. %s"), i + 1, *Name),
			FColor(255, 235, 150), PanelX + 26.f, Y + 6.f,  nullptr, 1.1f, false);
		DrawText(FString::Printf(TEXT("Heading %s — %s"), *CardinalStr, *DistStr),
			FColor(210, 210, 210), PanelX + 26.f, Y + 28.f, nullptr, 0.95f, false);

		Y += RowH;
		if (Y > PanelY + PanelH - 30.f) break;
	}

	DrawText(FString::Printf(TEXT("%d / %d active"), Quests.Num(), Mgr->MaxActiveQuests),
		FColor(180, 180, 180), PanelX + 16.f, PanelY + PanelH - 24.f, nullptr, 0.9f, false);

	// Bounty section — render below treasure list if a BountyManager exists in the level.
	TArray<AActor*> BountyFound;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABountyManager::StaticClass(), BountyFound);
	if (BountyFound.Num() == 0) return;
	ABountyManager* BM = Cast<ABountyManager>(BountyFound[0]);
	if (!BM) return;

	const TArray<FBountyQuest>& Bounties = BM->GetActiveBounties();
	if (Bounties.Num() == 0) return;

	DrawText(TEXT("◆  BOUNTIES  ◆"), FColor(255, 180, 100),
		PanelX + 16.f, Y + 8.f, nullptr, 1.05f, false);
	Y += 30.f;

	for (int32 i = 0; i < Bounties.Num(); ++i)
	{
		if (Y > PanelY + PanelH - 30.f) break;
		const FBountyQuest& B = Bounties[i];
		DrawFilledRect(PanelX + 16.f, Y, PanelW - 32.f, RowH - 6.f, FLinearColor(0.14f, 0.08f, 0.04f, 0.55f));
		DrawText(FString::Printf(TEXT("%s"), *B.Title),
			FColor(255, 220, 140), PanelX + 26.f, Y + 6.f, nullptr, 1.05f, false);
		DrawText(FString::Printf(TEXT("Kills %d / %d   Reward %dg"),
				B.CurrentKills, B.TargetKills, B.RewardGold),
			FColor(210, 210, 210), PanelX + 26.f, Y + 28.f, nullptr, 0.95f, false);
		Y += RowH;
	}
}

// -------- UPGRADE MENU OVERLAY (U key while docked) --------

void ACrownsBaneHUD::DrawUpgradeMenu(ACrownsBanePlayerController* PC, AUpgradeManager* Mgr, UPlayerInventory* Inv)
{
	if (!PC || !Canvas) return;

	const float SW = Canvas->ClipX;
	const float SH = Canvas->ClipY;

	// Dim background
	DrawFilledRect(0, 0, SW, SH, FLinearColor(0.0f, 0.0f, 0.0f, 0.55f));

	const float PanelW = FMath::Min(720.f, SW * 0.65f);
	const float PanelH = FMath::Min(540.f, SH * 0.78f);
	const float PanelX = (SW - PanelW) * 0.5f;
	const float PanelY = (SH - PanelH) * 0.5f;

	DrawFilledRect(PanelX, PanelY, PanelW, PanelH, FLinearColor(0.05f, 0.06f, 0.08f, 0.95f));
	DrawBorderedRect(PanelX, PanelY, PanelW, PanelH, FLinearColor(0,0,0,0),
		FLinearColor(0.8f, 0.6f, 0.2f, 1.0f), 2.5f);

	DrawText(TEXT("⚒  SHIPWRIGHT  ⚒"), FColor(255, 220, 100), PanelX + 18.f, PanelY + 12.f, nullptr, 1.5f, false);
	DrawText(TEXT("Press U to close   |   Number keys 1-7 to purchase"),
		FColor(180, 180, 180), PanelX + 18.f, PanelY + 44.f, nullptr, 0.95f, false);

	// Inventory readout
	const FString InvLine = Inv
		? FString::Printf(TEXT("Gold %d   Wood %d   Metal %d"), Inv->GetGold(), Inv->GetWood(), Inv->GetMetal())
		: TEXT("Gold 0   Wood 0   Metal 0");
	DrawText(InvLine, FColor(255, 215, 90), PanelX + PanelW - 320.f, PanelY + 44.f, nullptr, 0.95f, false);

	DrawFilledRect(PanelX + 16.f, PanelY + 72.f, PanelW - 32.f, 1.f, FLinearColor(0.6f, 0.5f, 0.25f, 0.7f));

	struct FRow { EUpgradeCategory Cat; const TCHAR* Label; const TCHAR* Hotkey; };
	static const FRow Rows[] = {
		{ EUpgradeCategory::Hull,          TEXT("Hull (Health)"),     TEXT("[1]") },
		{ EUpgradeCategory::Sails,         TEXT("Sails (Speed)"),     TEXT("[2]") },
		{ EUpgradeCategory::Weapons,       TEXT("Weapons (Damage)"),  TEXT("[3]") },
		{ EUpgradeCategory::CannonCount,   TEXT("Cannon Count"),      TEXT("[4]") },
		{ EUpgradeCategory::CargoHold,     TEXT("Cargo Hold"),        TEXT("[5]") },
		{ EUpgradeCategory::AmmoCapacity,  TEXT("Ammo Capacity"),     TEXT("[6]") },
		{ EUpgradeCategory::HullArmor,     TEXT("Hull Armor"),        TEXT("[7]") }
	};

	const float RowH = 56.f;
	float Y = PanelY + 86.f;

	for (const FRow& R : Rows)
	{
		const int32 Tier = Mgr ? Mgr->GetCurrentTier(R.Cat) : 0;
		const int32 MaxT = Mgr ? Mgr->GetMaxTier(R.Cat)    : 4;

		FUpgradeLevel Next;
		const bool bHasNext = Mgr ? Mgr->GetNextUpgradeData(R.Cat, Next) : false;
		const bool bAfford  = (Mgr && Inv) ? Mgr->CanAffordNextUpgrade(R.Cat, Inv) : false;
		const bool bMaxed   = Tier >= MaxT;

		FLinearColor RowBg = bMaxed
			? FLinearColor(0.06f, 0.14f, 0.06f, 0.6f)
			: (bAfford ? FLinearColor(0.10f, 0.10f, 0.05f, 0.55f) : FLinearColor(0.14f, 0.06f, 0.06f, 0.55f));
		DrawFilledRect(PanelX + 16.f, Y, PanelW - 32.f, RowH - 6.f, RowBg);

		DrawText(R.Hotkey, FColor(255, 220, 100), PanelX + 24.f, Y + 6.f, nullptr, 1.1f, false);
		DrawText(R.Label,  FColor(240, 240, 240), PanelX + 70.f, Y + 6.f, nullptr, 1.1f, false);

		// Tier dots — filled = purchased, empty = locked
		for (int32 t = 0; t < MaxT; ++t)
		{
			const float DotX = PanelX + 70.f + 280.f + t * 22.f;
			const float DotY = Y + 12.f;
			const FLinearColor Col = (t < Tier)
				? FLinearColor(1.0f, 0.85f, 0.2f, 1.0f)
				: FLinearColor(0.3f, 0.3f, 0.3f, 0.85f);
			DrawFilledRect(DotX, DotY, 14.f, 14.f, Col);
		}

		if (bMaxed)
		{
			DrawText(TEXT("MAX"), FColor(120, 230, 120), PanelX + 70.f, Y + 30.f, nullptr, 0.95f, false);
		}
		else if (bHasNext)
		{
			FString CostStr;
			if (Next.GoldCost  > 0) CostStr += FString::Printf(TEXT("%dg "),   Next.GoldCost);
			if (Next.WoodCost  > 0) CostStr += FString::Printf(TEXT("%dw "),  Next.WoodCost);
			if (Next.MetalCost > 0) CostStr += FString::Printf(TEXT("%dm "),  Next.MetalCost);

			const FColor CostColor = bAfford ? FColor(230, 220, 160) : FColor(220, 140, 140);
			DrawText(CostStr, CostColor, PanelX + 70.f, Y + 30.f, nullptr, 0.95f, false);

			if (!Next.Description.IsEmpty())
			{
				DrawText(Next.Description, FColor(180, 180, 180),
					PanelX + 70.f + 150.f, Y + 30.f, nullptr, 0.9f, false);
			}
		}

		Y += RowH;
	}

	DrawText(TEXT("(Must be docked to purchase)"), FColor(160, 160, 160),
		PanelX + 18.f, PanelY + PanelH - 30.f, nullptr, 0.9f, false);
}

// -------- BOARDING PROMPT + QTE --------
void ACrownsBaneHUD::DrawBoardingPrompt(AShipPawn* Ship)
{
	if (!Ship || !Canvas) return;

	const float SW = Canvas->ClipX;
	const float SH = Canvas->ClipY;

	if (Ship->bBoardingActive)
	{
		const float BW = 460.f;
		const float BH = 84.f;
		const float BX = (SW - BW) * 0.5f;
		const float BY = SH * 0.55f;

		DrawFilledRect(BX, BY, BW, BH, FLinearColor(0.10f, 0.05f, 0.02f, 0.92f));
		DrawBorderedRect(BX, BY, BW, BH, FLinearColor(0,0,0,0), FLinearColor(1.0f, 0.4f, 0.1f, 1.0f), 2.5f);
		DrawText(TEXT("BOARDING — MASH [SPACE]"), FColor(255, 220, 100), BX + 18.f, BY + 8.f, nullptr, 1.25f, false);

		const float HitFrac = (Ship->BoardingQTERequiredHits > 0)
			? FMath::Clamp((float)Ship->BoardingQTEHits / (float)Ship->BoardingQTERequiredHits, 0.f, 1.f)
			: 0.f;
		const float BarX = BX + 18.f;
		const float BarY = BY + 38.f;
		const float BarW = BW - 36.f;
		const float BarH = 14.f;
		DrawFilledRect(BarX, BarY, BarW, BarH, FLinearColor(0.2f, 0.15f, 0.05f, 0.9f));
		DrawFilledRect(BarX, BarY, BarW * HitFrac, BarH, FLinearColor(1.0f, 0.7f, 0.15f, 1.0f));
		DrawText(FString::Printf(TEXT("%d / %d   |   %.1fs"),
				Ship->BoardingQTEHits, Ship->BoardingQTERequiredHits,
				FMath::Max(0.f, Ship->BoardingQTETimeRemaining)),
			FColor(255, 230, 180), BarX, BarY + BarH + 4.f, nullptr, 0.9f, false);
		return;
	}

	if (!Ship->CurrentBoardingTarget) return;

	const float W = 320.f;
	const float H = 64.f;
	const float X = (SW - W) * 0.5f;
	const float Y = SH * 0.62f;

	DrawFilledRect(X, Y, W, H, FLinearColor(0.10f, 0.04f, 0.04f, 0.85f));
	DrawBorderedRect(X, Y, W, H, FLinearColor(0,0,0,0), FLinearColor(1.0f, 0.55f, 0.1f, 1.0f), 2.0f);
	DrawText(TEXT("[F] BOARD"), FColor(255, 200, 80), X + 22.f, Y + 8.f,  nullptr, 1.4f, false);
	DrawText(TEXT("Enemy is crippled — board for bonus loot"),
		FColor(220, 220, 220), X + 22.f, Y + 38.f, nullptr, 0.9f, false);
}

// -------- TRADER MENU OVERLAY (T key while docked) --------

void ACrownsBaneHUD::DrawTraderMenu(ACrownsBanePlayerController* PC, UPlayerInventory* Inv)
{
	if (!PC || !Canvas) return;

	const float SW = Canvas->ClipX;
	const float SH = Canvas->ClipY;
	DrawFilledRect(0, 0, SW, SH, FLinearColor(0.0f, 0.0f, 0.0f, 0.60f));

	const float PW = FMath::Min(740.f, SW * 0.65f);
	const float PH = FMath::Min(560.f, SH * 0.75f);
	const float PX = (SW - PW) * 0.5f;
	const float PY = (SH - PH) * 0.5f;

	DrawPanel(PX, PY, PW, PH, (uint8)CrownStyle::EPanelStyle::Highlight);

	// Dock context — type + name + price multipliers.
	EDockType DType = EDockType::Merchant;
	FString DName = TEXT("");
	if (PC->CurrentDocksZone)
	{
		DType = PC->CurrentDocksZone->DockType;
		DName = PC->CurrentDocksZone->DockName;
	}
	const TCHAR* TypeStr = (DType == EDockType::Naval) ? TEXT("NAVAL")
		: (DType == EDockType::PirateHaven) ? TEXT("PIRATE") : TEXT("MERCHANT");
	const FLinearColor TypeTint = (DType == EDockType::Naval)
		? FLinearColor(0.5f, 0.7f, 1.0f, 1.0f)
		: (DType == EDockType::PirateHaven) ? FLinearColor(1.0f, 0.5f, 0.3f, 1.0f)
		: CrownStyle::AccentGold;

	DrawText(TEXT("⚓  TRADER"), FColor(180, 255, 160), PX + CrownStyle::Sp3, PY + 14.f, nullptr, 1.5f, false);
	DrawText(FString::Printf(TEXT("%s  •  %s"), TypeStr, *DName), TypeTint.ToFColor(true),
		PX + 180.f, PY + 22.f, nullptr, 1.05f, false);
	DrawCaption(TEXT("T to close   |   1-6 to trade"), PX + CrownStyle::Sp3, PY + 52.f);

	const FString InvLine = Inv
		? FString::Printf(TEXT("%d g    %d wood    %d metal    %d/%d ammo"),
				Inv->GetGold(), Inv->GetWood(), Inv->GetMetal(),
				Inv->GetAmmo(), Inv->GetMaxAmmo())
		: TEXT("");
	DrawText(InvLine, FColor(255, 215, 90), PX + PW - 320.f, PY + 22.f, nullptr, 0.95f, false);

	DrawFilledRect(PX + CrownStyle::Sp3, PY + 80.f, PW - CrownStyle::Sp4, 1.f,
		CrownStyle::AccentGold * FLinearColor(1,1,1,0.5f));

	// Compute effective prices for display.
	const float BuyMul  = (DType == EDockType::PirateHaven) ? 0.70f : (DType == EDockType::Naval ? 1.20f : 1.00f);
	const float SellMul = (DType == EDockType::Merchant)    ? 1.25f : (DType == EDockType::Naval ? 0.70f : 1.00f);
	auto BuyPrice  = [&](int32 Base) { return FMath::Max(1, FMath::RoundToInt(Base * BuyMul)); };
	auto SellPayout = [&](int32 Base) { return FMath::Max(1, FMath::RoundToInt(Base * SellMul)); };

	struct FRow { const TCHAR* Key; const TCHAR* Label; const TCHAR* Desc; int32 Price; bool bSell; };
	const FRow Rows[] = {
		{ TEXT("1"), TEXT("Buy ammo"),    TEXT("+10 cannonballs"),   BuyPrice(PC->BuyAmmoPrice),   false },
		{ TEXT("2"), TEXT("Buy wood"),    TEXT("+10 wood for repair"), BuyPrice(PC->BuyWoodPrice),  false },
		{ TEXT("3"), TEXT("Buy metal"),   TEXT("+5 metal for armor"),  BuyPrice(PC->BuyMetalPrice), false },
		{ TEXT("4"), TEXT("Sell wood"),   TEXT("Trade 10 wood"),      SellPayout(PC->SellWoodPrice), true },
		{ TEXT("5"), TEXT("Sell metal"),  TEXT("Trade 5 metal"),      SellPayout(PC->SellMetalPrice), true },
		{ TEXT("6"), TEXT("Full repair"), TEXT("Restore hull HP"),    BuyPrice(PC->HealCost),       false },
	};

	float Y = PY + 96.f;
	const float RowH = 62.f;
	for (int32 i = 0; i < UE_ARRAY_COUNT(Rows); ++i)
	{
		const FRow& R = Rows[i];
		DrawFilledRect(PX + CrownStyle::Sp3, Y, PW - CrownStyle::Sp4, RowH - 8.f,
			FLinearColor(0.08f, 0.08f, 0.04f, 0.65f));

		// Number badge
		DrawFilledRect(PX + 24.f, Y + 8.f, 32.f, 32.f, CrownStyle::AccentGold * FLinearColor(1,1,1,0.8f));
		DrawText(R.Key, FColor(20, 20, 10), PX + 35.f, Y + 12.f, nullptr, 1.25f, false);

		DrawText(R.Label, CrownStyle::TextPrimary, PX + 70.f, Y + 6.f, nullptr, 1.15f, false);
		DrawText(R.Desc,  CrownStyle::TextDim,     PX + 70.f, Y + 28.f, nullptr, 0.9f, false);

		const FString CostStr = R.bSell
			? FString::Printf(TEXT("+%d g"), R.Price)
			: FString::Printf(TEXT("%d g"),  R.Price);
		const FColor PriceCol = R.bSell ? FColor(140, 230, 150) : FColor(255, 220, 140);
		DrawText(CostStr, PriceCol, PX + PW - 110.f, Y + 14.f, nullptr, 1.25f, false);

		Y += RowH;
	}

	const FString Hint = (DType == EDockType::Merchant)    ? TEXT("Best sell rates here.")
		: (DType == EDockType::PirateHaven) ? TEXT("Discount on goods, but pirate shipwrights overcharge upgrades.")
		: TEXT("Naval port — premium prices, no questions asked.");
	DrawCaption(Hint, PX + CrownStyle::Sp3, PY + PH - 28.f);
}

// -------- LOCK-ON RETICLE --------

void ACrownsBaneHUD::DrawLockOnReticle(AShipPawn* Ship)
{
	if (!Ship || !Ship->LockedTarget || !Canvas) return;
	AEnemyShipBase* T = Ship->LockedTarget;
	if (!T->HealthComponent || !T->HealthComponent->IsAlive()) return;

	const FVector ProjVec = Project(T->GetActorLocation());
	if (ProjVec.Z <= 0.0f) return; // behind camera
	const FVector2D ScreenPos(ProjVec.X, ProjVec.Y);

	const float R = 40.f;
	const FLinearColor Tint(1.0f, 0.25f, 0.25f, 0.95f);

	// 4 corner brackets around the target
	const float L = 10.f;
	const float Th = 3.f;
	// TL
	DrawFilledRect(ScreenPos.X - R,        ScreenPos.Y - R,        L, Th, Tint);
	DrawFilledRect(ScreenPos.X - R,        ScreenPos.Y - R,        Th, L, Tint);
	// TR
	DrawFilledRect(ScreenPos.X + R - L,    ScreenPos.Y - R,        L, Th, Tint);
	DrawFilledRect(ScreenPos.X + R - Th,   ScreenPos.Y - R,        Th, L, Tint);
	// BL
	DrawFilledRect(ScreenPos.X - R,        ScreenPos.Y + R - Th,   L, Th, Tint);
	DrawFilledRect(ScreenPos.X - R,        ScreenPos.Y + R - L,    Th, L, Tint);
	// BR
	DrawFilledRect(ScreenPos.X + R - L,    ScreenPos.Y + R - Th,   L, Th, Tint);
	DrawFilledRect(ScreenPos.X + R - Th,   ScreenPos.Y + R - L,    Th, L, Tint);

	// Distance label
	const float Dist = FVector::Dist(Ship->GetActorLocation(), T->GetActorLocation()) * 0.01f;
	DrawText(FString::Printf(TEXT("LOCK  %.0fm"), Dist),
		FColor(255, 120, 120), ScreenPos.X - R, ScreenPos.Y + R + 4.f, nullptr, 0.95f, false);
}

// -------- LEAD INDICATOR (predicted impact point for moving target) --------

void ACrownsBaneHUD::DrawLeadIndicator(AShipPawn* Ship)
{
	if (!Ship || !Ship->LockedTarget || !Ship->CannonComponent || !Canvas) return;
	AEnemyShipBase* T = Ship->LockedTarget;
	if (!T->HealthComponent || !T->HealthComponent->IsAlive()) return;

	// Estimate time-of-flight: distance / initial cannonball speed.  Good enough
	// for an aim assist that doesn't lie when the target stands still.
	const FVector ShipLoc   = Ship->GetActorLocation();
	const FVector TargetLoc = T->GetActorLocation();
	const float   Dist      = FVector::Dist(ShipLoc, TargetLoc);
	const float   ProjSpeed = 3000.0f;
	const float   TOF       = Dist / FMath::Max(1.0f, ProjSpeed);

	// Predicted velocity = current actor velocity (works for AI ships using AddActorWorldOffset).
	const FVector TargetVel = T->GetVelocity();
	const FVector PredictedLoc = TargetLoc + TargetVel * TOF;

	const FVector ProjVec = Project(PredictedLoc);
	if (ProjVec.Z <= 0.0f) return; // behind camera
	const FVector2D ScreenPos(ProjVec.X, ProjVec.Y);

	// Crosshair color reflects whether the predicted impact is within cannon range.
	const float Range  = Ship->CannonComponent->MaxRange;
	const float PredD  = FVector::Dist(ShipLoc, PredictedLoc);
	const float RatioR = PredD / FMath::Max(100.0f, Range);
	const FLinearColor Tint =
		(RatioR <= 0.6f)  ? FLinearColor(0.2f, 1.0f, 0.3f, 0.95f) :   // green: comfortable range
		(RatioR <= 1.0f)  ? FLinearColor(1.0f, 0.85f, 0.2f, 0.95f) :  // yellow: edge of range
		                    FLinearColor(1.0f, 0.25f, 0.25f, 0.9f);   // red: out of range

	// Diamond marker at predicted impact: 4 small rectangles forming a rotated square.
	const float S = 6.f;
	DrawFilledRect(ScreenPos.X - S, ScreenPos.Y - 1.f, S * 2.f, 2.f, Tint);
	DrawFilledRect(ScreenPos.X - 1.f, ScreenPos.Y - S, 2.f, S * 2.f, Tint);

	// Range readout
	DrawText(FString::Printf(TEXT("%.0fm"), PredD * 0.01f),
		Tint.ToFColor(true), ScreenPos.X + 10.f, ScreenPos.Y - 8.f, nullptr, 0.85f, false);
}

// ============== UNIFIED STYLE HELPERS (UI-A) ==============

void ACrownsBaneHUD::DrawPanel(float X, float Y, float W, float H, uint8 Style)
{
	const CrownStyle::EPanelStyle S = static_cast<CrownStyle::EPanelStyle>(Style);
	const FLinearColor Bg = (S == CrownStyle::EPanelStyle::Subtle) ? CrownStyle::BgLight : CrownStyle::BgDark;
	DrawFilledRect(X, Y, W, H, Bg);

	const FLinearColor Border = CrownStyle::BorderForStyle(S);
	if (Border.A > 0.01f)
	{
		const float T = CrownStyle::ThicknessForStyle(S);
		DrawBorderedRect(X, Y, W, H, CrownStyle::BgTransparent, Border, T);
	}

	// Subtle highlight stripe along the top for "lit from above" feel.
	if (S != CrownStyle::EPanelStyle::Subtle)
	{
		DrawFilledRect(X + 2.f, Y + 2.f, W - 4.f, 1.f, FLinearColor(1.0f, 1.0f, 1.0f, 0.06f));
	}
}

void ACrownsBaneHUD::DrawBody(const FString& Text, float X, float Y, float Scale)
{
	DrawText(Text, CrownStyle::TextPrimary, X, Y, nullptr, CrownStyle::ScaleBody * Scale, false);
}

void ACrownsBaneHUD::DrawHeading(const FString& Text, float X, float Y)
{
	DrawText(Text, CrownStyle::TextGold, X, Y, nullptr, CrownStyle::ScaleHeading, false);
}

void ACrownsBaneHUD::DrawCaption(const FString& Text, float X, float Y)
{
	DrawText(Text, CrownStyle::TextDim, X, Y, nullptr, CrownStyle::ScaleCaption, false);
}

void ACrownsBaneHUD::DrawSmoothBar(float X, float Y, float W, float H, float Frac, FLinearColor Fill, bool bPulse)
{
	Frac = FMath::Clamp(Frac, 0.0f, 1.0f);

	// Background trough (slightly larger for outline)
	DrawFilledRect(X - 1.f, Y - 1.f, W + 2.f, H + 2.f, FLinearColor(0.05f, 0.05f, 0.05f, 0.95f));

	// Fill
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	const float PulseAdd = bPulse ? (0.20f * CrownStyle::Pulse(Now, 1.2f)) : 0.0f;
	FLinearColor F = Fill;
	F.R = FMath::Clamp(F.R + PulseAdd, 0.f, 1.f);
	F.G = FMath::Clamp(F.G + PulseAdd, 0.f, 1.f);
	F.B = FMath::Clamp(F.B + PulseAdd, 0.f, 1.f);
	DrawFilledRect(X, Y, W * Frac, H, F);

	// Highlight strip (top 1px)
	DrawFilledRect(X, Y, W * Frac, 1.f, FLinearColor(1.0f, 1.0f, 1.0f, 0.4f));
}

// ============== UI-B: RESOURCE TOASTS + LOW-HP WARNING ==============

void ACrownsBaneHUD::PushResourceToast(const FString& Text, FLinearColor Tint)
{
	FResourceToast T;
	T.Text = Text;
	T.Tint = Tint;
	T.TimeRemaining = 1.8f;
	ResourceToasts.Insert(T, 0);
	if (ResourceToasts.Num() > 6) ResourceToasts.SetNum(6);
}

void ACrownsBaneHUD::DrawResourceToasts(float DeltaTime)
{
	if (!Canvas) return;

	// Auto-detect inventory deltas and push a toast for each.
	if (UPlayerInventory* Inv = GetPlayerInventory())
	{
		const int32 G = Inv->GetGold();
		const int32 W = Inv->GetWood();
		const int32 M = Inv->GetMetal();
		if (LastGold  >= 0 && G  != LastGold)  PushResourceToast(FString::Printf(TEXT("%s%d  GOLD"),  G  > LastGold  ? TEXT("+") : TEXT(""), G  - LastGold),  CrownStyle::AccentGold);
		if (LastWood  >= 0 && W  != LastWood)  PushResourceToast(FString::Printf(TEXT("%s%d  WOOD"),  W  > LastWood  ? TEXT("+") : TEXT(""), W  - LastWood),  FLinearColor(0.75f, 0.55f, 0.30f, 1.0f));
		if (LastMetal >= 0 && M  != LastMetal) PushResourceToast(FString::Printf(TEXT("%s%d  METAL"), M  > LastMetal ? TEXT("+") : TEXT(""), M  - LastMetal), CrownStyle::AccentSilver);
		LastGold = G; LastWood = W; LastMetal = M;
	}

	// Render stack, bottom-right, oldest fading first.
	const float BaseX = Canvas->ClipX - 220.f;
	const float BaseY = Canvas->ClipY - 220.f;
	for (int32 i = ResourceToasts.Num() - 1; i >= 0; --i)
	{
		FResourceToast& T = ResourceToasts[i];
		T.TimeRemaining -= DeltaTime;
		if (T.TimeRemaining <= 0.0f) { ResourceToasts.RemoveAt(i); continue; }

		const float Alpha = FMath::Clamp(T.TimeRemaining / 1.8f, 0.0f, 1.0f);
		FLinearColor Tint = T.Tint; Tint.A = Alpha;
		const float Y = BaseY + i * 26.f;
		DrawFilledRect(BaseX, Y, 200.f, 22.f, FLinearColor(0.04f, 0.04f, 0.06f, 0.55f * Alpha));
		DrawText(T.Text, Tint.ToFColor(true), BaseX + 12.f, Y + 4.f, nullptr, 0.95f, false);
	}
}

void ACrownsBaneHUD::DrawLowHealthFlash(AShipPawn* Ship)
{
	if (!Ship || !Ship->HealthComponent || !Canvas) return;
	const float Pct = Ship->HealthComponent->GetHealthPercent();
	if (Pct >= 0.30f) return;

	// Pulse intensity ramps as HP approaches 0.
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	const float UrgencyHz = FMath::Lerp(1.0f, 3.5f, FMath::Clamp(1.0f - Pct / 0.30f, 0.f, 1.f));
	const float A = CrownStyle::Pulse(Now, UrgencyHz) * 0.45f * (1.0f - Pct / 0.30f);

	const float SW = Canvas->ClipX;
	const float SH = Canvas->ClipY;
	const float T = SH * 0.18f;
	const FLinearColor Edge(0.85f, 0.08f, 0.08f, A);
	DrawFilledRect(0, 0, SW, T * 0.7f, Edge);
	DrawFilledRect(0, SH - T * 0.7f, SW, T * 0.7f, Edge);
	DrawFilledRect(0, 0, T * 0.9f, SH, FLinearColor(Edge.R, Edge.G, Edge.B, A * 1.1f));
	DrawFilledRect(SW - T * 0.9f, 0, T * 0.9f, SH, FLinearColor(Edge.R, Edge.G, Edge.B, A * 1.1f));
}

// ============== UI-D: COMBAT HUD ==============

void ACrownsBaneHUD::RegisterPlayerDamageFrom(float WorldYawDegrees)
{
	FDamageMarker M;
	M.WorldYaw = WorldYawDegrees;
	M.TimeRemaining = 0.7f;
	DamageMarkers.Add(M);
	if (DamageMarkers.Num() > 4) DamageMarkers.RemoveAt(0);
}

void ACrownsBaneHUD::RegisterPlayerHit()
{
	CurrentCombo++;
	ComboTimeRemaining = 3.0f; // 3s window between hits to keep combo alive
}

void ACrownsBaneHUD::PushKillFeed(const FString& Text)
{
	FKillFeedEntry E;
	E.Text = Text;
	E.TimeRemaining = 5.0f;
	KillFeed.Insert(E, 0);
	if (KillFeed.Num() > 5) KillFeed.SetNum(5);
}

void ACrownsBaneHUD::DrawEnemyInfoCard(AShipPawn* Ship)
{
	if (!Ship || !Ship->LockedTarget || !Canvas) return;
	AEnemyShipBase* T = Ship->LockedTarget;
	if (!T->HealthComponent || !T->HealthComponent->IsAlive()) return;

	// Identify type from class name (Sloop/Brig/Galleon).
	const FString ClassName = T->GetClass()->GetName();
	FString TypeName = TEXT("Vessel");
	int32 Threat = 1;
	if (ClassName.Contains(TEXT("Sloop")))   { TypeName = TEXT("Sloop");   Threat = 1; }
	else if (ClassName.Contains(TEXT("Brig"))) { TypeName = TEXT("Brig");    Threat = 3; }
	else if (ClassName.Contains(TEXT("Galleon"))) { TypeName = TEXT("Galleon"); Threat = 5; }

	const float W = 260.f;
	const float H = 110.f;
	const float X = Canvas->ClipX - W - CrownStyle::Sp3;
	const float Y = Canvas->ClipY * 0.30f;

	DrawPanel(X, Y, W, H, (uint8)CrownStyle::EPanelStyle::Danger);

	DrawHeading(TypeName, X + CrownStyle::Sp3, Y + CrownStyle::Sp2);

	// HP bar.
	const float HP = T->HealthComponent->GetHealthPercent();
	const FLinearColor HpCol = FLinearColor::LerpUsingHSV(CrownStyle::Danger, CrownStyle::Success, HP);
	DrawSmoothBar(X + CrownStyle::Sp3, Y + CrownStyle::Sp5 + 4.f, W - CrownStyle::Sp4, 10.f, HP, HpCol);
	DrawCaption(FString::Printf(TEXT("HP %.0f / %.0f"), T->HealthComponent->GetCurrentHealth(), T->HealthComponent->GetMaxHealth()),
		X + CrownStyle::Sp3, Y + CrownStyle::Sp5 + 16.f);

	// Threat stars.
	for (int32 i = 0; i < 5; ++i)
	{
		const float StarX = X + W - CrownStyle::Sp3 - (5 - i) * 14.f;
		const float StarY = Y + CrownStyle::Sp2 + 4.f;
		DrawFilledRect(StarX, StarY, 10.f, 10.f,
			(i < Threat) ? CrownStyle::Danger : FLinearColor(0.25f, 0.25f, 0.25f, 0.7f));
	}

	// Distance & bearing.
	const FVector ToTarget = T->GetActorLocation() - Ship->GetActorLocation();
	const float Dist = ToTarget.Size() * 0.01f;
	const float Bearing = FMath::RadiansToDegrees(FMath::Atan2(ToTarget.Y, ToTarget.X));
	const FString B = FString::Printf(TEXT("%.0fm   bearing %.0f°"), Dist, Bearing);
	DrawCaption(B, X + CrownStyle::Sp3, Y + H - CrownStyle::Sp4);
}

void ACrownsBaneHUD::DrawDamageDirection(float DeltaTime)
{
	if (!Canvas || DamageMarkers.Num() == 0) return;

	AShipPawn* Ship = GetPlayerShip();
	if (!Ship) return;

	const float CX = Canvas->ClipX * 0.5f;
	const float CY = Canvas->ClipY * 0.5f;
	const float R  = FMath::Min(Canvas->ClipX, Canvas->ClipY) * 0.4f;

	const float PlayerYaw = Ship->GetActorRotation().Yaw;

	for (int32 i = DamageMarkers.Num() - 1; i >= 0; --i)
	{
		FDamageMarker& M = DamageMarkers[i];
		M.TimeRemaining -= DeltaTime;
		if (M.TimeRemaining <= 0.0f) { DamageMarkers.RemoveAt(i); continue; }

		// Yaw of damage source relative to player forward.
		float Rel = FRotator::NormalizeAxis(M.WorldYaw - PlayerYaw);
		const float Rad = FMath::DegreesToRadians(Rel - 90.f);  // -90 so right=0, up=-90
		const float MX = CX + FMath::Cos(Rad) * R;
		const float MY = CY + FMath::Sin(Rad) * R;

		const float A = FMath::Clamp(M.TimeRemaining / 0.7f, 0.f, 1.f);
		const FLinearColor Tint(0.95f, 0.15f, 0.15f, A * 0.9f);
		// Arc-like triangle: 3 stacked rectangles approximating an arrow head.
		DrawFilledRect(MX - 18.f, MY - 4.f, 36.f, 8.f, Tint);
		DrawFilledRect(MX - 12.f, MY - 8.f, 24.f, 4.f, Tint);
		DrawFilledRect(MX - 6.f,  MY - 12.f, 12.f, 4.f, Tint);
	}
}

void ACrownsBaneHUD::DrawComboCounter(float DeltaTime)
{
	if (!Canvas) return;
	ComboTimeRemaining -= DeltaTime;
	if (ComboTimeRemaining <= 0.0f)
	{
		CurrentCombo = 0;
		return;
	}
	if (CurrentCombo < 2) return; // hide for 1-hit shots

	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	const float Pulse = 1.0f + 0.08f * CrownStyle::Pulse(Now, 2.0f);

	const FString Txt = FString::Printf(TEXT("%d  HITS"), CurrentCombo);
	const FColor C = (CurrentCombo >= 8) ? FColor(255, 80, 60) :
	                 (CurrentCombo >= 5) ? FColor(255, 180, 60) :
	                                       FColor(255, 230, 140);
	const float X = Canvas->ClipX * 0.5f - 60.f;
	const float Y = Canvas->ClipY * 0.30f;
	DrawText(Txt, C, X, Y, nullptr, 1.6f * Pulse, false);
}

void ACrownsBaneHUD::DrawKillFeed(float DeltaTime)
{
	if (!Canvas) return;

	const float X = Canvas->ClipX - 360.f;
	const float Y0 = 120.f;
	for (int32 i = KillFeed.Num() - 1; i >= 0; --i)
	{
		FKillFeedEntry& E = KillFeed[i];
		E.TimeRemaining -= DeltaTime;
		if (E.TimeRemaining <= 0.0f) { KillFeed.RemoveAt(i); continue; }
		const float A = FMath::Clamp(E.TimeRemaining / 5.0f, 0.f, 1.f);
		FColor Col = CrownStyle::TextGold;
		Col.A = (uint8)(A * 255);
		DrawFilledRect(X, Y0 + i * 28.f, 340.f, 24.f, FLinearColor(0.04f, 0.04f, 0.06f, A * 0.6f));
		DrawText(E.Text, Col, X + CrownStyle::Sp2, Y0 + i * 28.f + 4.f, nullptr, 1.0f, false);
	}
}

// ============== UI-G: CINEMATIC BANNERS ==============

void ACrownsBaneHUD::ShowBanner(const FString& Title, const FString& Subtitle, FLinearColor Tint, float Duration)
{
	BannerTitle = Title;
	BannerSubtitle = Subtitle;
	BannerTint = Tint;
	BannerTimeRemaining = Duration;
	BannerInitialDuration = Duration;
}

void ACrownsBaneHUD::ShowMissionComplete(const FString& QuestName, int32 Gold, int32 Wood, int32 Metal)
{
	MissionTitle = QuestName;
	MissionGold = Gold; MissionWood = Wood; MissionMetal = Metal;
	MissionTimeRemaining = 5.0f;
}

void ACrownsBaneHUD::DrawBanner(float DeltaTime)
{
	if (BannerTimeRemaining <= 0.0f || !Canvas) return;
	BannerTimeRemaining -= DeltaTime;

	const float D = FMath::Max(0.1f, BannerInitialDuration);
	const float T = 1.0f - FMath::Clamp(BannerTimeRemaining / D, 0.f, 1.f);

	// Letterbox in for the first 0.2s, hold, then out for last 0.5s.
	const float HoldT = FMath::Clamp(T * 5.0f, 0.f, 1.f);
	const float OutT  = FMath::Clamp((BannerTimeRemaining / D) * 2.0f, 0.f, 1.f);
	const float A     = FMath::Min(HoldT, OutT);

	const float SW = Canvas->ClipX;
	const float SH = Canvas->ClipY;
	const float Bar = SH * 0.12f * A;

	// Letterbox bars
	DrawFilledRect(0, 0, SW, Bar, FLinearColor(0,0,0, 0.85f * A));
	DrawFilledRect(0, SH - Bar, SW, Bar, FLinearColor(0,0,0, 0.85f * A));

	// Title (display scale 2.0) centered.
	const float CY = SH * 0.5f - 30.f;
	const float CX = SW * 0.5f - 200.f;

	FColor TitleCol = BannerTint.ToFColor(true);
	TitleCol.A = (uint8)(A * 255);
	DrawText(BannerTitle, TitleCol, CX, CY, nullptr, 2.0f, false);
	if (!BannerSubtitle.IsEmpty())
	{
		FColor SubCol(220, 220, 220, (uint8)(A * 220));
		DrawText(BannerSubtitle, SubCol, CX, CY + 36.f, nullptr, 1.0f, false);
	}
}

void ACrownsBaneHUD::DrawMissionComplete(float DeltaTime)
{
	if (MissionTimeRemaining <= 0.0f || !Canvas) return;
	MissionTimeRemaining -= DeltaTime;

	const float D = 5.0f;
	const float T = MissionTimeRemaining / D;
	const float A = FMath::Clamp(T * 1.5f, 0.f, 1.f);

	const float SW = Canvas->ClipX;
	const float SH = Canvas->ClipY;
	const float W = 480.f;
	const float H = 240.f;
	const float X = (SW - W) * 0.5f;
	const float Y = SH * 0.30f;

	DrawFilledRect(X, Y, W, H, FLinearColor(0.04f, 0.06f, 0.05f, 0.92f * A));
	DrawBorderedRect(X, Y, W, H, CrownStyle::BgTransparent, FLinearColor(CrownStyle::AccentGold.R, CrownStyle::AccentGold.G, CrownStyle::AccentGold.B, A), 2.5f);

	FColor Gold = CrownStyle::AccentGold.ToFColor(true);  Gold.A  = (uint8)(A * 255);
	FColor Body = CrownStyle::TextPrimary;                Body.A = (uint8)(A * 255);
	FColor Dim  = CrownStyle::TextDim;                    Dim.A   = (uint8)(A * 255);

	DrawText(TEXT("QUEST COMPLETE"), Gold, X + 24.f, Y + 18.f, nullptr, 1.6f, false);
	DrawText(MissionTitle,           Body, X + 24.f, Y + 60.f, nullptr, 1.2f, false);

	// Animated counters: reveal each reward over the first half of duration.
	const float Reveal = FMath::Clamp((1.0f - T) * 2.0f, 0.f, 1.f);
	const int32 GShow = FMath::FloorToInt(MissionGold * Reveal);
	const int32 WShow = FMath::FloorToInt(MissionWood * Reveal);
	const int32 MShow = FMath::FloorToInt(MissionMetal * Reveal);
	DrawText(FString::Printf(TEXT("Gold     +%d"),  GShow), Gold, X + 24.f, Y + 120.f, nullptr, 1.1f, false);
	DrawText(FString::Printf(TEXT("Wood     +%d"),  WShow), Body, X + 24.f, Y + 148.f, nullptr, 1.1f, false);
	DrawText(FString::Printf(TEXT("Metal    +%d"),  MShow), Body, X + 24.f, Y + 176.f, nullptr, 1.1f, false);

	DrawText(TEXT("Press SPACE to dismiss"), Dim, X + 24.f, Y + H - 30.f, nullptr, 0.9f, false);
}

// ============== UI-C: PAUSE MENU ==============

void ACrownsBaneHUD::DrawPauseMenu(ACrownsBanePlayerController* PC)
{
	if (!Canvas) return;
	const float SW = Canvas->ClipX;
	const float SH = Canvas->ClipY;

	// Heavy darken behind everything.
	DrawFilledRect(0, 0, SW, SH, FLinearColor(0.0f, 0.0f, 0.0f, 0.78f));

	const float PW = 420.f;
	const float PH = 380.f;
	const float PX = (SW - PW) * 0.5f;
	const float PY = (SH - PH) * 0.5f;

	DrawPanel(PX, PY, PW, PH, (uint8)CrownStyle::EPanelStyle::Highlight);

	DrawText(TEXT("PAUSED"), CrownStyle::AccentGold.ToFColor(true),
		PX + 24.f, PY + 18.f, nullptr, CrownStyle::ScaleDisplay, false);
	DrawCaption(TEXT("Crown's Bane"), PX + 24.f, PY + 56.f);

	DrawFilledRect(PX + CrownStyle::Sp3, PY + 80.f, PW - CrownStyle::Sp4, 1.f, CrownStyle::AccentGold * FLinearColor(1,1,1,0.5f));

	// Menu items.  Number keys / mouse later — this is just informative for now.
	struct FItem { const TCHAR* Key; const TCHAR* Label; };
	static const FItem Items[] = {
		{ TEXT("[ESC]"),   TEXT("Resume Game") },
		{ TEXT("[J]"),     TEXT("Quest Log") },
		{ TEXT("[U]"),     TEXT("Upgrades (docks only)") },
		{ TEXT("[T]"),     TEXT("Trader (docks only)") },
		{ TEXT("[F1]"),    TEXT("Help / Keybinds") },
	};

	float Y = PY + 100.f;
	for (int32 i = 0; i < UE_ARRAY_COUNT(Items); ++i)
	{
		const FItem& It = Items[i];
		DrawFilledRect(PX + CrownStyle::Sp3, Y, PW - CrownStyle::Sp4, 42.f, CrownStyle::BgLight);
		DrawText(It.Key,   CrownStyle::TextGold,     PX + CrownStyle::Sp4, Y + 12.f, nullptr, 1.1f, false);
		DrawText(It.Label, CrownStyle::TextPrimary,  PX + 90.f,            Y + 12.f, nullptr, 1.05f, false);
		Y += 50.f;
	}

	// Lifetime stats card to the right.
	const float SW2 = PW + 260.f;
	const float SX = PX + PW + CrownStyle::Sp3;
	const float SY = PY;
	const float STW = 260.f;
	const float STH = PH;
	DrawPanel(SX, SY, STW, STH, (uint8)CrownStyle::EPanelStyle::Primary);
	DrawText(TEXT("STATS"), CrownStyle::TextGold, SX + CrownStyle::Sp3, SY + 18.f, nullptr, 1.3f, false);

	const int32 Min = FMath::FloorToInt(PC->StatPlayTimeSeconds / 60.f);
	const int32 Sec = FMath::FloorToInt(FMath::Fmod(PC->StatPlayTimeSeconds, 60.f));

	struct FStat { const TCHAR* Label; FString Val; };
	const FStat Stats[] = {
		{ TEXT("Play time"),    FString::Printf(TEXT("%02d:%02d"), Min, Sec) },
		{ TEXT("Ships sunk"),   FString::FromInt(PC->StatShipsSunk) },
		{ TEXT("Boardings"),    FString::FromInt(PC->StatBoardingsWon) },
		{ TEXT("Shots fired"),  FString::FromInt(PC->StatCannonballsFired) },
		{ TEXT("Damage dealt"), FString::FromInt(PC->StatDamageDealt) },
		{ TEXT("Damage taken"), FString::FromInt(PC->StatDamageTaken) },
		{ TEXT("Gold earned"),  FString::FromInt(PC->StatGoldEarned) },
	};
	float SRY = SY + 60.f;
	for (const FStat& S : Stats)
	{
		DrawText(S.Label, CrownStyle::TextSecondary, SX + CrownStyle::Sp3, SRY,        nullptr, 0.95f, false);
		DrawText(S.Val,   CrownStyle::TextPrimary,   SX + 140.f,           SRY,        nullptr, 0.95f, false);
		SRY += 28.f;
	}
}

// ============== UI-F: HELP OVERLAY (F1) ==============

void ACrownsBaneHUD::DrawHelpOverlay()
{
	if (!Canvas) return;
	const float SW = Canvas->ClipX;
	const float SH = Canvas->ClipY;

	DrawFilledRect(0, 0, SW, SH, FLinearColor(0.0f, 0.0f, 0.0f, 0.78f));

	const float PW = 720.f;
	const float PH = 560.f;
	const float PX = (SW - PW) * 0.5f;
	const float PY = (SH - PH) * 0.5f;
	DrawPanel(PX, PY, PW, PH, (uint8)CrownStyle::EPanelStyle::Highlight);

	DrawText(TEXT("CONTROLS"), CrownStyle::AccentGold.ToFColor(true),
		PX + 24.f, PY + 18.f, nullptr, CrownStyle::ScaleDisplay, false);
	DrawCaption(TEXT("Press F1 again to close"), PX + 24.f, PY + 56.f);
	DrawFilledRect(PX + CrownStyle::Sp3, PY + 80.f, PW - CrownStyle::Sp4, 1.f,
		CrownStyle::AccentGold * FLinearColor(1,1,1,0.5f));

	struct FRow { const TCHAR* Key; const TCHAR* Desc; };
	static const FRow Rows[] = {
		{ TEXT("W / S"),    TEXT("Raise / lower sails (Stop / Half / Full)") },
		{ TEXT("A / D"),    TEXT("Turn ship port / starboard") },
		{ TEXT("Mouse"),    TEXT("Look around (camera yaw/pitch)") },
		{ TEXT("LMB / SPACE"), TEXT("Fire broadside (camera direction picks side)") },
		{ TEXT("Q / E"),    TEXT("Fire port / starboard broadside directly") },
		{ TEXT("RMB hold"), TEXT("Aim mode — zoom + slow-mo + fine mouse") },
		{ TEXT("Tab"),      TEXT("Cycle lock-on target") },
		{ TEXT("1-5"),      TEXT("Ammo: Round / Chain / Grape / Heavy / Explosive") },
		{ TEXT("F"),        TEXT("Board crippled enemy (mash SPACE in QTE)") },
		{ TEXT("B hold"),   TEXT("Brace for impact (-50% incoming, no fire)") },
		{ TEXT("V"),        TEXT("Drop anchor (hard stop)") },
		{ TEXT("U"),        TEXT("Upgrade menu (in docks)") },
		{ TEXT("T"),        TEXT("Trader menu (in docks)") },
		{ TEXT("J"),        TEXT("Quest log") },
		{ TEXT("ESC"),      TEXT("Pause menu") },
		{ TEXT("F1"),       TEXT("This help screen") },
	};

	float Y = PY + 96.f;
	for (int32 i = 0; i < UE_ARRAY_COUNT(Rows); ++i)
	{
		const FRow& R = Rows[i];
		const FLinearColor Bg = (i % 2 == 0)
			? FLinearColor(0.08f, 0.07f, 0.05f, 0.55f)
			: FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
		DrawFilledRect(PX + CrownStyle::Sp3, Y, PW - CrownStyle::Sp4, 26.f, Bg);
		DrawText(R.Key,  CrownStyle::TextGold,    PX + CrownStyle::Sp4, Y + 4.f, nullptr, 1.0f, false);
		DrawText(R.Desc, CrownStyle::TextPrimary, PX + 160.f,           Y + 4.f, nullptr, 0.95f, false);
		Y += 28.f;
	}
}

// ============== XP / LEVEL BAR ==============

void ACrownsBaneHUD::DrawXPBar(ACrownsBanePlayerController* PC)
{
	if (!PC || !PC->Progression || !Canvas) return;
	UShipProgressionComponent* P = PC->Progression;

	const float SW = Canvas->ClipX;
	const float W = 220.f;
	const float H = 12.f;
	const float X = (SW - W) * 0.5f;
	const float Y = HUDPaddingY + 88.f;

	// Label
	DrawText(FString::Printf(TEXT("LVL %d"), P->Level), CrownStyle::AccentGold.ToFColor(true),
		X, Y - 18.f, nullptr, 0.95f, false);
	if (P->bPerkChoicePending)
	{
		DrawText(TEXT("⚑ NEW PERK AVAILABLE"), FColor(255, 220, 100),
			X + 60.f, Y - 18.f, nullptr, 0.9f, false);
	}

	// XP bar
	const float Frac = P->GetXPForNextLevel() > 0
		? FMath::Clamp((float)P->XP / (float)P->GetXPForNextLevel(), 0.f, 1.f)
		: 0.f;
	DrawSmoothBar(X, Y, W, H, Frac, CrownStyle::AccentGold, false);
	DrawText(FString::Printf(TEXT("%d / %d XP"), P->XP, P->GetXPForNextLevel()),
		CrownStyle::TextSecondary, X + W * 0.5f - 30.f, Y - 1.f, nullptr, 0.7f, false);
}

// ============== PERK CHOICE OVERLAY (Гр.1B) ==============

void ACrownsBaneHUD::RefreshPerkChoices(ACrownsBanePlayerController* PC)
{
	if (!PC || !PC->Progression) { PendingPerkChoices.Reset(); return; }
	// Build pool of locked perks; pick up to 3 distinct.
	TArray<uint8> Pool;
	for (uint8 i = (uint8)EShipPerk::EagleEye; i <= (uint8)EShipPerk::Buccaneer; ++i)
	{
		if (!PC->Progression->HasPerk((EShipPerk)i)) Pool.Add(i);
	}
	PendingPerkChoices.Reset();
	while (PendingPerkChoices.Num() < 3 && Pool.Num() > 0)
	{
		const int32 Idx = FMath::RandRange(0, Pool.Num() - 1);
		PendingPerkChoices.Add(Pool[Idx]);
		Pool.RemoveAt(Idx);
	}
}

void ACrownsBaneHUD::DrawPerkChoiceOverlay(ACrownsBanePlayerController* PC)
{
	if (!PC || !PC->Progression || !Canvas) return;
	if (!PC->Progression->bPerkChoicePending) { PendingPerkChoices.Reset(); return; }
	if (PendingPerkChoices.Num() == 0) RefreshPerkChoices(PC);
	if (PendingPerkChoices.Num() == 0) return;

	const float SW = Canvas->ClipX;
	const float SH = Canvas->ClipY;
	DrawFilledRect(0, 0, SW, SH, FLinearColor(0.0f, 0.0f, 0.0f, 0.7f));

	const float PW = 700.f;
	const float PH = 360.f;
	const float PX = (SW - PW) * 0.5f;
	const float PY = (SH - PH) * 0.5f;
	DrawPanel(PX, PY, PW, PH, (uint8)CrownStyle::EPanelStyle::Highlight);

	DrawText(TEXT("⚓ LEVEL UP — CHOOSE PERK"), CrownStyle::AccentGold.ToFColor(true),
		PX + 24.f, PY + 16.f, nullptr, 1.6f, false);
	DrawCaption(TEXT("Press 1 / 2 / 3 to select"), PX + 24.f, PY + 54.f);

	// Perk row card.
	auto PerkInfo = [](EShipPerk P, FString& OutName, FString& OutDesc)
	{
		switch (P)
		{
		case EShipPerk::EagleEye:     OutName = TEXT("EAGLE EYE");      OutDesc = TEXT("Lock-on range doubled (9k → 18k cm)"); break;
		case EShipPerk::IronHull:     OutName = TEXT("IRON HULL");      OutDesc = TEXT("+15% max hull health"); break;
		case EShipPerk::Cutthroat:    OutName = TEXT("CUTTHROAT");      OutDesc = TEXT("Boarding loot multiplier ×3"); break;
		case EShipPerk::StormCaptain: OutName = TEXT("STORM CAPTAIN");  OutDesc = TEXT("Immune to storm wind chaos"); break;
		case EShipPerk::Marksman:     OutName = TEXT("MARKSMAN");       OutDesc = TEXT("+10% critical hit chance"); break;
		case EShipPerk::ReloadMaster: OutName = TEXT("RELOAD MASTER");  OutDesc = TEXT("Cannon reload −1.0 seconds"); break;
		case EShipPerk::WindReader:   OutName = TEXT("WIND READER");    OutDesc = TEXT("Wind drift on your shots halved"); break;
		case EShipPerk::Buccaneer:    OutName = TEXT("BUCCANEER");      OutDesc = TEXT("+25% to all gold gains"); break;
		default:                      OutName = TEXT("???");            OutDesc = TEXT(""); break;
		}
	};

	const float CardW = (PW - 80.f) / 3.f;
	const float CardH = PH - 110.f;
	const float CardY = PY + 86.f;
	for (int32 i = 0; i < PendingPerkChoices.Num(); ++i)
	{
		const EShipPerk P = (EShipPerk)PendingPerkChoices[i];
		FString Name, Desc;
		PerkInfo(P, Name, Desc);
		const float CardX = PX + 20.f + i * (CardW + 10.f);
		DrawFilledRect(CardX, CardY, CardW, CardH, FLinearColor(0.10f, 0.08f, 0.04f, 0.85f));
		DrawBorderedRect(CardX, CardY, CardW, CardH, CrownStyle::BgTransparent, CrownStyle::AccentGold, 1.5f);

		// Number badge
		DrawFilledRect(CardX + 12.f, CardY + 12.f, 28.f, 28.f, CrownStyle::AccentGold);
		DrawText(FString::FromInt(i + 1), FColor(20, 20, 10), CardX + 20.f, CardY + 14.f, nullptr, 1.25f, false);

		DrawText(Name, CrownStyle::TextGold, CardX + 50.f, CardY + 16.f, nullptr, 1.1f, false);

		// Wrap desc across multiple lines (rough — split on space at ~24 chars).
		// For simplicity show on two lines.
		const int32 Cut = 26;
		FString Line1 = Desc.Left(Cut);
		FString Line2 = (Desc.Len() > Cut) ? Desc.Mid(Cut) : TEXT("");
		// Try to break on word boundary
		int32 LastSpace = Line1.Find(TEXT(" "), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		if (Line2.Len() > 0 && LastSpace != INDEX_NONE && LastSpace > Cut - 8)
		{
			Line2 = Desc.Mid(LastSpace + 1);
			Line1 = Desc.Left(LastSpace);
		}
		DrawText(Line1, CrownStyle::TextPrimary, CardX + 12.f, CardY + 80.f,  nullptr, 0.95f, false);
		if (!Line2.IsEmpty())
			DrawText(Line2, CrownStyle::TextPrimary, CardX + 12.f, CardY + 102.f, nullptr, 0.95f, false);
	}
}
