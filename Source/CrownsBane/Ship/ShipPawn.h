#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
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

	// ---- FX Components (always-on children of ShipMesh) ----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FX")
	UNiagaraComponent* DamageSmokeFX;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FX")
	UNiagaraComponent* DamageFireFX;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FX")
	UNiagaraComponent* BowWakeFX;

	// Assets (assign in BP_PlayerShip Details, otherwise components stay empty)
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

	// HP thresholds for visual damage states (0..1)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX", meta=(ClampMin="0.0", ClampMax="1.0"))
	float SmokeHPThreshold = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX", meta=(ClampMin="0.0", ClampMax="1.0"))
	float FireHPThreshold = 0.3f;

	// Local-space offsets for damage FX (relative to ShipMesh)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
	FVector SmokeSocketOffset = FVector(0.f, 0.f, 200.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
	FVector FireSocketOffset = FVector(0.f, 0.f, 100.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
	FVector BowWakeOffset = FVector(400.f, 0.f, -30.f);

	// ---- Enhanced Input (assign in BP_PlayerShip Details) ----
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

	// Show debug text on screen (HUD-like)
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

	// Visual roll when turning (degrees at max turn)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Visual")
	float MaxVisualRoll = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Visual")
	float VisualRollInterpSpeed = 2.0f;

	// ---- Wind ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind")
	float WindInfluenceFactor = 0.2f;

	// ---- State ----
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	ESailLevel CurrentSailLevel = ESailLevel::Stop;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float CurrentSpeed = 0.0f;

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

protected:
	void Input_IncreaseSail(const FInputActionValue& Value);
	void Input_DecreaseSail(const FInputActionValue& Value);
	void Input_Turn(const FInputActionValue& Value);
	void Input_TurnCompleted(const FInputActionValue& Value);
	void Input_FireLeft(const FInputActionValue& Value);
	void Input_FireRight(const FInputActionValue& Value);

	// Registers IMC on the possessing PlayerController's Enhanced Input subsystem.
	// Called from PawnClientRestart (canonical), NotifyControllerChanged, and BeginPlay (fallback).
	void AddInputMappingContext();

private:
	void UpdateMovement(float DeltaTime);
	void UpdateVisualRoll(float DeltaTime);
	void UpdateDamageFX();
	void UpdateBowWake();
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
};
