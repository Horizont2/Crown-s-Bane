// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "Player/CrownsBanePlayerController.h"
#include "Player/PlayerInventory.h"
#include "Docks/DocksZone.h"
#include "Upgrades/UpgradeManager.h"
#include "Ship/ShipPawn.h"
#include "Components/HealthComponent.h"
#include "Loot/ResourceTypes.h"
#include "UI/CrownsBaneHUD.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Upgrades/UpgradeTypes.h"
#include "Player/CrownsBaneSaveGame.h"
#include "Player/PlayerInventory.h"
#include "Player/ShipProgressionComponent.h"
#include "Audio/SoundManager.h"
#include "EngineUtils.h"

ACrownsBanePlayerController::ACrownsBanePlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	PlayerInventory = CreateDefaultSubobject<UPlayerInventory>(TEXT("PlayerInventory"));
	Progression    = CreateDefaultSubobject<UShipProgressionComponent>(TEXT("Progression"));
	SoundManager   = CreateDefaultSubobject<USoundManager>(TEXT("SoundManager"));
	bIsInDocks = false;
	bUpgradeUIOpen = false;
}

void ACrownsBanePlayerController::BeginPlay()
{
	Super::BeginPlay();

	bShowMouseCursor = false;
	FInputModeGameOnly Mode;
	Mode.SetConsumeCaptureMouseDown(true);
	SetInputMode(Mode);

	ForceFocusGameViewport();

	// Bind level-up + perk-unlock to sound cues.
	if (Progression)
	{
		Progression->OnShipLevelUp.AddDynamic(this, &ACrownsBanePlayerController::HandleLevelUp);
		Progression->OnPerkUnlocked.AddDynamic(this, &ACrownsBanePlayerController::HandlePerkUnlocked);
	}
}

void ACrownsBanePlayerController::HandleLevelUp(int32 NewLevel, int32 NewXP)
{
	if (SoundManager) SoundManager->Play(ESoundCue::LevelUp);
	if (ACrownsBaneHUD* HUD = Cast<ACrownsBaneHUD>(GetHUD()))
	{
		HUD->ShowBanner(FString::Printf(TEXT("LEVEL %d"), NewLevel),
			(NewLevel % 5 == 0) ? TEXT("Choose your perk!") : TEXT("Ship grows stronger."),
			FLinearColor(1.0f, 0.85f, 0.2f, 1.0f), 2.8f);
		HUD->TriggerRadialBurst(FLinearColor(1.0f, 0.85f, 0.25f, 1.0f), 0.9f);
	}
}

void ACrownsBanePlayerController::HandlePerkUnlocked(EShipPerk Perk)
{
	if (SoundManager) SoundManager->Play(ESoundCue::PerkUnlock);
}

void ACrownsBanePlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	bShowMouseCursor = false;
	FInputModeGameOnly Mode;
	Mode.SetConsumeCaptureMouseDown(true);
	SetInputMode(Mode);

	FlushPressedKeys();
	ForceFocusGameViewport();

	FocusTimer = 0.0f;
	bForcedFocusOnce = false;

	UE_LOG(LogTemp, Log, TEXT("[PlayerController] Possessed %s — input mode locked to Game."),
		InPawn ? *InPawn->GetName() : TEXT("NULL"));

	// Re-apply any unlocked perks to the (potentially new) pawn's stats.
	ApplyPerkBonuses();
}

void ACrownsBanePlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	StatPlayTimeSeconds += DeltaSeconds;

	// Re-claim Slate focus for the first 5 seconds after BeginPlay/possess.
	// PIE frequently leaves focus on the editor toolbar so the game viewport
	// never sees a single key press — this is the fix.
	FocusTimer += DeltaSeconds;
	if (FocusTimer < 5.0f)
	{
		ForceFocusGameViewport();
	}
	else if (!bForcedFocusOnce)
	{
		bForcedFocusOnce = true;
		ForceFocusGameViewport();
	}
}

void ACrownsBanePlayerController::ForceFocusGameViewport()
{
	// Step 1: tell Slate that the game viewport owns focus.  Without this,
	// PIE often leaves focus on the editor toolbar, so key events go to
	// the editor and never reach the pawn.
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetAllUserFocusToGameViewport();
	}

	// Step 2: physically capture the mouse into the viewport, lock the cursor,
	// and grab joystick events.  All three are necessary in combination.
	if (GEngine && GEngine->GameViewport)
	{
		if (FViewport* Viewport = GEngine->GameViewport->Viewport)
		{
			Viewport->CaptureMouse(true);
			Viewport->LockMouseToViewport(true);
		}
	}
}

void ACrownsBanePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	// U-key docks toggle is now handled via Enhanced Input in ShipPawn (IA_ToggleDocks).
}

void ACrownsBanePlayerController::OnEnterDocks(ADocksZone* DocksZone)
{
	bIsInDocks = true;
	CurrentDocksZone = DocksZone;

	if (ACrownsBaneHUD* HUD = Cast<ACrownsBaneHUD>(GetHUD()))
		HUD->bShowDocksPrompt = true;
}

void ACrownsBanePlayerController::OnExitDocks()
{
	if (bUpgradeUIOpen) CloseUpgradeUI();
	bIsInDocks = false;
	CurrentDocksZone = nullptr;

	if (ACrownsBaneHUD* HUD = Cast<ACrownsBaneHUD>(GetHUD()))
		HUD->bShowDocksPrompt = false;
}

void ACrownsBanePlayerController::OpenUpgradeUI()
{
	if (!bIsInDocks || bUpgradeUIOpen) return;
	bUpgradeUIOpen = true;
	bShowMouseCursor = true;
	SetInputMode(FInputModeGameAndUI());
}

void ACrownsBanePlayerController::CloseUpgradeUI()
{
	if (!bUpgradeUIOpen) return;
	bUpgradeUIOpen = false;
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
}

void ACrownsBanePlayerController::ToggleUpgradeUI()
{
	bUpgradeUIOpen ? CloseUpgradeUI() : OpenUpgradeUI();
}

void ACrownsBanePlayerController::ToggleQuestLog()
{
	bQuestLogOpen = !bQuestLogOpen;
	if (ACrownsBaneHUD* HUD = Cast<ACrownsBaneHUD>(GetHUD()))
	{
		HUD->bShowQuestLog = bQuestLogOpen;
	}
}

void ACrownsBanePlayerController::TogglePauseMenu()
{
	bPauseMenuOpen = !bPauseMenuOpen;
	SetPause(bPauseMenuOpen);
	bShowMouseCursor = bPauseMenuOpen;
	if (bPauseMenuOpen) SetInputMode(FInputModeGameAndUI());
	else                SetInputMode(FInputModeGameOnly());
	if (SoundManager) SoundManager->Play(ESoundCue::UIOpen);
}

void ACrownsBanePlayerController::ToggleTraderMenu()
{
	// Trader only available while docked.
	if (!bTraderMenuOpen && !bIsInDocks) return;
	bTraderMenuOpen = !bTraderMenuOpen;
	bShowMouseCursor = bTraderMenuOpen;
	if (bTraderMenuOpen)
	{
		SetInputMode(FInputModeGameAndUI());
	}
	else
	{
		SetInputMode(FInputModeGameOnly());
	}
}

static float DockPriceMul_Buy(EDockType T)
{
	switch (T)
	{
	case EDockType::PirateHaven: return 0.70f; // -30%
	case EDockType::Naval:       return 1.20f; // +20% (limited stock implication)
	case EDockType::Merchant:
	default:                     return 1.00f;
	}
}
static float DockPriceMul_Sell(EDockType T)
{
	// Merchants pay best; pirates pay middling; navy pays poorly for plunder.
	switch (T)
	{
	case EDockType::Merchant:    return 1.25f;
	case EDockType::PirateHaven: return 1.00f;
	case EDockType::Naval:       return 0.70f;
	default:                     return 1.00f;
	}
}
static float DockPriceMul_Upgrade(EDockType T)
{
	switch (T)
	{
	case EDockType::PirateHaven: return 1.20f; // pirate shipwrights extort more
	case EDockType::Naval:       return 1.00f;
	case EDockType::Merchant:
	default:                     return 0.90f; // merchant ports cut a deal
	}
}

bool ACrownsBanePlayerController::BuyAmmo(int32 Amount)
{
	if (!bIsInDocks || !PlayerInventory) return false;
	const EDockType T = CurrentDocksZone ? CurrentDocksZone->DockType : EDockType::Merchant;
	const int32 Price = FMath::Max(1, FMath::RoundToInt(BuyAmmoPrice * DockPriceMul_Buy(T)));
	if (!PlayerInventory->SpendResource(EResourceType::Gold, Price)) return false;
	PlayerInventory->AddAmmo(Amount);
	return true;
}

bool ACrownsBanePlayerController::BuyWood(int32 Amount)
{
	if (!bIsInDocks || !PlayerInventory) return false;
	const EDockType T = CurrentDocksZone ? CurrentDocksZone->DockType : EDockType::Merchant;
	const int32 Price = FMath::Max(1, FMath::RoundToInt(BuyWoodPrice * DockPriceMul_Buy(T)));
	if (!PlayerInventory->SpendResource(EResourceType::Gold, Price)) return false;
	PlayerInventory->AddResource(EResourceType::Wood, Amount);
	return true;
}

bool ACrownsBanePlayerController::BuyMetal(int32 Amount)
{
	if (!bIsInDocks || !PlayerInventory) return false;
	const EDockType T = CurrentDocksZone ? CurrentDocksZone->DockType : EDockType::Merchant;
	const int32 Price = FMath::Max(1, FMath::RoundToInt(BuyMetalPrice * DockPriceMul_Buy(T)));
	if (!PlayerInventory->SpendResource(EResourceType::Gold, Price)) return false;
	PlayerInventory->AddResource(EResourceType::Metal, Amount);
	return true;
}

bool ACrownsBanePlayerController::SellWood(int32 Amount)
{
	if (!bIsInDocks || !PlayerInventory) return false;
	if (!PlayerInventory->SpendResource(EResourceType::Wood, Amount)) return false;
	const EDockType T = CurrentDocksZone ? CurrentDocksZone->DockType : EDockType::Merchant;
	const int32 Payout = FMath::Max(1, FMath::RoundToInt(SellWoodPrice * DockPriceMul_Sell(T)));
	PlayerInventory->AddResource(EResourceType::Gold, Payout);
	return true;
}

bool ACrownsBanePlayerController::SellMetal(int32 Amount)
{
	if (!bIsInDocks || !PlayerInventory) return false;
	if (!PlayerInventory->SpendResource(EResourceType::Metal, Amount)) return false;
	const EDockType T = CurrentDocksZone ? CurrentDocksZone->DockType : EDockType::Merchant;
	const int32 Payout = FMath::Max(1, FMath::RoundToInt(SellMetalPrice * DockPriceMul_Sell(T)));
	PlayerInventory->AddResource(EResourceType::Gold, Payout);
	return true;
}

bool ACrownsBanePlayerController::PayForHeal()
{
	if (!bIsInDocks || !PlayerInventory) return false;
	AShipPawn* Ship = GetShipPawn();
	if (!Ship || !Ship->HealthComponent) return false;
	if (Ship->HealthComponent->GetHealthPercent() >= 0.999f) return false; // already full
	const EDockType T = CurrentDocksZone ? CurrentDocksZone->DockType : EDockType::Merchant;
	const int32 Cost = FMath::Max(1, FMath::RoundToInt(HealCost * DockPriceMul_Buy(T)));
	if (!PlayerInventory->SpendResource(EResourceType::Gold, Cost)) return false;
	Ship->HealthComponent->Heal(Ship->HealthComponent->GetMaxHealth());
	return true;
}

bool ACrownsBanePlayerController::BuyUpgrade(uint8 CategoryByte)
{
	if (!bIsInDocks) return false;

	AUpgradeManager* UM = GetUpgradeManager();
	AShipPawn* Ship = GetShipPawn();
	if (!UM || !Ship || !PlayerInventory) return false;

	const bool bOK = UM->PurchaseUpgrade((EUpgradeCategory)CategoryByte, PlayerInventory, Ship);
	if (bOK && SoundManager) SoundManager->Play(ESoundCue::UIClick, 1.2f);
	return bOK;
}

AUpgradeManager* ACrownsBanePlayerController::GetUpgradeManager() const
{
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUpgradeManager::StaticClass(), Actors);
	return Actors.Num() > 0 ? Cast<AUpgradeManager>(Actors[0]) : nullptr;
}

AShipPawn* ACrownsBanePlayerController::GetShipPawn() const
{
	return Cast<AShipPawn>(GetPawn());
}
void ACrownsBanePlayerController::StatBumpShipsSunk()      { ++StatShipsSunk; }
void ACrownsBanePlayerController::StatBumpBoardingsWon()   { ++StatBoardingsWon; }
void ACrownsBanePlayerController::StatBumpCannonballsFired(int32 N) { StatCannonballsFired += N; }
void ACrownsBanePlayerController::StatBumpDamageDealt(float Dmg)    { StatDamageDealt += FMath::FloorToInt(Dmg); }
void ACrownsBanePlayerController::StatBumpDamageTaken(float Dmg)    { StatDamageTaken += FMath::FloorToInt(Dmg); }
void ACrownsBanePlayerController::StatBumpGoldEarned(int32 Amt)     { StatGoldEarned += Amt; }

void ACrownsBanePlayerController::AutoSaveGame(FVector RespawnLoc, FRotator RespawnRot)
{
	UCrownsBaneSaveGame* SG = Cast<UCrownsBaneSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UCrownsBaneSaveGame::StaticClass()));
	if (!SG) return;

	if (PlayerInventory)
	{
		SG->Resources.Gold  = PlayerInventory->GetGold();
		SG->Resources.Wood  = PlayerInventory->GetWood();
		SG->Resources.Metal = PlayerInventory->GetMetal();
		SG->Ammo            = PlayerInventory->GetAmmo();
	}

	SG->RespawnLocation = RespawnLoc;
	SG->RespawnRotation = RespawnRot;
	SG->TotalShipsSunk  = StatShipsSunk;
	SG->StatPlayTimeSeconds = StatPlayTimeSeconds;

	UGameplayStatics::SaveGameToSlot(SG, UCrownsBaneSaveGame::SlotName, UCrownsBaneSaveGame::UserIndex);
	UE_LOG(LogTemp, Log, TEXT("[Save] Auto-saved at %s"), *RespawnLoc.ToString());
}

bool ACrownsBanePlayerController::LoadGameFromSlot()
{
	if (!UGameplayStatics::DoesSaveGameExist(UCrownsBaneSaveGame::SlotName, UCrownsBaneSaveGame::UserIndex))
	{
		return false;
	}
	UCrownsBaneSaveGame* SG = Cast<UCrownsBaneSaveGame>(
		UGameplayStatics::LoadGameFromSlot(UCrownsBaneSaveGame::SlotName, UCrownsBaneSaveGame::UserIndex));
	if (!SG) return false;

	if (PlayerInventory)
	{
		PlayerInventory->Gold  = SG->Resources.Gold;
		PlayerInventory->Wood  = SG->Resources.Wood;
		PlayerInventory->Metal = SG->Resources.Metal;
		PlayerInventory->CannonAmmo = SG->Ammo;
	}
	if (AShipPawn* Ship = GetShipPawn())
	{
		Ship->SetActorLocation(SG->RespawnLocation);
		Ship->SetActorRotation(SG->RespawnRotation);
		if (Ship->HealthComponent) Ship->HealthComponent->FullHeal();
	}
	StatShipsSunk = SG->TotalShipsSunk;
	StatPlayTimeSeconds = SG->StatPlayTimeSeconds;
	UE_LOG(LogTemp, Log, TEXT("[Save] Loaded slot."));
	return true;
}

void ACrownsBanePlayerController::ApplyPerkBonuses()
{
	if (!Progression) return;
	AShipPawn* Ship = GetShipPawn();
	if (!Ship) return;

	// IronHull: +15% MaxHP.  Apply by setting MaxHealth and topping current HP up.
	if (Progression->HasPerk(EShipPerk::IronHull) && Ship->HealthComponent)
	{
		const float NewMax = Ship->HealthComponent->GetMaxHealth() * 1.15f;
		// Only apply once — guard by checking we haven't already inflated past base.
		// (Simple approach: store a flag — but we just clamp to NewMax which is idempotent
		// if the base value didn't change in this session.)
		Ship->HealthComponent->MaxHealth = NewMax;
		Ship->HealthComponent->Heal(NewMax * 0.15f);
	}

	// EagleEye: lock-on max range x2.
	if (Progression->HasPerk(EShipPerk::EagleEye))
	{
		Ship->LockOnMaxRange = FMath::Max(Ship->LockOnMaxRange, 18000.0f);
	}

	// Cutthroat: boarding loot ×3.
	if (Progression->HasPerk(EShipPerk::Cutthroat))
	{
		Ship->BoardingLootMultiplier = FMath::Max(Ship->BoardingLootMultiplier, 3.0f);
	}

	// ReloadMaster: -1.0s reload time on the cannon component.
	if (Progression->HasPerk(EShipPerk::ReloadMaster) && Ship->CannonComponent)
	{
		Ship->CannonComponent->ReloadTime = FMath::Max(0.5f, Ship->CannonComponent->ReloadTime - 1.0f);
	}
}

void ACrownsBanePlayerController::BumpFaction(float NavalDelta, float PirateDelta)
{
	NavalRep  = FMath::Clamp(NavalRep + NavalDelta,  -100.f, 100.f);
	PirateRep = FMath::Clamp(PirateRep + PirateDelta, -100.f, 100.f);
}

bool ACrownsBanePlayerController::FastTravelToNearestDock()
{
	UWorld* W = GetWorld();
	if (!W) return false;
	AShipPawn* Ship = GetShipPawn();
	if (!Ship || !PlayerInventory) return false;
	if (!PlayerInventory->SpendResource(EResourceType::Gold, 50)) return false;

	ADocksZone* Best = nullptr;
	float BestD2 = TNumericLimits<float>::Max();
	for (TActorIterator<ADocksZone> It(W); It; ++It)
	{
		ADocksZone* D = *It;
		if (!D) continue;
		const float D2 = FVector::DistSquared(Ship->GetActorLocation(), D->GetActorLocation());
		if (D2 < BestD2) { BestD2 = D2; Best = D; }
	}
	if (!Best) { PlayerInventory->AddResource(EResourceType::Gold, 50); return false; }

	Ship->SetActorLocation(Best->GetActorLocation() + FVector(0, 0, 60));
	Ship->SetActorRotation(Best->GetActorRotation());
	if (ACrownsBaneHUD* HUD = Cast<ACrownsBaneHUD>(GetHUD()))
	{
		HUD->ShowBanner(TEXT("FAST TRAVEL"),
			FString::Printf(TEXT("Arrived at %s"), *Best->DockName),
			FLinearColor(0.4f, 0.75f, 1.0f, 1.0f), 3.0f);
	}
	return true;
}

void ACrownsBanePlayerController::SetWaypoint(FVector Location, const FString& Label)
{
	bHasWaypoint = true;
	WaypointLocation = Location;
	WaypointLabel = Label;
	if (ACrownsBaneHUD* HUD = Cast<ACrownsBaneHUD>(GetHUD()))
	{
		HUD->PushResourceToast(FString::Printf(TEXT("↖ Waypoint set: %s"), *Label),
			FLinearColor(0.4f, 0.75f, 1.0f, 1.0f));
	}
}
