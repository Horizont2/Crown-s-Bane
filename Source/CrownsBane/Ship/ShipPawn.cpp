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
#include "Player/CrownsBanePlayerController.h"
#include "UI/CrownsBaneHUD.h"
#include "Player/ShipProgressionComponent.h"
#include "Audio/SoundManager.h"
#include "AI/EnemyShipBase.h"
#include "EngineUtils.h"
#include "Docks/DocksZone.h"
#include "Player/PlayerInventory.h"
#include "Loot/ResourceTypes.h"
#include "Combat/Cannonball.h"
#include "Engine/DamageEvents.h"

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
	MakeIA(IA_ToggleDocks, EInputActionValueType::Boolean, TEXT("IA_ToggleDocks_Auto"));
	MakeIA(IA_QuestLog,    EInputActionValueType::Boolean, TEXT("IA_QuestLog_Auto"));
	MakeIA(IA_Board,       EInputActionValueType::Boolean, TEXT("IA_Board_Auto"));
	MakeIA(IA_Trader,      EInputActionValueType::Boolean, TEXT("IA_Trader_Auto"));
	MakeIA(IA_LockOn,      EInputActionValueType::Boolean, TEXT("IA_LockOn_Auto"));
	MakeIA(IA_Brace,       EInputActionValueType::Boolean, TEXT("IA_Brace_Auto"));
	MakeIA(IA_DropAnchor,  EInputActionValueType::Boolean, TEXT("IA_DropAnchor_Auto"));
	MakeIA(IA_Pause,       EInputActionValueType::Boolean, TEXT("IA_Pause_Auto"));
	MakeIA(IA_Help,        EInputActionValueType::Boolean, TEXT("IA_Help_Auto"));
	MakeIA(IA_Settings,    EInputActionValueType::Boolean, TEXT("IA_Settings_Auto"));

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
		ShipMappingContext->MapKey(IA_ToggleDocks, EKeys::U);
		ShipMappingContext->MapKey(IA_QuestLog,    EKeys::J);
		ShipMappingContext->MapKey(IA_Board,       EKeys::F);
		ShipMappingContext->MapKey(IA_Trader,      EKeys::T);
		ShipMappingContext->MapKey(IA_LockOn,      EKeys::Tab);
		ShipMappingContext->MapKey(IA_Brace,       EKeys::B);
		ShipMappingContext->MapKey(IA_DropAnchor,  EKeys::V);
		ShipMappingContext->MapKey(IA_Pause,       EKeys::Escape);
		ShipMappingContext->MapKey(IA_Help,        EKeys::F1);
		ShipMappingContext->MapKey(IA_Settings,    EKeys::F2);
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
	if (IA_ToggleDocks) Overlay->MapKey(IA_ToggleDocks, EKeys::U);
	if (IA_QuestLog)    Overlay->MapKey(IA_QuestLog,    EKeys::J);
	if (IA_Board)       Overlay->MapKey(IA_Board,       EKeys::F);
	if (IA_Trader)      Overlay->MapKey(IA_Trader,      EKeys::T);
	if (IA_LockOn)      Overlay->MapKey(IA_LockOn,      EKeys::Tab);
	if (IA_Brace)       Overlay->MapKey(IA_Brace,       EKeys::B);
	if (IA_DropAnchor)  Overlay->MapKey(IA_DropAnchor,  EKeys::V);
	if (IA_Pause)       Overlay->MapKey(IA_Pause,       EKeys::Escape);
	if (IA_Help)        Overlay->MapKey(IA_Help,        EKeys::F1);
	if (IA_Settings)    Overlay->MapKey(IA_Settings,    EKeys::F2);

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

	if (bIsSinking)
	{
		TickSinking(DeltaTime);
		return;
	}

	PollRawInputFallback(DeltaTime);
	UpdateAiming(DeltaTime);
	UpdateMovement(DeltaTime);
	UpdateBoardingTarget();
	UpdateSharkHazard(DeltaTime);

	// First-time tutorial tip: ANY enemy within 3000cm.
	{
		bool bAnyEnemyClose = false;
		const FVector MyLoc = GetActorLocation();
		for (TActorIterator<AEnemyShipBase> It(GetWorld()); It; ++It)
		{
			if (*It && (*It)->HealthComponent && (*It)->HealthComponent->IsAlive() &&
			    FVector::DistSquared(MyLoc, (*It)->GetActorLocation()) < 3000.f * 3000.f)
			{
				bAnyEnemyClose = true; break;
			}
		}
		if (bAnyEnemyClose)
		{
			TryShowTip(this, TEXT("FirstLockOn"),
				TEXT("Enemy in range. Press [TAB] to lock on, [RMB] to aim."));
		}
	}

	// While bracing, ship cannot fire (handled by gating in DoCameraAimFire below).

	// Ramming check: if any enemy is within 500cm AND we're moving fast enough,
	// apply mutual damage scaled by our speed.  RamCooldown prevents per-frame spam.
	RamCooldown = FMath::Max(0.0f, RamCooldown - DeltaTime);
	if (RamCooldown <= 0.0f && CurrentSpeed >= RamMinSpeedForDamage)
	{
		for (TActorIterator<AEnemyShipBase> It(GetWorld()); It; ++It)
		{
			AEnemyShipBase* E = *It;
			if (!E || !E->HealthComponent || !E->HealthComponent->IsAlive()) continue;
			const float D = FVector::Dist(GetActorLocation(), E->GetActorLocation());
			if (D > 500.0f) continue;

			const float RamDmg = CurrentSpeed * RamDamageScale;
			FDamageEvent DmgEvent;
			E->TakeDamage(RamDmg, DmgEvent, GetController(), this);
			if (HealthComponent)
			{
				HealthComponent->TakeDamage(RamDmg * RamSelfDamageFraction);
			}
			// Push both ships apart so we don't immediately re-trigger.
			const FVector Sep = (GetActorLocation() - E->GetActorLocation()).GetSafeNormal2D();
			E->AddActorWorldOffset(-Sep * 200.0f, false);
			AddActorWorldOffset(Sep * 100.0f, false);
			CurrentSpeed *= 0.4f;
			RamCooldown = 1.2f;
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Red,
				FString::Printf(TEXT("RAM! %.0f dmg / -%.0f self"), RamDmg, RamDmg * RamSelfDamageFraction));
			break;
		}
	}

	// Boarding QTE countdown.  Real-time DeltaTime (not dilated) preferred but
	// regular DeltaTime is fine because the player can re-board on failure.
	if (bBoardingActive)
	{
		BoardingQTETimeRemaining -= DeltaTime;
		if (BoardingQTETimeRemaining <= 0.0f)
		{
			bBoardingActive = false;
			BoardingQTETarget = nullptr;
			if (GEngine) GEngine->AddOnScreenDebugMessage(8888, 2.5f, FColor::Red,
				TEXT("BOARDING FAILED — crew retreated."));
		}
	}

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
		if (IA_ToggleDocks) EIC->BindAction(IA_ToggleDocks, ETriggerEvent::Started, this, &AShipPawn::Input_ToggleDocks);
		if (IA_QuestLog)    EIC->BindAction(IA_QuestLog,    ETriggerEvent::Started, this, &AShipPawn::Input_ToggleQuestLog);
		if (IA_Board)       EIC->BindAction(IA_Board,       ETriggerEvent::Started, this, &AShipPawn::Input_Board);
		if (IA_Trader)      EIC->BindAction(IA_Trader,      ETriggerEvent::Started, this, &AShipPawn::Input_Trader);
		if (IA_LockOn)      EIC->BindAction(IA_LockOn,      ETriggerEvent::Started, this, &AShipPawn::Input_LockOn);
		if (IA_Brace)
		{
			EIC->BindAction(IA_Brace, ETriggerEvent::Started,   this, &AShipPawn::Input_BraceStart);
			EIC->BindAction(IA_Brace, ETriggerEvent::Completed, this, &AShipPawn::Input_BraceStop);
			EIC->BindAction(IA_Brace, ETriggerEvent::Canceled,  this, &AShipPawn::Input_BraceStop);
		}
		if (IA_DropAnchor)  EIC->BindAction(IA_DropAnchor,  ETriggerEvent::Started, this, &AShipPawn::Input_DropAnchor);
		if (IA_Pause)       EIC->BindAction(IA_Pause,       ETriggerEvent::Started, this, &AShipPawn::Input_Pause);
		if (IA_Help)        EIC->BindAction(IA_Help,        ETriggerEvent::Started, this, &AShipPawn::Input_Help);
		if (IA_Settings)    EIC->BindAction(IA_Settings,    ETriggerEvent::Started, this, &AShipPawn::Input_Settings);

		bEnhancedInputReady = true;
	}
	else
	{
		bEnhancedInputReady = false;
	}
}

float AShipPawn::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float BraceMul = bBracing ? (1.0f - BraceDamageReduction) : 1.0f;
	const float MitigatedDamage = DamageAmount * (1.0f - ArmorReduction) * BraceMul;
	float Actual = Super::TakeDamage(MitigatedDamage, DamageEvent, EventInstigator, DamageCauser);

	// Sail damage: Heavy / Explosive shots tear into the rigging.
	if (DamageCauser)
	{
		if (ACannonball* Ball = Cast<ACannonball>(DamageCauser))
		{
			if (Ball->CannonballData.Type == ECannonballType::Heavy ||
			    Ball->CannonballData.Type == ECannonballType::Explosive)
			{
				SailIntegrity = FMath::Clamp(SailIntegrity - SailDamagePerHeavyHit, 0.10f, 1.0f);
			}
		}
	}

	// Damage-direction marker for HUD.
	if (MitigatedDamage > 0.0f && DamageCauser)
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			if (ACrownsBaneHUD* HUD = Cast<ACrownsBaneHUD>(PC->GetHUD()))
			{
				const FVector Dir = (DamageCauser->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
				const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X));
				HUD->RegisterPlayerDamageFrom(Yaw);
			}
		}
	}

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

void AShipPawn::Input_ToggleDocks(const FInputActionValue&)
{
	if (ACrownsBanePlayerController* PC = Cast<ACrownsBanePlayerController>(GetController()))
	{
		PC->ToggleUpgradeUI();
	}
}

void AShipPawn::Input_ToggleQuestLog(const FInputActionValue&)
{
	if (ACrownsBanePlayerController* PC = Cast<ACrownsBanePlayerController>(GetController()))
	{
		PC->ToggleQuestLog();
	}
}

void AShipPawn::Input_Board(const FInputActionValue&)
{
	ExecuteBoarding();
}

void AShipPawn::Input_Trader(const FInputActionValue&)
{
	if (ACrownsBanePlayerController* PC = Cast<ACrownsBanePlayerController>(GetController()))
	{
		PC->ToggleTraderMenu();
	}
}

void AShipPawn::Input_LockOn(const FInputActionValue&)
{
	CycleLockOnTarget();
}

void AShipPawn::Input_BraceStart(const FInputActionValue&) { bBracing = true; }
void AShipPawn::Input_BraceStop (const FInputActionValue&) { bBracing = false; }

void AShipPawn::Input_DropAnchor(const FInputActionValue&)
{
	// Instant hard stop — speed, target speed, and sail level all snap to zero.
	CurrentSpeed = 0.0f;
	CurrentSailLevel = ESailLevel::Stop;
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Cyan, TEXT("ANCHOR DROPPED"));
}

void AShipPawn::Input_Pause(const FInputActionValue&)
{
	if (ACrownsBanePlayerController* PC = Cast<ACrownsBanePlayerController>(GetController()))
	{
		PC->TogglePauseMenu();
	}
}

void AShipPawn::Input_Help(const FInputActionValue&)
{
	if (ACrownsBaneHUD* HUD = Cast<ACrownsBaneHUD>(GetWorld()->GetFirstPlayerController()->GetHUD()))
	{
		HUD->bShowHelpOverlay = !HUD->bShowHelpOverlay;
	}
}

void AShipPawn::Input_Settings(const FInputActionValue&)
{
	if (ACrownsBaneHUD* HUD = Cast<ACrownsBaneHUD>(GetWorld()->GetFirstPlayerController()->GetHUD()))
	{
		HUD->PauseSubTab = (HUD->PauseSubTab == 1) ? 0 : 1;
	}
}

void AShipPawn::CycleLockOnTarget()
{
	UWorld* W = GetWorld();
	if (!W) return;

	// Collect every alive enemy within LockOnMaxRange, sorted by camera-relative yaw.
	TArray<AEnemyShipBase*> Candidates;
	const FVector MyLoc = GetActorLocation();
	const float MaxR2 = LockOnMaxRange * LockOnMaxRange;
	for (TActorIterator<AEnemyShipBase> It(W); It; ++It)
	{
		AEnemyShipBase* E = *It;
		if (!E || !E->HealthComponent || !E->HealthComponent->IsAlive()) continue;
		if (FVector::DistSquared(MyLoc, E->GetActorLocation()) > MaxR2) continue;
		Candidates.Add(E);
	}
	if (Candidates.Num() == 0) { LockedTarget = nullptr; return; }

	// Sort by clockwise yaw from camera forward so Tab cycles in a natural ring.
	const FVector CamFwd = Camera ? Camera->GetForwardVector() : GetActorForwardVector();
	Candidates.Sort([this, &MyLoc, &CamFwd](const AEnemyShipBase& A, const AEnemyShipBase& B)
	{
		const FVector DA = (A.GetActorLocation() - MyLoc).GetSafeNormal2D();
		const FVector DB = (B.GetActorLocation() - MyLoc).GetSafeNormal2D();
		const float AA = FMath::Atan2(FVector::CrossProduct(CamFwd, DA).Z, FVector::DotProduct(CamFwd, DA));
		const float AB = FMath::Atan2(FVector::CrossProduct(CamFwd, DB).Z, FVector::DotProduct(CamFwd, DB));
		return AA < AB;
	});

	// Find current locked target in list and advance by one; if not present, lock first.
	int32 Idx = Candidates.IndexOfByKey(LockedTarget);
	LockedTarget = Candidates[(Idx + 1) % Candidates.Num()];
}

static void TryShowTip(AShipPawn* Ship, const FString& Key, const FString& Text)
{
	if (!Ship) return;
	if (APlayerController* PC = Cast<APlayerController>(Ship->GetController()))
	{
		if (ACrownsBaneHUD* HUD = Cast<ACrownsBaneHUD>(PC->GetHUD()))
		{
			HUD->ShowTipOnce(Key, Text);
		}
	}
}

void AShipPawn::UpdateBoardingTarget()
{
	UWorld* W = GetWorld();
	if (!W) { CurrentBoardingTarget = nullptr; return; }

	const FVector MyLoc = GetActorLocation();
	AEnemyShipBase* Best = nullptr;
	float BestDist2 = BoardingDistance * BoardingDistance;

	for (TActorIterator<AEnemyShipBase> It(W); It; ++It)
	{
		AEnemyShipBase* E = *It;
		if (!E || !E->HealthComponent) continue;
		if (!E->HealthComponent->IsAlive()) continue;
		if (E->HealthComponent->GetHealthPercent() > BoardableHealthThreshold) continue;

		const float D2 = FVector::DistSquared(MyLoc, E->GetActorLocation());
		if (D2 < BestDist2)
		{
			BestDist2 = D2;
			Best = E;
		}
	}
	CurrentBoardingTarget = Best;

	// Tutorial: first time a boardable target shows up.
	if (Best)
	{
		TryShowTip(this, TEXT("FirstBoard"),
			TEXT("Press [F] to board crippled enemies for bonus loot."));
	}
}

void AShipPawn::ExecuteBoarding()
{
	if (bBoardingActive) return;                             // already in QTE
	if (!CurrentBoardingTarget || !CurrentBoardingTarget->HealthComponent) return;

	// Start the QTE — player must mash SPACE before the timer runs out.
	bBoardingActive = true;
	BoardingQTETarget = CurrentBoardingTarget;
	BoardingQTEHits = 0;
	BoardingQTETimeRemaining = BoardingQTEDuration;
	if (GEngine && bShowDebugOnScreen)
	{
		GEngine->AddOnScreenDebugMessage(8888, 1.5f, FColor::Orange,
			TEXT("BOARDING — mash SPACE to subdue the crew!"));
	}
}

void AShipPawn::RegisterBoardingQTEPress()
{
	if (!bBoardingActive || !BoardingQTETarget) return;
	BoardingQTEHits++;
	if (BoardingQTEHits >= BoardingQTERequiredHits)
	{
		// SUCCESS — instant kill + loot multiplier.
		if (BoardingQTETarget->HealthComponent)
		{
			const float Dmg = BoardingQTETarget->HealthComponent->GetCurrentHealth() + 1.0f;
			FDamageEvent DmgEvent;
			BoardingQTETarget->TakeDamage(Dmg, DmgEvent, GetController(), this);
		}
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(8888, 3.f, FColor::Green,
				FString::Printf(TEXT("BOARDING WON — loot x%.1f"), BoardingLootMultiplier));
		}
		bBoardingActive = false;
		BoardingQTETarget = nullptr;
		CurrentBoardingTarget = nullptr;
	}
}

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
	const float SensScale = bIsAiming ? AimLookSensitivityScale : 1.0f;
	LookYawOffset = FRotator::NormalizeAxis(LookYawOffset + Delta.X * LookYawSensitivity * SensScale);
	// Mouse-up should pitch the camera UP.  UE's Mouse2D delivers positive Y on
	// mouse-up, so we ADD (not subtract) Delta.Y here.  bInvertMouseY restores
	// the legacy/inverted behaviour for players who prefer it.
	const float PitchSign = bInvertMouseY ? -1.0f : 1.0f;
	LookPitchOffset = FMath::Clamp(LookPitchOffset + PitchSign * Delta.Y * LookPitchSensitivity * SensScale, LookPitchMin, LookPitchMax);

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
	if (bBracing) return; // Brace blocks fire

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

	// Boarding QTE — every fresh SpaceBar press counts as one mash.
	{
		const bool bSpaceNow = PC->IsInputKeyDown(EKeys::SpaceBar);
		if (bBoardingActive && bSpaceNow && !bQTEPrevSpace)
		{
			RegisterBoardingQTEPress();
		}
		bQTEPrevSpace = bSpaceNow;
	}

	// Menu hotkeys (number row 1-7).  Active only while the relevant menu is
	// open, so they never accidentally fire during sailing.
	if (ACrownsBanePlayerController* CBPC = Cast<ACrownsBanePlayerController>(PC))
	{
		static const FKey NumKeys[] = {
			EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four,
			EKeys::Five, EKeys::Six, EKeys::Seven
		};
		if (CBPC->IsUpgradeUIOpen())
		{
			for (int32 i = 0; i < UE_ARRAY_COUNT(NumKeys); ++i)
			{
				if (PC->IsInputKeyDown(NumKeys[i]) &&
				    ConsumeActionCooldown(FName(*FString::Printf(TEXT("Upg%d"), i)), 0.30f))
				{
					CBPC->BuyUpgrade((uint8)i);
				}
			}
		}
		if (CBPC->IsTraderMenuOpen())
		{
			if (PC->IsInputKeyDown(NumKeys[0]) && ConsumeActionCooldown(TEXT("TrBuy1"), 0.30f)) CBPC->BuyAmmo();
			if (PC->IsInputKeyDown(NumKeys[1]) && ConsumeActionCooldown(TEXT("TrBuy2"), 0.30f)) CBPC->BuyWood();
			if (PC->IsInputKeyDown(NumKeys[2]) && ConsumeActionCooldown(TEXT("TrBuy3"), 0.30f)) CBPC->BuyMetal();
			if (PC->IsInputKeyDown(NumKeys[3]) && ConsumeActionCooldown(TEXT("TrSel4"), 0.30f)) CBPC->SellWood();
			if (PC->IsInputKeyDown(NumKeys[4]) && ConsumeActionCooldown(TEXT("TrSel5"), 0.30f)) CBPC->SellMetal();
			if (PC->IsInputKeyDown(NumKeys[5]) && ConsumeActionCooldown(TEXT("TrHeal6"), 0.30f)) CBPC->PayForHeal();
		}

		// Perk choice hotkeys 1/2/3 — only when overlay is active.
		if (CBPC->Progression && CBPC->Progression->bPerkChoicePending)
		{
			if (ACrownsBaneHUD* HUD = Cast<ACrownsBaneHUD>(CBPC->GetHUD()))
			{
				for (int32 i = 0; i < FMath::Min(3, HUD->PendingPerkChoices.Num()); ++i)
				{
					const FKey K = (i == 0) ? EKeys::One : (i == 1) ? EKeys::Two : EKeys::Three;
					if (PC->IsInputKeyDown(K) && ConsumeActionCooldown(FName(*FString::Printf(TEXT("PerkChoose%d"), i)), 0.30f))
					{
						CBPC->Progression->ChoosePerk((EShipPerk)HUD->PendingPerkChoices[i]);
						HUD->PendingPerkChoices.Reset();
						CBPC->ApplyPerkBonuses();
						if (CBPC->SoundManager) CBPC->SoundManager->Play(ESoundCue::PerkUnlock);
						break;
					}
				}
			}
			// Don't fall through to other 1-5 bindings while overlay open.
			return;
		}

		// Ammo switching 1-5 — only when no menu is open and we have a CannonComponent.
		if (!CBPC->IsTraderMenuOpen() && !CBPC->IsUpgradeUIOpen() && !CBPC->IsQuestLogOpen() && CannonComponent)
		{
			static const ECannonballType TypeMap[5] = {
				ECannonballType::Standard, ECannonballType::Chain,
				ECannonballType::Grape,    ECannonballType::Heavy,
				ECannonballType::Explosive
			};
			for (int32 i = 0; i < 5; ++i)
			{
				if (PC->IsInputKeyDown(NumKeys[i]) && ConsumeActionCooldown(FName(*FString::Printf(TEXT("Ammo%d"), i)), 0.30f))
				{
					CannonComponent->ActiveCannonballType = TypeMap[i];
					if (GEngine)
					{
						static const TCHAR* Names[5] = {
							TEXT("Round Shot"), TEXT("Chain Shot"),
							TEXT("Grape Shot"), TEXT("Heavy Shot"),
							TEXT("Explosive Shell")
						};
						GEngine->AddOnScreenDebugMessage(7777, 2.f, FColor::Yellow,
							FString::Printf(TEXT("Ammo: %s"), Names[i]));
					}
				}
			}
		}
	}

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

void AShipPawn::TriggerHitStop()
{
	HitStopTimeRemaining = HitStopDuration;
	if (UWorld* W = GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(W, HitStopTimeDilation);
	}
}

void AShipPawn::UpdateAiming(float DeltaTime)
{
	if (!CannonComponent || !Camera || !SpringArm) return;

	CannonComponent->SetIsAiming(bIsAiming);

	// Hit-stop overrides aim dilation: while ticking down, the world stays slow.
	// Otherwise we let the aim path decide the dilation as before.
	if (HitStopTimeRemaining > 0.0f)
	{
		HitStopTimeRemaining -= DeltaTime;
		if (HitStopTimeRemaining <= 0.0f && !bIsAiming)
		{
			if (UWorld* W = GetWorld())
				UGameplayStatics::SetGlobalTimeDilation(W, 1.0f);
		}
	}

	// Battle camera: pull SpringArm out a bit when there are enemies in close range.
	const float BattleR2 = BattleCameraTriggerRange * BattleCameraTriggerRange;
	bool bInBattle = false;
	for (TActorIterator<AEnemyShipBase> It(GetWorld()); It; ++It)
	{
		AEnemyShipBase* E = *It;
		if (!E || !E->HealthComponent || !E->HealthComponent->IsAlive()) continue;
		if (FVector::DistSquared(GetActorLocation(), E->GetActorLocation()) <= BattleR2)
		{
			bInBattle = true; break;
		}
	}
	// Battle boost is consumed by the non-aim else-branch below.
	const float BattleBoost = (bInBattle && !bIsAiming) ? BattleSpringArmBoost : 0.0f;

	// FOV zoom — interp current camera FOV toward target (snappy and cinematic).
	const float TargetFOV = bIsAiming ? AimFOV : DefaultFOV;
	Camera->SetFieldOfView(FMath::FInterpTo(Camera->FieldOfView, TargetFOV, DeltaTime, AimZoomSpeed));

	// Global time dilation while aiming — snap is fine because the dilation
	// itself slows everything else.  Skip if already correct.
	if (UWorld* W = GetWorld())
	{
		const float WantDil = bIsAiming ? AimTimeDilation : 1.0f;
		if (!FMath::IsNearlyEqual(UGameplayStatics::GetGlobalTimeDilation(W), WantDil, 0.01f))
		{
			UGameplayStatics::SetGlobalTimeDilation(W, WantDil);
		}
	}

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
		SpringArm->TargetArmLength = FMath::FInterpTo(SpringArm->TargetArmLength,
			DefaultSpringArmLength + BattleBoost, DeltaTime, AimZoomSpeed);
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
	// Sail integrity caps max speed — torn sails can't pull as much.
	const float SailMul = FMath::Clamp(SailIntegrity, 0.1f, 1.0f);
	return MaxSpeed * SailMult * Penalty * SailMul * GetWindMultiplier();
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

void AShipPawn::HandleHealthChanged(float CurrentHealth, float MaxHealth)
{
	UpdateDamageFX();
	if (CurrentHealth < LastSeenHealth - 0.01f)
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			if (ACrownsBaneHUD* HUD = Cast<ACrownsBaneHUD>(PC->GetHUD()))
			{
				const float Dealt = LastSeenHealth - CurrentHealth;
				const float Intensity = FMath::Clamp(Dealt / FMath::Max(1.0f, MaxHealth * 0.25f), 0.25f, 1.0f);
				HUD->TriggerDamageFlash(Intensity);
			}
		}
	}
	LastSeenHealth = CurrentHealth;
}

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

	bIsAiming = false;

	// Kick off the cinematic sink: time slows, controls freeze, hull tips and
	// descends.  Tick() now branches to TickSinking() until SinkDuration elapses.
	bIsSinking = true;
	SinkElapsed = 0.0f;
	CurrentSpeed = 0.0f;
	TurnInputValue = 0.0f;
	if (UWorld* W = GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(W, SinkTimeDilation);
	}
}

void AShipPawn::TickSinking(float DeltaTime)
{
	SinkElapsed += DeltaTime;
	const float T = FMath::Clamp(SinkElapsed / FMath::Max(0.1f, SinkDuration), 0.0f, 1.0f);

	// Pitch nose-down and sink along Z over the duration; the mesh handles
	// the visible motion so collision/root stays well-behaved.
	if (ShipMesh)
	{
		FRotator R = ShipMesh->GetRelativeRotation();
		R.Pitch = FMath::Lerp(0.0f, SinkPitchDegrees, T);
		R.Roll  = FMath::Lerp(0.0f, -8.0f, T);
		ShipMesh->SetRelativeRotation(R);

		FVector L = ShipMesh->GetRelativeLocation();
		L.Z = FMath::Lerp(0.0f, -SinkDepth, T);
		ShipMesh->SetRelativeLocation(L);
	}

	// Restore time and stop ticking the sink once the animation completes.
	if (T >= 1.0f)
	{
		bIsSinking = false;
		if (UWorld* W = GetWorld())
		{
			UGameplayStatics::SetGlobalTimeDilation(W, 1.0f);
		}
		RespawnAtLastDock();
	}
}

void AShipPawn::UpdateSharkHazard(float DeltaTime)
{
	if (!HealthComponent || !HealthComponent->IsAlive())
	{
		bSharksAttached = false;
		return;
	}
	const float HPpct = HealthComponent->GetHealthPercent();
	const bool bShouldAttach = HPpct <= SharkHPThreshold;

	// Toast on first attach.
	if (bShouldAttach && !bSharksAttached)
	{
		bSharksAttached = true;
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			if (ACrownsBaneHUD* HUD = Cast<ACrownsBaneHUD>(PC->GetHUD()))
			{
				HUD->PushResourceToast(TEXT("⚠ Sharks circling — repair soon!"),
					FLinearColor(1.0f, 0.4f, 0.4f, 1.0f));
			}
		}
	}
	else if (!bShouldAttach && bSharksAttached)
	{
		bSharksAttached = false;
	}

	if (!bSharksAttached) return;

	// Apply DPS in 1s chunks so the damage event fires cleanly.
	SharkDamageAccum += SharkDPS * DeltaTime;
	if (SharkDamageAccum >= 1.0f)
	{
		const float Apply = FMath::FloorToFloat(SharkDamageAccum);
		SharkDamageAccum -= Apply;
		HealthComponent->TakeDamage(Apply);
	}
}

void AShipPawn::RespawnAtLastDock()
{
	UWorld* W = GetWorld();
	if (!W) return;

	// Find nearest dock as fallback respawn point.
	ADocksZone* NearestDock = nullptr;
	float BestD2 = TNumericLimits<float>::Max();
	for (TActorIterator<ADocksZone> It(W); It; ++It)
	{
		ADocksZone* D = *It;
		if (!D) continue;
		const float D2 = FVector::DistSquared(GetActorLocation(), D->GetActorLocation());
		if (D2 < BestD2) { BestD2 = D2; NearestDock = D; }
	}
	if (!NearestDock)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ShipPawn] Respawn: no DocksZone found — staying put."));
		// Restore mesh transform at least.
		if (ShipMesh) { ShipMesh->SetRelativeLocation(FVector::ZeroVector); ShipMesh->SetRelativeRotation(FRotator::ZeroRotator); }
		if (HealthComponent) HealthComponent->Heal(HealthComponent->GetMaxHealth());
		LastSeenHealth = HealthComponent ? HealthComponent->GetCurrentHealth() : -1.f;
		SailIntegrity = 1.0f;
		return;
	}

	// Despawn enemies in radius around the dock so we don't immediately die again.
	const FVector DockLoc = NearestDock->GetActorLocation();
	const float R2 = RespawnEnemyDespawnRadius * RespawnEnemyDespawnRadius;
	for (TActorIterator<AEnemyShipBase> It(W); It; ++It)
	{
		AEnemyShipBase* E = *It;
		if (E && FVector::DistSquared(E->GetActorLocation(), DockLoc) < R2)
		{
			E->Destroy();
		}
	}

	// Gold penalty.
	if (ACrownsBanePlayerController* PC = Cast<ACrownsBanePlayerController>(GetController()))
	{
		int32 Lost = 0;
		if (UPlayerInventory* Inv = PC->PlayerInventory)
		{
			Lost = FMath::FloorToInt(Inv->GetGold() * DeathGoldPenaltyFraction);
			Inv->SpendResource(EResourceType::Gold, Lost);
		}
		// Death screen overlay.
		if (ACrownsBaneHUD* HUD = Cast<ACrownsBaneHUD>(PC->GetHUD()))
		{
			HUD->TriggerDeathScreen(Lost, NearestDock->DockName.IsEmpty()
				? TEXT("Port") : NearestDock->DockName);
		}
	}

	// Teleport ship to dock + heal + reset state.
	SetActorLocation(DockLoc + FVector(0, 0, 60));
	SetActorRotation(NearestDock->GetActorRotation());
	if (HealthComponent) HealthComponent->Heal(HealthComponent->GetMaxHealth());
	LastSeenHealth = HealthComponent ? HealthComponent->GetCurrentHealth() : -1.f;
	SailIntegrity = 1.0f;
	CurrentSpeed = 0.0f;
	CurrentSailLevel = ESailLevel::Stop;
	if (ShipMesh)
	{
		ShipMesh->SetRelativeLocation(FVector::ZeroVector);
		ShipMesh->SetRelativeRotation(FRotator::ZeroRotator);
	}
}