// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "Ship/ShipPawn.h"
#include "Combat/CannonComponent.h"
#include "Components/HealthComponent.h"
#include "Systems/WindSystem.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InputComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"

AShipPawn::AShipPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// Ship mesh - root component
	ShipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShipMesh"));
	RootComponent = ShipMesh;
	ShipMesh->SetSimulatePhysics(false);
	ShipMesh->SetCollisionProfileName(TEXT("Pawn"));

	// Spring arm for third-person camera
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 1800.0f;
	SpringArm->SetRelativeRotation(FRotator(-25.0f, 0.0f, 0.0f));
	SpringArm->bUsePawnControlRotation = false;
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritYaw = true;
	SpringArm->bInheritRoll = false;
	SpringArm->bDoCollisionTest = true;

	// Third-person camera
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	// Cannon component
	CannonComponent = CreateDefaultSubobject<UCannonComponent>(TEXT("CannonComponent"));

	// Health component
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->MaxHealth = 200.0f;

	AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void AShipPawn::BeginPlay()
{
	Super::BeginPlay();

	// Cache the wind system from the world
	CachedWindSystem = nullptr;
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AWindSystem::StaticClass(), FoundActors);
	if (FoundActors.Num() > 0)
	{
		CachedWindSystem = Cast<AWindSystem>(FoundActors[0]);
	}

	UE_LOG(LogTemp, Log, TEXT("ShipPawn: BeginPlay complete. Wind system: %s"),
		CachedWindSystem ? TEXT("found") : TEXT("not found"));
}

void AShipPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update speed penalty
	if (SpeedPenaltyTimeRemaining > 0.0f)
	{
		SpeedPenaltyTimeRemaining -= DeltaTime;
		if (SpeedPenaltyTimeRemaining <= 0.0f)
		{
			SpeedPenaltyTimeRemaining = 0.0f;
			SpeedPenaltyFraction = 0.0f;
			UE_LOG(LogTemp, Log, TEXT("ShipPawn: Speed penalty expired."));
		}
	}

	UpdateMovement(DeltaTime);
	UpdateTurning(DeltaTime, TurnInputValue);
}

void AShipPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// W increases sail, S decreases
	PlayerInputComponent->BindAction("IncreaseSail", IE_Pressed, this, &AShipPawn::IncreaseSail);
	PlayerInputComponent->BindAction("DecreaseSail", IE_Pressed, this, &AShipPawn::DecreaseSail);

	// A/D axis for turning
	PlayerInputComponent->BindAxis("Turn", this, &AShipPawn::TurnRight);

	// LMB / RMB for cannon broadsides
	PlayerInputComponent->BindAction("FireLeft", IE_Pressed, this, &AShipPawn::FireLeft);
	PlayerInputComponent->BindAction("FireRight", IE_Pressed, this, &AShipPawn::FireRight);
}

float AShipPawn::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	// HealthComponent hooks into OnTakeAnyDamage automatically via BeginPlay binding
	return ActualDamage;
}

void AShipPawn::IncreaseSail()
{
	switch (CurrentSailLevel)
	{
	case ESailLevel::Stop:
		CurrentSailLevel = ESailLevel::HalfSail;
		UE_LOG(LogTemp, Log, TEXT("ShipPawn: Sail set to Half."));
		break;
	case ESailLevel::HalfSail:
		CurrentSailLevel = ESailLevel::FullSail;
		UE_LOG(LogTemp, Log, TEXT("ShipPawn: Sail set to Full."));
		break;
	case ESailLevel::FullSail:
		// Already at max
		break;
	}
}

void AShipPawn::DecreaseSail()
{
	switch (CurrentSailLevel)
	{
	case ESailLevel::FullSail:
		CurrentSailLevel = ESailLevel::HalfSail;
		UE_LOG(LogTemp, Log, TEXT("ShipPawn: Sail set to Half."));
		break;
	case ESailLevel::HalfSail:
		CurrentSailLevel = ESailLevel::Stop;
		UE_LOG(LogTemp, Log, TEXT("ShipPawn: Sail set to Stop."));
		break;
	case ESailLevel::Stop:
		// Already stopped
		break;
	}
}

void AShipPawn::TurnRight(float Value)
{
	TurnInputValue = Value;
}

void AShipPawn::FireLeft()
{
	if (CannonComponent)
	{
		CannonComponent->FireBroadside(ECannonSide::Left);
	}
}

void AShipPawn::FireRight()
{
	if (CannonComponent)
	{
		CannonComponent->FireBroadside(ECannonSide::Right);
	}
}

float AShipPawn::GetTargetSpeed() const
{
	float SailMultiplier = 0.0f;
	switch (CurrentSailLevel)
	{
	case ESailLevel::Stop:
		SailMultiplier = 0.0f;
		break;
	case ESailLevel::HalfSail:
		SailMultiplier = HalfSailSpeedMultiplier;
		break;
	case ESailLevel::FullSail:
		SailMultiplier = 1.0f;
		break;
	}

	// Apply chain-shot speed penalty
	float PenaltyFactor = 1.0f - SpeedPenaltyFraction;
	PenaltyFactor = FMath::Clamp(PenaltyFactor, 0.0f, 1.0f);

	// Apply wind multiplier
	float WindMult = GetWindMultiplier();

	return MaxSpeed * SailMultiplier * PenaltyFactor * WindMult;
}

float AShipPawn::GetWindMultiplier() const
{
	if (!CachedWindSystem)
	{
		return 1.0f;
	}

	FVector ShipForward = GetActorForwardVector();
	float WindBonus = CachedWindSystem->GetWindSpeedMultiplier(ShipForward);
	// WindBonus returns value in range [-1, 1], we scale by WindInfluenceFactor
	return 1.0f + (WindBonus * WindInfluenceFactor);
}

void AShipPawn::UpdateMovement(float DeltaTime)
{
	float TargetSpeed = GetTargetSpeed();

	// Apply inertia: smoothly interpolate current speed toward target
	if (CurrentSpeed < TargetSpeed)
	{
		CurrentSpeed = FMath::Min(CurrentSpeed + AccelerationRate * DeltaTime, TargetSpeed);
	}
	else if (CurrentSpeed > TargetSpeed)
	{
		CurrentSpeed = FMath::Max(CurrentSpeed - DecelerationRate * DeltaTime, TargetSpeed);
	}

	// Move the ship forward
	if (CurrentSpeed > 0.01f)
	{
		FVector ForwardMove = GetActorForwardVector() * CurrentSpeed * DeltaTime;
		AddActorWorldOffset(ForwardMove, true);
	}
}

void AShipPawn::UpdateTurning(float DeltaTime, float TurnInput)
{
	if (FMath::IsNearlyZero(TurnInput))
	{
		return;
	}

	// Wider turning radius at higher speed: lerp turn rate between full and reduced
	float SpeedFraction = (MaxSpeed > 0.0f) ? (CurrentSpeed / MaxSpeed) : 0.0f;
	float TurnRate = FMath::Lerp(BaseTurnRate, BaseTurnRate * HighSpeedTurnFactor, SpeedFraction);

	float YawDelta = TurnInput * TurnRate * DeltaTime;
	AddActorLocalRotation(FRotator(0.0f, YawDelta, 0.0f));
}

void AShipPawn::ApplySpeedPenalty(float PenaltyFraction, float Duration)
{
	SpeedPenaltyFraction = FMath::Clamp(PenaltyFraction, 0.0f, 1.0f);
	SpeedPenaltyTimeRemaining = Duration;
	UE_LOG(LogTemp, Log, TEXT("ShipPawn: Speed penalty applied: %.0f%% for %.1fs"),
		PenaltyFraction * 100.0f, Duration);
}

void AShipPawn::UpgradeMaxSpeed(float BonusSpeed)
{
	MaxSpeed += BonusSpeed;
	UE_LOG(LogTemp, Log, TEXT("ShipPawn: MaxSpeed upgraded to %.1f"), MaxSpeed);
}

void AShipPawn::UpgradeTurnRate(float BonusTurnRate)
{
	BaseTurnRate += BonusTurnRate;
	UE_LOG(LogTemp, Log, TEXT("ShipPawn: BaseTurnRate upgraded to %.1f"), BaseTurnRate);
}
