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

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override; // ƒŒƒ¿ÕŒ ƒÀﬂ —“¿¡≤À‹ÕŒ√Œ ≤Õœ”“”
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	// ---- Components ----
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

	// ---- Movement Settings ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MaxSpeed = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float HalfSailSpeedMultiplier = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float AccelerationRate = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float DecelerationRate = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float BaseTurnRate = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float HighSpeedTurnFactor = 0.4f;

	// ---- Wind ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind")
	float WindInfluenceFactor = 0.2f;

	// ---- State (read-only) ----
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	ESailLevel CurrentSailLevel = ESailLevel::Stop;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float CurrentSpeed = 0.0f;

	// ---- Public API ----
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
	void Input_FireLeft(const FInputActionValue& Value);
	void Input_FireRight(const FInputActionValue& Value);

private:
	void UpdateMovement(float DeltaTime);
	float GetTargetSpeed() const;
	float GetWindMultiplier() const;

	AWindSystem* CachedWindSystem;
	float TurnInputValue = 0.0f;
	float SpeedPenaltyFraction = 0.0f;
	float SpeedPenaltyTimeRemaining = 0.0f;
};