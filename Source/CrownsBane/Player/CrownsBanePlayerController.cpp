// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "Player/CrownsBanePlayerController.h"
#include "Player/PlayerInventory.h"
#include "Docks/DocksZone.h"
#include "Upgrades/UpgradeManager.h"
#include "Ship/ShipPawn.h"
#include "UI/CrownsBaneHUD.h"
#include "Components/InputComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

ACrownsBanePlayerController::ACrownsBanePlayerController()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create the player inventory as a component on the controller
	PlayerInventory = CreateDefaultSubobject<UPlayerInventory>(TEXT("PlayerInventory"));
}

void ACrownsBanePlayerController::BeginPlay()
{
	Super::BeginPlay();

	bIsInDocks = false;
	bUpgradeUIOpen = false;

	UE_LOG(LogTemp, Log, TEXT("CrownsBanePlayerController: BeginPlay complete."));
}

void ACrownsBanePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!InputComponent) return;

	// Movement actions (bound to ship pawn via SetupPlayerInputComponent in ShipPawn)
	// W/S: IncreaseSail / DecreaseSail - handled in ShipPawn
	// A/D: Turn axis - handled in ShipPawn
	// LMB/RMB: FireLeft / FireRight - handled in ShipPawn

	// Controller-level inputs
	InputComponent->BindAction("OpenUpgradeUI", IE_Pressed, this, &ACrownsBanePlayerController::InputOpenUpgradeUI);
	InputComponent->BindAction("CloseUpgradeUI", IE_Pressed, this, &ACrownsBanePlayerController::InputCloseUpgradeUI);
}

void ACrownsBanePlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ACrownsBanePlayerController::OnEnterDocks(ADocksZone* DocksZone)
{
	bIsInDocks = true;
	CurrentDocksZone = DocksZone;

	UE_LOG(LogTemp, Log, TEXT("CrownsBanePlayerController: Entered docks."));

	// Show the docks prompt in the HUD
	ACrownsBaneHUD* HUD = Cast<ACrownsBaneHUD>(GetHUD());
	if (HUD)
	{
		HUD->bShowDocksPrompt = true;
	}
}

void ACrownsBanePlayerController::OnExitDocks()
{
	bIsInDocks = false;

	if (bUpgradeUIOpen)
	{
		CloseUpgradeUI();
	}

	CurrentDocksZone = nullptr;

	ACrownsBaneHUD* HUD = Cast<ACrownsBaneHUD>(GetHUD());
	if (HUD)
	{
		HUD->bShowDocksPrompt = false;
	}

	UE_LOG(LogTemp, Log, TEXT("CrownsBanePlayerController: Exited docks."));
}

void ACrownsBanePlayerController::OpenUpgradeUI()
{
	if (!bIsInDocks)
	{
		UE_LOG(LogTemp, Log, TEXT("CrownsBanePlayerController: Cannot open upgrade UI outside of docks."));
		return;
	}

	if (bUpgradeUIOpen) return;

	bUpgradeUIOpen = true;

	// Show mouse cursor for UI navigation
	bShowMouseCursor = true;
	SetInputMode(FInputModeGameAndUI());

	UE_LOG(LogTemp, Log, TEXT("CrownsBanePlayerController: Upgrade UI opened."));
	// Blueprint/Widget implementation hooks in here via event
}

void ACrownsBanePlayerController::CloseUpgradeUI()
{
	if (!bUpgradeUIOpen) return;

	bUpgradeUIOpen = false;

	// Return to game input mode
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());

	UE_LOG(LogTemp, Log, TEXT("CrownsBanePlayerController: Upgrade UI closed."));
}

void ACrownsBanePlayerController::InputOpenUpgradeUI()
{
	if (bIsInDocks && !bUpgradeUIOpen)
	{
		OpenUpgradeUI();
	}
	else if (bUpgradeUIOpen)
	{
		CloseUpgradeUI();
	}
}

void ACrownsBanePlayerController::InputCloseUpgradeUI()
{
	if (bUpgradeUIOpen)
	{
		CloseUpgradeUI();
	}
}

bool ACrownsBanePlayerController::BuyUpgrade(uint8 CategoryByte)
{
	if (!bIsInDocks)
	{
		UE_LOG(LogTemp, Log, TEXT("CrownsBanePlayerController: Can only buy upgrades while docked!"));
		return false;
	}

	AUpgradeManager* UM = GetUpgradeManager();
	AShipPawn* Ship = GetShipPawn();

	if (!UM)
	{
		UE_LOG(LogTemp, Warning, TEXT("CrownsBanePlayerController: No UpgradeManager found!"));
		return false;
	}

	if (!Ship)
	{
		UE_LOG(LogTemp, Warning, TEXT("CrownsBanePlayerController: No ShipPawn found!"));
		return false;
	}

	if (!PlayerInventory)
	{
		UE_LOG(LogTemp, Warning, TEXT("CrownsBanePlayerController: No PlayerInventory!"));
		return false;
	}

	EUpgradeCategory Category = (EUpgradeCategory)CategoryByte;
	bool bSuccess = UM->PurchaseUpgrade(Category, PlayerInventory, Ship);

	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("CrownsBanePlayerController: Upgrade purchased successfully!"));
	}

	return bSuccess;
}

AUpgradeManager* ACrownsBanePlayerController::GetUpgradeManager() const
{
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUpgradeManager::StaticClass(), Actors);
	return (Actors.Num() > 0) ? Cast<AUpgradeManager>(Actors[0]) : nullptr;
}

AShipPawn* ACrownsBanePlayerController::GetShipPawn() const
{
	return Cast<AShipPawn>(GetPawn());
}
