// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "Player/CrownsBanePlayerController.h"
#include "Player/PlayerInventory.h"
#include "Docks/DocksZone.h"
#include "Upgrades/UpgradeManager.h"
#include "Ship/ShipPawn.h"
#include "UI/CrownsBaneHUD.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Upgrades/UpgradeTypes.h"

ACrownsBanePlayerController::ACrownsBanePlayerController()
{
	PrimaryActorTick.bCanEverTick = false;
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
}

void ACrownsBanePlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// Re-apply game-only input every time we take control of a pawn so the
	// viewport never stays in UI mode after a re-possession.
	bShowMouseCursor = false;
	FInputModeGameOnly Mode;
	Mode.SetConsumeCaptureMouseDown(true);
	SetInputMode(Mode);

	// Ensure the viewport actually receives key events — without this, PIE
	// sometimes leaves focus on the editor UI.
	FlushPressedKeys();

	UE_LOG(LogTemp, Log, TEXT("[PlayerController] Possessed %s — input mode locked to Game."),
		InPawn ? *InPawn->GetName() : TEXT("NULL"));
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