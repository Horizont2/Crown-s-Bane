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
	Patrol  UMETA(DisplayName = "Patrol"),
	Chase   UMETA(DisplayName = "Chase"),
	Attack  UMETA(DisplayName = "Attack"),
	Retreat UMETA(DisplayName = "Retreat"),
	Sink    UMETA(DisplayName = "Sink")
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX", meta=(ClampMin="0.0", ClampMax="1.0"))
	float SmokeHPThreshold = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX", meta=(ClampMin="0.0", ClampMax="1.0"))
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

	// HP fraction at which this ship abandons combat and flees (0 = never retreat)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta=(ClampMin="0.0", ClampMax="1.0"))
	float RetreatHealthThreshold = 0.25f;

	// Speed during retreat (usually faster than chase)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float RetreatSpeed = 700.0f;

	// Set false in subclasses that should never retreat (e.g. enraged Galleon)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	bool bCanRetreat = true;

	// If true, this ship attacks on sight regardless of wanted level (e.g. scripted bosses).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	bool bIgnoreWantedLevel = false;

	// Turns true when the player damages us: we return fire even without wanted level.
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	bool bHasAggro = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float FireCooldown = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float PatrolRadius = 3000.0f;

	UPROPERTY(BlueprintReadOnly, Category = "AI")
	EShipAIState CurrentState = EShipAIState::Patrol;

	// Loot class spawned on death
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	TSubclassOf<AActor> LootSpawnerClass;

	// ---- Movement ----
	UFUNCTION(BlueprintCallable, Category = "AI")
	void MoveToward(FVector TargetLocation, float Speed, float DeltaTime);

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
	float PatrolWaitTimer = 0.0f;

	UFUNCTION()
	void OnDeathDelegate();

	UFUNCTION()
	void OnHealthChangedHandler(float CurrentHealth, float MaxHealth);
};
