#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Combat/CannonComponent.h"
#include "EnemyShipBase.generated.h"

class UStaticMeshComponent;
class UHealthComponent;
class ALootSpawner;
class AWantedLevelManager;
class UNiagaraComponent;
class UNiagaraSystem;
class USoundBase;

UENUM(BlueprintType)
enum class EShipAIState : uint8
{
	Patrol    UMETA(DisplayName = "Patrol"),
	Chase     UMETA(DisplayName = "Chase"),
	Attack    UMETA(DisplayName = "Attack"),
	Evasive   UMETA(DisplayName = "Evasive Maneuver"),
	Retreat   UMETA(DisplayName = "Retreat"),
	Surrender UMETA(DisplayName = "Surrender"),
	Sink      UMETA(DisplayName = "Sink")
};

UCLASS(Abstract)
class CROWNSBANE_API AEnemyShipBase : public APawn
{
	GENERATED_BODY()

public:
	AEnemyShipBase();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	// ---- Components ----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ShipMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCannonComponent* CannonComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UHealthComponent* HealthComponent;

	// ---- Damage FX ----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FX")
	UNiagaraComponent* DamageSmokeFX;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FX")
	UNiagaraComponent* DamageFireFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
	UNiagaraSystem* SmokeAsset = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
	UNiagaraSystem* FireAsset = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
	UNiagaraSystem* DeathExplosionAsset = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
	USoundBase* DeathSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SmokeHPThreshold = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FireHPThreshold = 0.3f;

	// ---- AI Settings ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float DetectionRange = 4000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float AttackRange = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float FireRange = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float PreferredEngagementDistance = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float PatrolSpeed = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float ChaseSpeed = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float AttackSpeed = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float TurnRate = 35.0f;

	// AI Physics & Inertia
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Physics")
	float AccelerationInterpSpeed = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Physics")
	float TurnInterpSpeed = 1.5f;

	// AI Obstacle Avoidance
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Navigation")
	float AvoidanceRayLength = 2500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RetreatHealthThreshold = 0.25f;

	// At or below this HP fraction, ship considers surrender (10%) — once it has
	// taken at least one hit and the player is close enough.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SurrenderHealthThreshold = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta=(ClampMin="0.0", ClampMax="1.0"))
	float SurrenderChance = 0.5f;

	// Cooldown between evasive maneuvers (seconds).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float EvasiveCooldown = 6.0f;

	float TimeSinceLastEvasive = 99.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float EvasiveDuration = 1.5f;

	float EvasiveTimeRemaining = 0.0f;
	float EvasiveTurnSign = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	bool bCanEvade = true;

	// Named/legendary ship? Triggers boss intro banner + kill cam.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Named")
	bool bNamedEnemy = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Named")
	FString NamedTitle = TEXT("");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Named")
	FString NamedSubtitle = TEXT("");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Named")
	FLinearColor NamedTint = FLinearColor(1.0f, 0.3f, 0.3f, 1.0f);

	// XP multiplier for kills (bounties/legendary yield more)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Named")
	float XPMultiplier = 1.0f;

	// One-shot: intro banner has been shown for this instance.
	bool bIntroShown = false;
	void CheckAndShowNamedIntro();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float RetreatSpeed = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	bool bCanRetreat = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	bool bIgnoreWantedLevel = false;

	UPROPERTY(BlueprintReadOnly, Category = "AI")
	bool bHasAggro = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float FireCooldown = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float PatrolRadius = 3000.0f;

	UPROPERTY(BlueprintReadOnly, Category = "AI")
	EShipAIState CurrentState = EShipAIState::Patrol;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	TSubclassOf<AActor> LootSpawnerClass;

	// ---- Movement ----
	UFUNCTION(BlueprintCallable, Category = "AI")
	void MoveToward(FVector TargetLocation, float TargetSpeed, float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "AI")
	void TurnToward(FVector TargetLocation, float DeltaTime);

	UFUNCTION(BlueprintPure, Category = "AI")
	EShipAIState GetAIState() const { return CurrentState; }

	UFUNCTION(BlueprintPure, Category = "AI")
	float GetCurrentSpeed() const { return CurrentSpeedActual; }

protected:
	virtual void OnDeath();
	virtual void HandleStatePatrol(float DeltaTime);
	virtual void HandleStateChase(float DeltaTime);
	virtual void HandleStateAttack(float DeltaTime);
	virtual void HandleStateRetreat(float DeltaTime);
	virtual void HandleStateSink(float DeltaTime);

	void TransitionToState(EShipAIState NewState);
	bool IsPlayerInRange(float Range) const;
	APawn* GetPlayerPawn() const;
	bool IsBroadsideAligned(ECannonSide& OutSide) const;
	void TryFireAtPlayer();
	void UpdateDamageFX();

	FVector SpawnLocation;
	FVector PatrolTarget;

	float FireCooldownTimer = 0.0f;
	float SinkTimer = 0.0f;
	float CurrentSpeedActual = 0.0f;
	float CurrentYawSpeed = 0.0f; // ����� ��� �������� ��������
	float PatrolWaitTimer = 0.0f;

	UFUNCTION()
	void OnDeathDelegate();

	UFUNCTION()
	void OnHealthChangedHandler(float CurrentHealth, float MaxHealth);
};