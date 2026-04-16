// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "AI/ShipAIController.h"
#include "AI/EnemyShipBase.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

AShipAIController::AShipAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	ControlledShip = nullptr;
	TargetPawn = nullptr;
}

void AShipAIController::BeginPlay()
{
	Super::BeginPlay();
}

void AShipAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ControlledShip = Cast<AEnemyShipBase>(InPawn);
	if (ControlledShip)
	{
		UE_LOG(LogTemp, Log, TEXT("ShipAIController: Possessing %s"), *InPawn->GetName());
	}

	// Default target is the player
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC && PC->GetPawn())
	{
		TargetPawn = PC->GetPawn();
	}
}

void AShipAIController::OnUnPossess()
{
	Super::OnUnPossess();
	ControlledShip = nullptr;
	UE_LOG(LogTemp, Log, TEXT("ShipAIController: Unpossessing ship."));
}

void AShipAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// The actual AI logic lives in EnemyShipBase::Tick via state machine.
	// The AIController is responsible for target acquisition here.

	// Re-validate target if lost
	if (!TargetPawn || !IsValid(TargetPawn))
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		if (PC && PC->GetPawn())
		{
			TargetPawn = PC->GetPawn();
		}
	}
}

void AShipAIController::SetTarget(APawn* NewTarget)
{
	TargetPawn = NewTarget;
	UE_LOG(LogTemp, Log, TEXT("ShipAIController: Target set to %s"),
		NewTarget ? *NewTarget->GetName() : TEXT("None"));
}
