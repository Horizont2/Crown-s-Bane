#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ShipPawn.generated.h"

class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class UCannonComponent;
class UHealthComponent;
class UFloatingPawnMovement;
class AWindSystem;

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

	// ---- Movement Settings ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MaxSpeed = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float HalfSailSpeedMultiplier = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float AccelerationRate = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float DecelerationRate = 60.0f;

	// Turn rate in degrees/second (reduced at higher speeds for wider turning radius)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float BaseTurnRate = 45.0f;

	// At full speed, turn rate is multiplied by this factor (< 1 for wider radius)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float HighSpeedTurnFactor = 0.4f;

	// Ship mass for inertia calculations
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float ShipMass = 5000.0f;

	// ---- Wind ----
	// Wind provides up to +20% or -20% speed bonus based on alignment
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind")
	float WindInfluenceFactor = 0.2f;

	// ---- Sail State ----
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	ESailLevel CurrentSailLevel = ESailLevel::Stop;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float CurrentSpeed = 0.0f;

	// ---- Speed Penalty (from chain shot) ----
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void ApplySpeedPenalty(float PenaltyFraction, float Duration);

	UFUNCTION(BlueprintPure, Category = "Movement")
	float GetCurrentSpeed() const { return CurrentSpeed; }

	UFUNCTION(BlueprintPure, Category = "Movement")
	ESailLevel GetSailLevel() const { return CurrentSailLevel; }

	// Called by upgrade system
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void UpgradeMaxSpeed(float BonusSpeed);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void UpgradeTurnRate(float BonusTurnRate);

protected:
	// Input handlers
	void IncreaseSail();
	void DecreaseSail();
	void TurnRight(float Value);
	void FireLeft();
	void FireRight();

private:
	void UpdateMovement(float DeltaTime);
	void UpdateTurning(float DeltaTime, float TurnInput);
	float GetTargetSpeed() const;
	float GetWindMultiplier() const;

	AWindSystem* CachedWindSystem;

	float TurnInputValue = 0.0f;

	// Speed penalty from chain shot
	float SpeedPenaltyFraction = 0.0f;
	float SpeedPenaltyTimeRemaining = 0.0f;
};
