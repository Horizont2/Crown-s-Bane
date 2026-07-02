// Copyright 2024 Crown's Bane. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Combat/CannonComponent.h" // Needed for ECannonSide enum
#include "ShipPawn.generated.h"

class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class UCannonComponent;
class UHealthComponent;
class AWindSystem;
class UInputMappingContext;
class UInputAction;
class UNiagaraComponent;
class UNiagaraSystem;
class USoundBase;
class UCameraShakeBase;
struct FInputActionValue;

UENUM(BlueprintType)
enum class ESailLevel : uint8
{
	Stop        UMETA(DisplayName = "Stop"),
	HalfSail    UMETA(DisplayName = "Half Sail"),
	FullSail    UMETA(DisplayName = "Full Sail")
};

UCLASS()
class CROWNSBANE_API AShipPawn : public APawn
{
	GENERATED_BODY()

public:
	AShipPawn();

protected:
	virtual void BeginPlay() override;
	virtual void PawnClientRestart() override;
	virtual void NotifyControllerChanged() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	// ---- Components ----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ShipMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCannonComponent* CannonComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UHealthComponent* HealthComponent;

	// ---- FX Components ----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FX")
	UNiagaraComponent* DamageSmokeFX;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FX")
	UNiagaraComponent* DamageFireFX;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FX")
	UNiagaraComponent* BowWakeFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
	UNiagaraSystem* SmokeAsset = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
	UNiagaraSystem* FireAsset = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
	UNiagaraSystem* WakeAsset = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
	UNiagaraSystem* DeathExplosionAsset = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
	USoundBase* DeathSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
	TSubclassOf<UCameraShakeBase> HitCameraShake;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX", meta = (ClampMin = "0.0"))
	float HitCameraShakeScale = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SmokeHPThreshold = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FireHPThreshold = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
	FVector SmokeSocketOffset = FVector(0.f, 0.f, 200.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
	FVector FireSocketOffset = FVector(0.f, 0.f, 100.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
	FVector BowWakeOffset = FVector(400.f, 0.f, -30.f);

	// ---- Enhanced Input ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* ShipMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_IncreaseSail;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_DecreaseSail;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Turn;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_FireLeft;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_FireRight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Fire;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Aim;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Look;

	// Toggle docks/upgrade UI (U key). Active only when inside DocksZone.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_ToggleDocks;

	// Toggle quest log (J key).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_QuestLog;

	// Board nearest crippled enemy (F key).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Board;

	// Open the docks trader menu (T key).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Trader;

	// Cycle lock-on target (Tab key).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_LockOn;

	// Brace for impact (B hold) — halves incoming damage but stops fire.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Brace;

	// Drop anchor (V) — emergency hard stop.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_DropAnchor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Pause;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Help;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Settings;

	// ---- Brace / Ramming ----
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Brace")
	bool bBracing = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Brace", meta=(ClampMin="0.0", ClampMax="0.95"))
	float BraceDamageReduction = 0.5f;

	// Ram damage dealt to enemy per cm/s of contact speed.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Ram")
	float RamDamageScale = 0.05f;

	// Fraction of ram damage applied to self.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Ram")
	float RamSelfDamageFraction = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Ram")
	float RamMinSpeedForDamage = 600.0f;

	// Cooldown so ramming doesn't apply on every tick of overlap.
	float RamCooldown = 0.0f;

	// ---- Boarding ----
	// Enemy HP fraction at or below which the ship becomes boardable.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Boarding", meta=(ClampMin="0.05", ClampMax="0.5"))
	float BoardableHealthThreshold = 0.15f;

	// Max distance from player to enemy for boarding to be allowed (cm).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Boarding")
	float BoardingDistance = 800.0f;

	// Loot multiplier applied to enemy's drop table when boarded (vs. sunk).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Boarding")
	float BoardingLootMultiplier = 2.5f;

	// Cached: the enemy currently in boarding range, or null.  HUD reads this to draw the prompt.
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Boarding")
	class AEnemyShipBase* CurrentBoardingTarget = nullptr;

	// ---- Boarding QTE state ----
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Boarding|QTE")
	bool bBoardingActive = false;

	// Hits required to win the QTE (spacebar mashes).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Boarding|QTE")
	int32 BoardingQTERequiredHits = 6;

	// Time window before the QTE auto-fails.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Boarding|QTE")
	float BoardingQTEDuration = 4.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Boarding|QTE")
	int32 BoardingQTEHits = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Boarding|QTE")
	float BoardingQTETimeRemaining = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Boarding|QTE")
	class AEnemyShipBase* BoardingQTETarget = nullptr;

	// Press SPACE during a QTE to register a hit.
	void RegisterBoardingQTEPress();

	// ---- Lock-on target (Tab cycles between visible enemies) ----
	UPROPERTY(BlueprintReadOnly, Category = "Combat|LockOn")
	class AEnemyShipBase* LockedTarget = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|LockOn")
	float LockOnMaxRange = 9000.0f;

	UFUNCTION(BlueprintCallable, Category = "Combat|LockOn")
	void CycleLockOnTarget();

	UFUNCTION(BlueprintCallable, Category = "Combat|LockOn")
	void ClearLockOn() { LockedTarget = nullptr; }

	// ---- Camera & Aiming ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float DefaultSpringArmLength = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float AimSpringArmLength = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float AimZoomSpeed = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Aim")
	float DefaultFOV = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Aim")
	float AimFOV = 65.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Aim", meta=(ClampMin="0.2", ClampMax="1.0"))
	float AimTimeDilation = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Aim", meta=(ClampMin="0.1", ClampMax="1.0"))
	float AimLookSensitivityScale = 0.5f;

	// Battle camera kicks in when enemies are within this range (cm) — pulls back slightly.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Battle")
	float BattleCameraTriggerRange = 2500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Battle")
	float BattleSpringArmBoost = 350.0f;

	// Brief time-stop on landing a player hit (seconds).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Feel", meta=(ClampMin="0.0", ClampMax="0.2"))
	float HitStopDuration = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Feel", meta=(ClampMin="0.05", ClampMax="1.0"))
	float HitStopTimeDilation = 0.1f;

	UFUNCTION(BlueprintCallable, Category = "Camera|Feel")
	void TriggerHitStop();

	float HitStopTimeRemaining = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	float LookYawSensitivity = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	float LookPitchSensitivity = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	float LookPitchMin = -70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	float LookPitchMax = 10.0f;

	// Mouse-up = camera-up by default.  Flip to invert.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	bool bInvertMouseY = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bShowDebugOnScreen = true;

	// ---- Movement Settings ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MaxSpeed = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float HalfSailSpeedMultiplier = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float AccelerationRate = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float DecelerationRate = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float BaseTurnRate = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float HighSpeedTurnFactor = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Visual")
	float MaxVisualRoll = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Visual")
	float VisualRollInterpSpeed = 2.0f;

	// Z offset (cm) applied to ShipMesh so the hull sits AT the water line.
	// Tweak per imported mesh: negative = lower into water, positive = raise it.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Visual")
	float WaterLineOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind")
	float WindInfluenceFactor = 0.2f;

	// ---- State ----
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	ESailLevel CurrentSailLevel = ESailLevel::Stop;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float CurrentSpeed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Aim")
	bool bIsAiming = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Aim")
	ECannonSide AimingSide = ECannonSide::Right;

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void ApplySpeedPenalty(float PenaltyFraction, float Duration);

	UFUNCTION(BlueprintPure, Category = "Movement")
	float GetCurrentSpeed() const { return CurrentSpeed; }

	UFUNCTION(BlueprintPure, Category = "Movement")
	ESailLevel GetSailLevel() const { return CurrentSailLevel; }

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void UpgradeMaxSpeed(float BonusSpeed);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void UpgradeTurnRate(float BonusTurnRate);

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float ArmorReduction = 0.0f;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void UpgradeHullArmor(float AdditionalReductionPct);

	UPROPERTY(BlueprintReadOnly, Category = "Input")
	bool bEnhancedInputReady = false;

	UPROPERTY(BlueprintReadOnly, Category = "Input")
	FString LastInputSource = TEXT("none");

protected:
	void Input_IncreaseSail(const FInputActionValue& Value);
	void Input_DecreaseSail(const FInputActionValue& Value);
	void Input_Turn(const FInputActionValue& Value);
	void Input_TurnCompleted(const FInputActionValue& Value);
	void Input_FireLeft(const FInputActionValue& Value);
	void Input_FireRight(const FInputActionValue& Value);
	void Input_Fire(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);

	// New Aim Inputs
	void Input_AimStart(const FInputActionValue& Value);
	void Input_AimStop(const FInputActionValue& Value);

	// UI hotkeys
	void Input_ToggleDocks(const FInputActionValue& Value);
	void Input_ToggleQuestLog(const FInputActionValue& Value);
	void Input_Board(const FInputActionValue& Value);
	void Input_Trader(const FInputActionValue& Value);
	void Input_LockOn(const FInputActionValue& Value);
	void Input_BraceStart(const FInputActionValue& Value);
	void Input_BraceStop(const FInputActionValue& Value);
	void Input_DropAnchor(const FInputActionValue& Value);
	void Input_Pause(const FInputActionValue& Value);
	void Input_Help(const FInputActionValue& Value);
	void Input_Settings(const FInputActionValue& Value);

	void UpdateBoardingTarget();
	void ExecuteBoarding();

	void DoIncreaseSail();
	void DoDecreaseSail();
	void DoFireLeft();
	void DoFireRight();
	void DoCameraAimFire();
	void DoSetTurnAxis(float Value);
	void DoLook(const FVector2D& Delta);

	bool ConsumeActionCooldown(FName ActionTag, float CooldownSec = 0.25f);
	void PollRawInputFallback(float DeltaTime);
	void AddInputMappingContext();
	void EnsureInputAssetsExist();

private:
	void UpdateMovement(float DeltaTime);
	void UpdateVisualRoll(float DeltaTime);
	void UpdateDamageFX();
	void UpdateBowWake();
	void UpdateAiming(float DeltaTime); // Dynamic AC4 Aim calculation

	float GetTargetSpeed() const;
	float GetWindMultiplier() const;

	UFUNCTION()
	void HandleHealthChanged(float CurrentHealth, float MaxHealth);

	UFUNCTION()
	void HandleDeath();

	AWindSystem* CachedWindSystem;
	float TurnInputValue = 0.0f;
	float SpeedPenaltyFraction = 0.0f;
	float SpeedPenaltyTimeRemaining = 0.0f;
	float CurrentVisualRoll = 0.0f;

	bool bRawPrevW = false;
	bool bRawPrevS = false;
	bool bRawPrevQ = false;
	bool bRawPrevE = false;
	bool bRawPrevFire = false;
	bool bQTEPrevSpace = false;

	TMap<FName, float> ActionFireTimes;

	float LookYawOffset = 0.0f;
	float LookPitchOffset = 0.0f;

	float LastSeenHealth = -1.0f;

	// Cinematic death-sink state.
	bool bIsSinking = false;
	float SinkElapsed = 0.0f;
	void TickSinking(float DeltaTime);

	// Respawn at last dock with gold penalty.
	void RespawnAtLastDock();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Death", meta=(ClampMin="0.0", ClampMax="1.0"))
	float DeathGoldPenaltyFraction = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Death")
	float RespawnEnemyDespawnRadius = 4000.0f;

	// Sails integrity 0..1 — Heavy/Explosive hits damage it; below 1.0 it
	// linearly scales MaxSpeed.  Restored to 1.0 at docks.
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Sails")
	float SailIntegrity = 1.0f;

	// ---- Sharks (low HP hazard) ----
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Hazards")
	bool bSharksAttached = false;

	// HP threshold below which sharks start circling.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Hazards", meta=(ClampMin="0.0", ClampMax="1.0"))
	float SharkHPThreshold = 0.30f;

	// Damage per second from sharks while attached.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Hazards")
	float SharkDPS = 4.0f;

	float SharkDamageAccum = 0.0f;
	void UpdateSharkHazard(float DeltaTime);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Sails")
	float SailDamagePerHeavyHit = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Sails")
	float SailRegenAtDock = 1.0f;
private:

public:
	// Tuneables for the death-sink sequence.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX|Death")
	float SinkDuration = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX|Death")
	float SinkDepth = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX|Death")
	float SinkPitchDegrees = -22.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX|Death", meta=(ClampMin="0.1", ClampMax="1.0"))
	float SinkTimeDilation = 0.5f;
};