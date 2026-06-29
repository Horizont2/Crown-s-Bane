// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "Ship/ShipPawn.h"
#include "Combat/CannonComponent.h"
#include "Components/HealthComponent.h"
#include "Systems/WindSystem.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputModifiers.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Camera/CameraShakeBase.h"

AShipPawn::AShipPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	ShipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShipMesh"));
	ShipMesh->SetupAttachment(RootComponent);
	ShipMesh->SetSimulatePhysics(false);
	ShipMesh->SetCollisionProfileName(TEXT("Pawn"));
	ShipMesh->SetGenerateOverlapEvents(true);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = DefaultSpringArmLength;
	SpringArm->SetRelativeRotation(FRotator(-25.0f, 0.0f, 0.0f));
	SpringArm->bUsePawnControlRotation = false;
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritYaw = true;
	SpringArm->bInheritRoll = false;
	SpringArm->bDoCollisionTest = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	CannonComponent = CreateDefaultSubobject<UCannonComponent>(TEXT("CannonComponent"));
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->MaxHealth = 200.0f;

	DamageSmokeFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("DamageSmokeFX"));
	DamageSmokeFX->SetupAttachment(ShipMesh);
	DamageSmokeFX->bAutoActivate = false;
	DamageSmokeFX->SetRelativeLocation(SmokeSocketOffset);

	DamageFireFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("DamageFireFX"));
	DamageFireFX->SetupAttachment(ShipMesh);
	DamageFireFX->bAutoActivate = false;
	DamageFireFX->SetRelativeLocation(FireSocketOffset);

	BowWakeFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("BowWakeFX"));
	BowWakeFX->SetupAttachment(ShipMesh);
	BowWakeFX->bAutoActivate = false;
	BowWakeFX->SetRelativeLocation(BowWakeOffset);

	AutoPossessPlayer = EAutoReceiveInput::Player0;
	AutoPossessAI = EAutoPossessAI::Disabled;

	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
}

void AShipPawn::BeginPlay()
{
	Super::BeginPlay();

	EnsureInputAssetsExist();
	AddInputMappingContext();

	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AWindSystem::StaticClass(), Found);
	CachedWindSystem = Found.Num() > 0 ? Cast<AWindSystem>(Found[0]) : nullptr;

	if (HealthComponent)
	{
		HealthComponent->OnHealthChanged.AddDynamic(this, &AShipPawn::HandleHealthChanged);
		HealthComponent->OnDeath.AddDynamic(this, &AShipPawn::HandleDeath);
	}

	if (DamageSmokeFX && SmokeAsset) DamageSmokeFX->SetAsset(SmokeAsset);
	if (DamageFireFX && FireAsset)   DamageFireFX->SetAsset(FireAsset);
	if (BowWakeFX && WakeAsset)      BowWakeFX->SetAsset(WakeAsset);
}

void AShipPawn::PawnClientRestart()
{
	Super::PawnClientRestart();
	EnsureInputAssetsExist();
	AddInputMappingContext();
}

void AShipPawn::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
	EnsureInputAssetsExist();
	AddInputMappingContext();
}

void AShipPawn::EnsureInputAssetsExist()
{
	auto MakeIA = [this](UInputAction*& Target, EInputActionValueType ValueType, const TCHAR* Tag)
		{
			if (Target) return;
			Target = NewObject<UInputAction>(this, UInputAction::StaticClass(), FName(Tag));
			Target->ValueType = ValueType;
		};

	MakeIA(IA_IncreaseSail, EInputActionValueType::Boolean, TEXT("IA_IncreaseSail_Auto"));
	MakeIA(IA_DecreaseSail, EInputActionValueType::Boolean, TEXT("IA_DecreaseSail_Auto"));
	MakeIA(IA_Turn, EInputActionValueType::Axis1D, TEXT("IA_Turn_Auto"));
	MakeIA(IA_FireLeft, EInputActionValueType::Boolean, TEXT("IA_FireLeft_Auto"));
	MakeIA(IA_FireRight, EInputActionValueType::Boolean, TEXT("IA_FireRight_Auto"));
	MakeIA(IA_Fire, EInputActionValueType::Boolean, TEXT("IA_Fire_Auto"));
	MakeIA(IA_Aim, EInputActionValueType::Boolean, TEXT("IA_Aim_Auto"));
	MakeIA(IA_Look, EInputActionValueType::Axis2D, TEXT("IA_Look_Auto"));

	if (!ShipMappingContext)
	{
		ShipMappingContext = NewObject<UInputMappingContext>(this, UInputMappingContext::StaticClass(), TEXT("IMC_Ship_Auto"));
		ShipMappingContext->MapKey(IA_IncreaseSail, EKeys::W);
		ShipMappingContext->MapKey(IA_DecreaseSail, EKeys::S);
		ShipMappingContext->MapKey(IA_Turn, EKeys::D);
		FEnhancedActionKeyMapping& TurnLeftMap = ShipMappingContext->MapKey(IA_Turn, EKeys::A);
		TurnLeftMap.Modifiers.Add(NewObject<UInputModifierNegate>(ShipMappingContext));

		ShipMappingContext->MapKey(IA_FireLeft, EKeys::Q);
		ShipMappingContext->MapKey(IA_FireRight, EKeys::E);
		ShipMappingContext->MapKey(IA_Fire, EKeys::LeftMouseButton);
		ShipMappingContext->MapKey(IA_Aim, EKeys::RightMouseButton);
		ShipMappingContext->MapKey(IA_Look, EKeys::Mouse2D);
	}
}

void AShipPawn::AddInputMappingContext()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	ULocalPlayer* LP = PC->GetLocalPlayer();
	if (!LP) return;

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP);
	if (!Subsystem) return;

	Subsystem->ClearAllMappings();

	UInputMappingContext* Overlay = NewObject<UInputMappingContext>(this, UInputMappingContext::StaticClass(), TEXT("IMC_Ship_Canonical"));
	if (IA_IncreaseSail) Overlay->MapKey(IA_IncreaseSail, EKeys::W);
	if (IA_DecreaseSail) Overlay->MapKey(IA_DecreaseSail, EKeys::S);
	if (IA_Turn)
	{
		Overlay->MapKey(IA_Turn, EKeys::D);
		FEnhancedActionKeyMapping& MA = Overlay->MapKey(IA_Turn, EKeys::A);
		MA.Modifiers.Add(NewObject<UInputModifierNegate>(Overlay));
	}
	if (IA_FireLeft)  Overlay->MapKey(IA_FireLeft, EKeys::Q);
	if (IA_FireRight) Overlay->MapKey(IA_FireRight, EKeys::E);
	if (IA_Fire) Overlay->MapKey(IA_Fire, EKeys::LeftMouseButton);
	if (IA_Aim) Overlay->MapKey(IA_Aim, EKeys::RightMouseButton);
	if (IA_Look) Overlay->MapKey(IA_Look, EKeys::Mouse2D);

	Subsystem->AddMappingContext(Overlay, 0);
}

void AShipPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (SpeedPenaltyTimeRemaining > 0.0f)
	{
		SpeedPenaltyTimeRemaining -= DeltaTime;
		if (SpeedPenaltyTimeRemaining <= 0.0f)
		{
			SpeedPenaltyTimeRemaining = 0.0f;
			SpeedPenaltyFraction = 0.0f;
		}
	}

	PollRawInputFallback(DeltaTime);
	UpdateAiming(DeltaTime);
	UpdateMovement(DeltaTime);

	if (!FMath::IsNearlyZero(TurnInputValue))
	{
		float SpeedFraction = MaxSpeed > 0.0f ? CurrentSpeed / MaxSpeed : 0.0f;
		float TurnRate = FMath::Lerp(BaseTurnRate, BaseTurnRate * HighSpeedTurnFactor, SpeedFraction);
		AddActorLocalRotation(FRotator(0.0f, TurnInputValue * TurnRate * DeltaTime, 0.0f));
	}

	UpdateVisualRoll(DeltaTime);
	UpdateBowWake();
}

void AShipPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	EnsureInputAssetsExist();

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (EIC)
	{
		if (IA_IncreaseSail) EIC->BindAction(IA_IncreaseSail, ETriggerEvent::Started, this, &AShipPawn::Input_IncreaseSail);
		if (IA_DecreaseSail) EIC->BindAction(IA_DecreaseSail, ETriggerEvent::Started, this, &AShipPawn::Input_DecreaseSail);
		if (IA_Turn)
		{
			EIC->BindAction(IA_Turn, ETriggerEvent::Triggered, this, &AShipPawn::Input_Turn);
			EIC->BindAction(IA_Turn, ETriggerEvent::Completed, this, &AShipPawn::Input_TurnCompleted);
			EIC->BindAction(IA_Turn, ETriggerEvent::Canceled, this, &AShipPawn::Input_TurnCompleted);
		}
		if (IA_FireLeft)  EIC->BindAction(IA_FireLeft, ETriggerEvent::Started, this, &AShipPawn::Input_FireLeft);
		if (IA_FireRight) EIC->BindAction(IA_FireRight, ETriggerEvent::Started, this, &AShipPawn::Input_FireRight);
		if (IA_Fire)      EIC->BindAction(IA_Fire, ETriggerEvent::Started, this, &AShipPawn::Input_Fire);

		if (IA_Aim)
		{
			EIC->BindAction(IA_Aim, ETriggerEvent::Started, this, &AShipPawn::Input_AimStart);
			EIC->BindAction(IA_Aim, ETriggerEvent::Completed, this, &AShipPawn::Input_AimStop);
			EIC->BindAction(IA_Aim, ETriggerEvent::Canceled, this, &AShipPawn::Input_AimStop);
		}

		if (IA_Look) EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &AShipPawn::Input_Look);

		bEnhancedInputReady = true;
	}
	else
	{
		bEnhancedInputReady = false;
	}
}

float AShipPawn::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float MitigatedDamage = DamageAmount * (1.0f - ArmorReduction);
	float Actual = Super::TakeDamage(MitigatedDamage, DamageEvent, EventInstigator, DamageCauser);

	if (MitigatedDamage > 0.0f && HitCameraShake)
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			const float MaxHP = HealthComponent ? HealthComponent->GetMaxHealth() : 200.0f;
			const float Scale = FMath::Clamp(HitCameraShakeScale * (MitigatedDamage / MaxHP) * 5.0f, 0.3f, HitCameraShakeScale * 2.0f);
			PC->ClientStartCameraShake(HitCameraShake, Scale);
		}
	}
	return Actual;
}

void AShipPawn::Input_IncreaseSail(const FInputActionValue&) { if (ConsumeActionCooldown(TEXT("IncSail"))) DoIncreaseSail(); }
void AShipPawn::Input_DecreaseSail(const FInputActionValue&) { if (ConsumeActionCooldown(TEXT("DecSail"))) DoDecreaseSail(); }
void AShipPawn::Input_Turn(const FInputActionValue& Value) { DoSetTurnAxis(Value.Get<float>()); }
void AShipPawn::Input_TurnCompleted(const FInputActionValue&) { DoSetTurnAxis(0.0f); }
void AShipPawn::Input_FireLeft(const FInputActionValue&) { if (ConsumeActionCooldown(TEXT("FireL"), 0.15f)) DoFireLeft(); }
void AShipPawn::Input_FireRight(const FInputActionValue&) { if (ConsumeActionCooldown(TEXT("FireR"), 0.15f)) DoFireRight(); }
void AShipPawn::Input_Fire(const FInputActionValue&) { if (ConsumeActionCooldown(TEXT("FireCam"), 0.15f)) DoCameraAimFire(); }
void AShipPawn::Input_AimStart(const FInputActionValue&) { bIsAiming = true; }
void AShipPawn::Input_AimStop(const FInputActionValue&) { bIsAiming = false; }
void AShipPawn::Input_Look(const FInputActionValue& Value) { DoLook(Value.Get<FVector2D>()); }

bool AShipPawn::ConsumeActionCooldown(FName ActionTag, float CooldownSec)
{
	UWorld* W = GetWorld();
	if (!W) return true;
	const float Now = W->GetTimeSeconds();
	if (const float* Last = ActionFireTimes.Find(ActionTag))
	{
		if (Now - *Last < CooldownSec) return false;
	}
	ActionFireTimes.Add(ActionTag, Now);
	return true;
}

void AShipPawn::DoIncreaseSail()
{
	switch (CurrentSailLevel)
	{
	case ESailLevel::Stop:     CurrentSailLevel = ESailLevel::HalfSail; break;
	case ESailLevel::HalfSail: CurrentSailLevel = ESailLevel::FullSail; break;
	default: break;
	}
}

void AShipPawn::DoDecreaseSail()
{
	switch (CurrentSailLevel)
	{
	case ESailLevel::FullSail: CurrentSailLevel = ESailLevel::HalfSail; break;
	case ESailLevel::HalfSail: CurrentSailLevel = ESailLevel::Stop;     break;
	default: break;
	}
}

void AShipPawn::DoSetTurnAxis(float Value)
{
	TurnInputValue = FMath::Clamp(Value, -1.0f, 1.0f);
}

void AShipPawn::DoFireLeft() { if (CannonComponent) CannonComponent->FireBroadside(ECannonSide::Left); }
void AShipPawn::DoFireRight() { if (CannonComponent) CannonComponent->FireBroadside(ECannonSide::Right); }

void AShipPawn::DoLook(const FVector2D& Delta)
{
	LookYawOffset = FRotator::NormalizeAxis(LookYawOffset + Delta.X * LookYawSensitivity);
	LookPitchOffset = FMath::Clamp(LookPitchOffset - Delta.Y * LookPitchSensitivity, LookPitchMin, LookPitchMax);

	if (SpringArm)
	{
		FRotator Rel = SpringArm->GetRelativeRotation();
		Rel.Yaw = LookYawOffset;
		Rel.Pitch = LookPitchOffset - 25.0f;
		SpringArm->SetRelativeRotation(Rel);
	}
}

void AShipPawn::DoCameraAimFire()
{
	if (!CannonComponent || !Camera) return;

	if (bIsAiming)
	{
		CannonComponent->FireBroadside(AimingSide);
	}
	else
	{
		const FVector CamFwd = Camera->GetForwardVector();
		const float DotRight = FVector::DotProduct(CamFwd, GetActorRightVector());

		if (DotRight > 0.35f) CannonComponent->FireBroadside(ECannonSide::Right);
		else if (DotRight < -0.35f) CannonComponent->FireBroadside(ECannonSide::Left);
	}
}

void AShipPawn::PollRawInputFallback(float DeltaTime)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	if (!bEnhancedInputReady)
	{
		bIsAiming = PC->IsInputKeyDown(EKeys::RightMouseButton);
	}

	const bool bW = PC->IsInputKeyDown(EKeys::W) || PC->IsInputKeyDown(EKeys::Up);
	if (bW && !bRawPrevW && ConsumeActionCooldown(TEXT("IncSail"), 0.25f)) DoIncreaseSail();
	bRawPrevW = bW;

	const bool bS = PC->IsInputKeyDown(EKeys::S) || PC->IsInputKeyDown(EKeys::Down);
	if (bS && !bRawPrevS && ConsumeActionCooldown(TEXT("DecSail"), 0.25f)) DoDecreaseSail();
	bRawPrevS = bS;

	const bool bQ = PC->IsInputKeyDown(EKeys::Q);
	if (bQ && !bRawPrevQ && ConsumeActionCooldown(TEXT("FireL"), 0.15f)) DoFireLeft();
	bRawPrevQ = bQ;

	const bool bE = PC->IsInputKeyDown(EKeys::E);
	if (bE && !bRawPrevE && ConsumeActionCooldown(TEXT("FireR"), 0.15f)) DoFireRight();
	bRawPrevE = bE;

	const bool bFire = PC->IsInputKeyDown(EKeys::LeftMouseButton) || PC->IsInputKeyDown(EKeys::SpaceBar);
	if (bFire && !bRawPrevFire && ConsumeActionCooldown(TEXT("FireCam"), 0.15f)) DoCameraAimFire();
	bRawPrevFire = bFire;

	float Raw = 0.0f;
	if (PC->IsInputKeyDown(EKeys::D) || PC->IsInputKeyDown(EKeys::Right)) Raw += 1.0f;
	if (PC->IsInputKeyDown(EKeys::A) || PC->IsInputKeyDown(EKeys::Left))  Raw -= 1.0f;
	if (!bEnhancedInputReady || FMath::IsNearlyZero(TurnInputValue)) DoSetTurnAxis(Raw);

	float MX = 0.f, MY = 0.f;
	PC->GetInputMouseDelta(MX, MY);
	if (!bEnhancedInputReady && (!FMath::IsNearlyZero(MX) || !FMath::IsNearlyZero(MY)))
	{
		DoLook(FVector2D(MX, MY));
	}
}

void AShipPawn::UpdateAiming(float DeltaTime)
{
	if (!CannonComponent || !Camera || !SpringArm) return;

	CannonComponent->SetIsAiming(bIsAiming);

	if (bIsAiming)
	{
		// 1. Приближуємо камеру, але залишаємо їй вільний огляд
		SpringArm->TargetArmLength = FMath::FInterpTo(SpringArm->TargetArmLength, AimSpringArmLength, DeltaTime, AimZoomSpeed);

		// 2. Визначаємо активний борт за азимутом камери
		FVector CamFwd = Camera->GetForwardVector();
		float DotRight = FVector::DotProduct(CamFwd, GetActorRightVector());
		AimingSide = (DotRight > 0.0f) ? ECannonSide::Right : ECannonSide::Left;

		// 3. Відв'язуємо дистанцію прицілу від прямого променя
		// Переводимо нахил камери (Pitch) у дистанцію по воді. 
		FVector CamFwd2D = CamFwd.GetSafeNormal2D();
		float Pitch = SpringArm->GetRelativeRotation().Pitch;

		// MapRange: чим вище дивиться камера (ближче до -5), тим далі ціль
		float NormalizedPitch = FMath::GetMappedRangeValueClamped(FVector2D(-60.0f, -5.0f), FVector2D(0.0f, 1.0f), Pitch);
		float TargetDist = FMath::Lerp(500.0f, CannonComponent->MaxRange, NormalizedPitch);

		FVector TargetLoc = GetActorLocation() + CamFwd2D * TargetDist;
		TargetLoc.Z = 0.0f; // Приціл завжди на площині води

		CannonComponent->UpdateAimTarget(AimingSide, TargetLoc);
	}
	else
	{
		SpringArm->TargetArmLength = FMath::FInterpTo(SpringArm->TargetArmLength, DefaultSpringArmLength, DeltaTime, AimZoomSpeed);
	}
}

float AShipPawn::GetTargetSpeed() const
{
	float SailMult = 0.0f;
	switch (CurrentSailLevel)
	{
	case ESailLevel::HalfSail: SailMult = HalfSailSpeedMultiplier; break;
	case ESailLevel::FullSail: SailMult = 1.0f;                    break;
	default:                   SailMult = 0.0f;                    break;
	}

	float Penalty = FMath::Clamp(1.0f - SpeedPenaltyFraction, 0.0f, 1.0f);
	return MaxSpeed * SailMult * Penalty * GetWindMultiplier();
}

float AShipPawn::GetWindMultiplier() const
{
	if (!CachedWindSystem) return 1.0f;
	float Dot = CachedWindSystem->GetWindSpeedMultiplier(GetActorForwardVector());
	return 1.0f + (Dot * WindInfluenceFactor);
}

void AShipPawn::UpdateMovement(float DeltaTime)
{
	float Target = GetTargetSpeed();
	if (CurrentSpeed < Target)
		CurrentSpeed = FMath::Min(CurrentSpeed + AccelerationRate * DeltaTime, Target);
	else if (CurrentSpeed > Target)
		CurrentSpeed = FMath::Max(CurrentSpeed - DecelerationRate * DeltaTime, Target);

	if (CurrentSpeed > 0.01f)
	{
		AddActorWorldOffset(GetActorForwardVector() * CurrentSpeed * DeltaTime, false);
	}
}

void AShipPawn::UpdateVisualRoll(float DeltaTime)
{
	if (!ShipMesh) return;

	float TargetRoll = -TurnInputValue * MaxVisualRoll;
	CurrentVisualRoll = FMath::FInterpTo(CurrentVisualRoll, TargetRoll, DeltaTime, VisualRollInterpSpeed);

	FRotator LocalRot = ShipMesh->GetRelativeRotation();
	LocalRot.Roll = CurrentVisualRoll;
	ShipMesh->SetRelativeRotation(LocalRot);
}

void AShipPawn::ApplySpeedPenalty(float PenaltyFraction, float Duration)
{
	SpeedPenaltyFraction = FMath::Clamp(PenaltyFraction, 0.0f, 1.0f);
	SpeedPenaltyTimeRemaining = Duration;
}

void AShipPawn::UpgradeMaxSpeed(float BonusSpeed) { MaxSpeed += BonusSpeed; }
void AShipPawn::UpgradeTurnRate(float BonusTurnRate) { BaseTurnRate += BonusTurnRate; }
void AShipPawn::UpgradeHullArmor(float AdditionalReductionPct)
{
	ArmorReduction = FMath::Clamp(ArmorReduction + AdditionalReductionPct * 0.01f, 0.0f, 0.75f);
}

void AShipPawn::UpdateDamageFX()
{
	if (!HealthComponent) return;
	const float Pct = HealthComponent->GetHealthPercent();

	if (DamageSmokeFX)
	{
		const bool bShouldSmoke = (Pct < SmokeHPThreshold) && HealthComponent->IsAlive();
		if (bShouldSmoke && !DamageSmokeFX->IsActive()) DamageSmokeFX->Activate(true);
		else if (!bShouldSmoke && DamageSmokeFX->IsActive()) DamageSmokeFX->Deactivate();
	}

	if (DamageFireFX)
	{
		const bool bShouldBurn = (Pct < FireHPThreshold) && HealthComponent->IsAlive();
		if (bShouldBurn && !DamageFireFX->IsActive()) DamageFireFX->Activate(true);
		else if (!bShouldBurn && DamageFireFX->IsActive()) DamageFireFX->Deactivate();
	}
}

void AShipPawn::UpdateBowWake()
{
	if (!BowWakeFX) return;
	const bool bMoving = CurrentSpeed > 50.0f;
	if (bMoving && !BowWakeFX->IsActive()) BowWakeFX->Activate(true);
	else if (!bMoving && BowWakeFX->IsActive()) BowWakeFX->Deactivate();

	if (BowWakeFX->IsActive())
	{
		const float SpeedNorm = MaxSpeed > 0 ? FMath::Clamp(CurrentSpeed / MaxSpeed, 0.f, 1.f) : 0.f;
		BowWakeFX->SetVariableFloat(TEXT("SpeedScale"), SpeedNorm);
	}
}

void AShipPawn::HandleHealthChanged(float, float) { UpdateDamageFX(); }

void AShipPawn::HandleDeath()
{
	UWorld* World = GetWorld();
	if (World)
	{
		const FVector Loc = GetActorLocation();
		if (DeathExplosionAsset) UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, DeathExplosionAsset, Loc, GetActorRotation());
		if (DeathSound) UGameplayStatics::PlaySoundAtLocation(World, DeathSound, Loc);
	}

	if (DamageSmokeFX) DamageSmokeFX->Deactivate();
	if (DamageFireFX)  DamageFireFX->Deactivate();
	if (BowWakeFX)     BowWakeFX->Deactivate();
}