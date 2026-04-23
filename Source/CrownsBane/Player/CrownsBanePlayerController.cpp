// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "Player/CrownsBanePlayerController.h"
#include "Player/PlayerInventory.h"
#include "Docks/DocksZone.h"
#include "Upgrades/UpgradeManager.h"
#include "Ship/ShipPawn.h"
#include "UI/CrownsBaneHUD.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Upgrades/UpgradeTypes.h"

ACrownsBanePlayerController::ACrownsBanePlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	PlayerInventory = CreateDefaultSubobject<UPlayerInventory>(TEXT("PlayerInventory"));
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
}

void ACrownsBanePlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

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

	if (InputComponent)
	{
		InputComponent->BindAction("ToggleUpgradeUI", IE_Pressed, this, &ACrownsBanePlayerController::ToggleUpgradeUI);
	}
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

bool ACrownsBanePlayerController::BuyUpgrade(uint8 CategoryByte)
{
	if (!bIsInDocks) return false;

	AUpgradeManager* UM = GetUpgradeManager();
	AShipPawn* Ship = GetShipPawn();
	if (!UM || !Ship || !PlayerInventory) return false;

	return UM->PurchaseUpgrade((EUpgradeCategory)CategoryByte, PlayerInventory, Ship);
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