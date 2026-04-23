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

	// Separate SceneComponent root so visual mesh roll does not affect ActorForwardVector.
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	ShipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShipMesh"));
	ShipMesh->SetupAttachment(RootComponent);
	ShipMesh->SetSimulatePhysics(false);
	ShipMesh->SetCollisionProfileName(TEXT("Pawn"));
	ShipMesh->SetGenerateOverlapEvents(true);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 1800.0f;
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

	// CRITICAL: auto-possess by Player 0, disable AI auto-possess
	AutoPossessPlayer = EAutoReceiveInput::Player0;
	AutoPossessAI     = EAutoPossessAI::Disabled;

	bUseControllerRotationYaw   = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll  = false;
}

void AShipPawn::BeginPlay()
{
	Super::BeginPlay();

	// Ensure IMC/IA assets exist, creating defaults if BP left them empty.
	// This makes input work out-of-the-box with zero Blueprint configuration.
	EnsureInputAssetsExist();

	// Fallback: some possession orderings let BeginPlay run with a valid controller.
	AddInputMappingContext();

	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AWindSystem::StaticClass(), Found);
	CachedWindSystem = Found.Num() > 0 ? Cast<AWindSystem>(Found[0]) : nullptr;

	// Bind health events for damage FX
	if (HealthComponent)
	{
		HealthComponent->OnHealthChanged.AddDynamic(this, &AShipPawn::HandleHealthChanged);
		HealthComponent->OnDeath.AddDynamic(this, &AShipPawn::HandleDeath);
	}

	// Assign Niagara assets to components if provided via BP
	if (DamageSmokeFX && SmokeAsset) DamageSmokeFX->SetAsset(SmokeAsset);
	if (DamageFireFX && FireAsset)   DamageFireFX->SetAsset(FireAsset);
	if (BowWakeFX && WakeAsset)      BowWakeFX->SetAsset(WakeAsset);

	UE_LOG(LogTemp, Log, TEXT("[ShipPawn] BeginPlay. Controller=%s  Wind=%s"),
		GetController() ? *GetController()->GetName() : TEXT("NULL"),
		CachedWindSystem ? TEXT("found") : TEXT("not found"));
}

// Canonical UE5 possession hook — called on client after PlayerController takes this pawn.
void AShipPawn::PawnClientRestart()
{
	Super::PawnClientRestart();
	EnsureInputAssetsExist();
	AddInputMappingContext();
	UE_LOG(LogTemp, Log, TEXT("[ShipPawn] PawnClientRestart -> IMC registered."));
}

// Called on server/standalone when controller changes (including initial possession).
void AShipPawn::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
	EnsureInputAssetsExist();
	AddInputMappingContext();
	UE_LOG(LogTemp, Log, TEXT("[ShipPawn] NotifyControllerChanged -> IMC registered."));
}

void AShipPawn::EnsureInputAssetsExist()
{
	// Create missing Input Actions. These are transient objects outlived by the pawn.
	auto MakeIA = [this](UInputAction*& Target, EInputActionValueType ValueType, const TCHAR* Tag)
	{
		if (Target) return;
		Target = NewObject<UInputAction>(this, UInputAction::StaticClass(), FName(Tag));
		Target->ValueType = ValueType;
		UE_LOG(LogTemp, Warning, TEXT("[ShipPawn] Auto-created fallback %s (BP field was NULL)."), Tag);
	};

	MakeIA(IA_IncreaseSail, EInputActionValueType::Boolean, TEXT("IA_IncreaseSail_Auto"));
	MakeIA(IA_DecreaseSail, EInputActionValueType::Boolean, TEXT("IA_DecreaseSail_Auto"));
	MakeIA(IA_Turn,         EInputActionValueType::Axis1D,  TEXT("IA_Turn_Auto"));
	MakeIA(IA_FireLeft,     EInputActionValueType::Boolean, TEXT("IA_FireLeft_Auto"));
	MakeIA(IA_FireRight,    EInputActionValueType::Boolean, TEXT("IA_FireRight_Auto"));
	MakeIA(IA_Fire,         EInputActionValueType::Boolean, TEXT("IA_Fire_Auto"));

	if (!ShipMappingContext)
	{
		ShipMappingContext = NewObject<UInputMappingContext>(this, UInputMappingContext::StaticClass(), TEXT("IMC_Ship_Auto"));

		ShipMappingContext->MapKey(IA_IncreaseSail, EKeys::W);
		ShipMappingContext->MapKey(IA_DecreaseSail, EKeys::S);

		// Turn axis: D = positive, A = negative via Negate modifier
		ShipMappingContext->MapKey(IA_Turn, EKeys::D);
		FEnhancedActionKeyMapping& TurnLeftMap = ShipMappingContext->MapKey(IA_Turn, EKeys::A);
		UInputModifierNegate* Neg = NewObject<UInputModifierNegate>(ShipMappingContext);
		TurnLeftMap.Modifiers.Add(Neg);

		// Arrow keys also work
		ShipMappingContext->MapKey(IA_Turn, EKeys::Right);
		FEnhancedActionKeyMapping& TurnLeftArrow = ShipMappingContext->MapKey(IA_Turn, EKeys::Left);
		TurnLeftArrow.Modifiers.Add(NewObject<UInputModifierNegate>(ShipMappingContext));

		// Legacy manual-side fire
		ShipMappingContext->MapKey(IA_FireLeft,  EKeys::Q);
		ShipMappingContext->MapKey(IA_FireRight, EKeys::E);

		// Camera-aimed fire (preferred): LMB and SpaceBar
		ShipMappingContext->MapKey(IA_Fire, EKeys::LeftMouseButton);
		ShipMappingContext->MapKey(IA_Fire, EKeys::SpaceBar);

		UE_LOG(LogTemp, Warning, TEXT("[ShipPawn] Auto-created fallback IMC_Ship with default keybinds "
			"(W/S sails, A/D or arrows turn, Q/LMB port fire, E/RMB starboard fire)."));
	}
}

void AShipPawn::AddInputMappingContext()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[ShipPawn] AddInputMappingContext: no PlayerController yet."));
		return;
	}

	ULocalPlayer* LP = PC->GetLocalPlayer();
	if (!LP)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ShipPawn] No LocalPlayer on controller!"));
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP);
	if (!Subsystem)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[ShipPawn] EnhancedInput subsystem missing. "
			     "Enable Enhanced Input plugin AND set Project Settings > Input > "
			     "Default Input Component Class = EnhancedInputComponent."));
		if (GEngine && bShowDebugOnScreen)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red,
				TEXT("Enhanced Input subsystem not found! Check Project Settings > Input."));
		}
		return;
	}

	if (!ShipMappingContext)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[ShipPawn] ShipMappingContext is NULL! Assign IMC_Ship in BP_PlayerShip Details panel."));
		if (GEngine && bShowDebugOnScreen)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red,
				TEXT("BP_PlayerShip: ShipMappingContext NOT assigned! Assign IMC_Ship in Details."));
		}
		return;
	}

	Subsystem->ClearAllMappings();
	Subsystem->AddMappingContext(ShipMappingContext, 0);

	if (GEngine && bShowDebugOnScreen)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
			FString::Printf(TEXT("IMC registered: %s"), *ShipMappingContext->GetName()));
	}
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

	// Final-fallback raw polling — runs unconditionally, but edge-triggered
	// state means it never double-fires when Enhanced Input is also active.
	// For TurnAxis, if EnhancedInput just updated it, raw polling will match
	// the same key state (idempotent).
	PollRawInputFallback(DeltaTime);

	UpdateMovement(DeltaTime);

	if (!FMath::IsNearlyZero(TurnInputValue))
	{
		float SpeedFraction = MaxSpeed > 0.0f ? CurrentSpeed / MaxSpeed : 0.0f;
		float TurnRate = FMath::Lerp(BaseTurnRate, BaseTurnRate * HighSpeedTurnFactor, SpeedFraction);
		AddActorLocalRotation(FRotator(0.0f, TurnInputValue * TurnRate * DeltaTime, 0.0f));
	}

	UpdateVisualRoll(DeltaTime);
	UpdateBowWake();

	if (GEngine && bShowDebugOnScreen)
	{
		static const TCHAR* SailNames[] = { TEXT("Stop"), TEXT("Half"), TEXT("Full") };
		GEngine->AddOnScreenDebugMessage(1001, 0.0f, FColor::Yellow,
			FString::Printf(TEXT("[Ship] Sail=%s Speed=%.0f Turn=%.2f EI=%s LastInput=%s Ctrl=%s"),
				SailNames[(int32)CurrentSailLevel], CurrentSpeed, TurnInputValue,
				bEnhancedInputReady ? TEXT("OK") : TEXT("FALLBACK"),
				*LastInputSource,
				GetController() ? *GetController()->GetName() : TEXT("NONE")));
	}
}

void AShipPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Make sure IA properties are non-null before binding.
	EnsureInputAssetsExist();

	// Attempt Enhanced Input binding (preferred).
	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (EIC)
	{
		if (IA_IncreaseSail)
			EIC->BindAction(IA_IncreaseSail, ETriggerEvent::Started,   this, &AShipPawn::Input_IncreaseSail);
		if (IA_DecreaseSail)
			EIC->BindAction(IA_DecreaseSail, ETriggerEvent::Started,   this, &AShipPawn::Input_DecreaseSail);
		if (IA_Turn)
		{
			EIC->BindAction(IA_Turn, ETriggerEvent::Triggered, this, &AShipPawn::Input_Turn);
			EIC->BindAction(IA_Turn, ETriggerEvent::Completed, this, &AShipPawn::Input_TurnCompleted);
			EIC->BindAction(IA_Turn, ETriggerEvent::Canceled,  this, &AShipPawn::Input_TurnCompleted);
		}
		if (IA_FireLeft)  EIC->BindAction(IA_FireLeft,  ETriggerEvent::Started, this, &AShipPawn::Input_FireLeft);
		if (IA_FireRight) EIC->BindAction(IA_FireRight, ETriggerEvent::Started, this, &AShipPawn::Input_FireRight);
		if (IA_Fire)      EIC->BindAction(IA_Fire,      ETriggerEvent::Started, this, &AShipPawn::Input_Fire);

		bEnhancedInputReady = true;
		UE_LOG(LogTemp, Log, TEXT("[ShipPawn] Enhanced Input bound successfully."));
	}
	else
	{
		bEnhancedInputReady = false;
		UE_LOG(LogTemp, Warning,
			TEXT("[ShipPawn] EnhancedInputComponent cast failed; raw polling fallback will drive input. "
			     "Check Project Settings > Input > Default Input Component Class."));
		if (GEngine && bShowDebugOnScreen)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow,
				TEXT("EnhancedInputComponent not found — using raw key polling fallback."));
		}
	}
}

float AShipPawn::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	// Apply hull armor reduction BEFORE Super::TakeDamage so HealthComponent receives the reduced value
	const float MitigatedDamage = DamageAmount * (1.0f - ArmorReduction);
	float Actual = Super::TakeDamage(MitigatedDamage, DamageEvent, EventInstigator, DamageCauser);

	if (MitigatedDamage > 0.0f && HitCameraShake)
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			const float MaxHP = HealthComponent ? HealthComponent->GetMaxHealth() : 200.0f;
			const float Scale = FMath::Clamp(HitCameraShakeScale * (MitigatedDamage / MaxHP) * 5.0f,
				0.3f, HitCameraShakeScale * 2.0f);
			PC->ClientStartCameraShake(HitCameraShake, Scale);
		}
	}

	return Actual;
}

void AShipPawn::Input_IncreaseSail(const FInputActionValue& /*Value*/)
{
	LastInputSource = TEXT("EnhancedInput");
	DoIncreaseSail();
}

void AShipPawn::Input_DecreaseSail(const FInputActionValue& /*Value*/)
{
	LastInputSource = TEXT("EnhancedInput");
	DoDecreaseSail();
}

void AShipPawn::Input_Turn(const FInputActionValue& Value)
{
	LastInputSource = TEXT("EnhancedInput");
	DoSetTurnAxis(Value.Get<float>());
}

void AShipPawn::Input_TurnCompleted(const FInputActionValue& /*Value*/)
{
	DoSetTurnAxis(0.0f);
}

void AShipPawn::Input_FireLeft(const FInputActionValue& /*Value*/)
{
	LastInputSource = TEXT("EnhancedInput");
	DoFireLeft();
}

void AShipPawn::Input_FireRight(const FInputActionValue& /*Value*/)
{
	LastInputSource = TEXT("EnhancedInput");
	DoFireRight();
}

void AShipPawn::Input_Fire(const FInputActionValue& /*Value*/)
{
	LastInputSource = TEXT("EnhancedInput");
	DoCameraAimFire();
}

// ---- Shared action implementations ---------------------------------------

void AShipPawn::DoIncreaseSail()
{
	switch (CurrentSailLevel)
	{
	case ESailLevel::Stop:     CurrentSailLevel = ESailLevel::HalfSail; break;
	case ESailLevel::HalfSail: CurrentSailLevel = ESailLevel::FullSail; break;
	default: break;
	}
	UE_LOG(LogTemp, Log, TEXT("[ShipPawn] Sail -> %d (%s)"), (int)CurrentSailLevel, *LastInputSource);
}

void AShipPawn::DoDecreaseSail()
{
	switch (CurrentSailLevel)
	{
	case ESailLevel::FullSail: CurrentSailLevel = ESailLevel::HalfSail; break;
	case ESailLevel::HalfSail: CurrentSailLevel = ESailLevel::Stop;     break;
	default: break;
	}
	UE_LOG(LogTemp, Log, TEXT("[ShipPawn] Sail -> %d (%s)"), (int)CurrentSailLevel, *LastInputSource);
}

void AShipPawn::DoSetTurnAxis(float Value)
{
	TurnInputValue = FMath::Clamp(Value, -1.0f, 1.0f);
}

void AShipPawn::DoFireLeft()
{
	if (CannonComponent) CannonComponent->FireBroadside(ECannonSide::Left);
}

void AShipPawn::DoFireRight()
{
	if (CannonComponent) CannonComponent->FireBroadside(ECannonSide::Right);
}

void AShipPawn::DoCameraAimFire()
{
	if (!CannonComponent || !Camera) return;

	const FVector CamFwd = Camera->GetForwardVector();
	const float DotRight = FVector::DotProduct(CamFwd, GetActorRightVector());

	if (DotRight > 0.35f)
	{
		CannonComponent->FireBroadside(ECannonSide::Right);
	}
	else if (DotRight < -0.35f)
	{
		CannonComponent->FireBroadside(ECannonSide::Left);
	}
	else if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.2f, FColor::Yellow,
			TEXT("Turn camera left or right to aim broadside"));
	}
}

void AShipPawn::PollRawInputFallback(float /*DeltaTime*/)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	// --- Edge-triggered actions: only fire on raw poll if Enhanced Input is
	// not driving bindings (avoids double-trigger).
	if (!bEnhancedInputReady)
	{
		const bool bW = PC->IsInputKeyDown(EKeys::W) || PC->IsInputKeyDown(EKeys::Up);
		if (bW && !bRawPrevW) { LastInputSource = TEXT("RawPoll"); DoIncreaseSail(); }
		bRawPrevW = bW;

		const bool bS = PC->IsInputKeyDown(EKeys::S) || PC->IsInputKeyDown(EKeys::Down);
		if (bS && !bRawPrevS) { LastInputSource = TEXT("RawPoll"); DoDecreaseSail(); }
		bRawPrevS = bS;

		const bool bQ = PC->IsInputKeyDown(EKeys::Q);
		if (bQ && !bRawPrevQ) { LastInputSource = TEXT("RawPoll"); DoFireLeft(); }
		bRawPrevQ = bQ;

		const bool bE = PC->IsInputKeyDown(EKeys::E);
		if (bE && !bRawPrevE) { LastInputSource = TEXT("RawPoll"); DoFireRight(); }
		bRawPrevE = bE;

		const bool bFire = PC->IsInputKeyDown(EKeys::LeftMouseButton) || PC->IsInputKeyDown(EKeys::SpaceBar);
		if (bFire && !bRawPrevFire) { LastInputSource = TEXT("RawPoll"); DoCameraAimFire(); }
		bRawPrevFire = bFire;
	}

	// --- Continuous turn axis: always safe to re-derive since it's idempotent.
	// We only apply if Enhanced Input hasn't bound OR the EI turn axis is zero
	// (keeps both pathways compatible).
	if (!bEnhancedInputReady)
	{
		float Raw = 0.0f;
		if (PC->IsInputKeyDown(EKeys::D) || PC->IsInputKeyDown(EKeys::Right)) Raw += 1.0f;
		if (PC->IsInputKeyDown(EKeys::A) || PC->IsInputKeyDown(EKeys::Left))  Raw -= 1.0f;
		DoSetTurnAxis(Raw);
		if (!FMath::IsNearlyZero(Raw)) LastInputSource = TEXT("RawPoll");
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
		// sweep=false avoids getting stuck on invisible water collision
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

void AShipPawn::UpgradeMaxSpeed(float BonusSpeed)
{
	MaxSpeed += BonusSpeed;
}

void AShipPawn::UpgradeTurnRate(float BonusTurnRate)
{
	BaseTurnRate += BonusTurnRate;
}

void AShipPawn::UpgradeHullArmor(float AdditionalReductionPct)
{
	// AdditionalReductionPct is expressed as percent (e.g. 10 = 10%)
	ArmorReduction = FMath::Clamp(ArmorReduction + AdditionalReductionPct * 0.01f, 0.0f, 0.75f);
	UE_LOG(LogTemp, Log, TEXT("[ShipPawn] Hull armor now %.0f%% reduction"), ArmorReduction * 100.0f);
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

void AShipPawn::HandleHealthChanged(float /*CurrentHealth*/, float /*MaxHealth*/)
{
	UpdateDamageFX();
}

void AShipPawn::HandleDeath()
{
	// Play death FX at ship location
	UWorld* World = GetWorld();
	if (World)
	{
		const FVector Loc = GetActorLocation();
		if (DeathExplosionAsset)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, DeathExplosionAsset, Loc, GetActorRotation());
		}
		if (DeathSound)
		{
			UGameplayStatics::PlaySoundAtLocation(World, DeathSound, Loc);
		}
	}

	// Stop persistent FX
	if (DamageSmokeFX) DamageSmokeFX->Deactivate();
	if (DamageFireFX)  DamageFireFX->Deactivate();
	if (BowWakeFX)     BowWakeFX->Deactivate();
}
