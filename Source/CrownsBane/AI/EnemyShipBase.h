#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Combat/CannonComponent.h"
#include "EnemyShipBase.generated.h"

class UStaticMeshComponent;
class UHealthComponent;
class ALootSpawner;
class AWantedLevelManager;

UENUM(BlueprintType)
enum class EShipAIState : uint8
{
	Patrol  UMETA(DisplayName = "Patrol"),
	Chase   UMETA(DisplayName = "Chase"),
	Attack  UMETA(DisplayName = "Attack"),
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

	// ---- AI Settings ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float DetectionRange = 4000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float AttackRange = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float FireRange = 1800.0f;

	// Desired distance from player (for sloops that keep distance)
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

	// Time between fire attempts
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float FireCooldown = 5.0f;

	// Patrol radius around spawn point
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
	virtual void HandleStateSink(float DeltaTime);

	void TransitionToState(EShipAIState NewState);
	bool IsPlayerInRange(float Range) const;
	APawn* GetPlayerPawn() const;
	bool IsBroadsideAligned(ECannonSide& OutSide) const;
	void TryFireAtPlayer();

	FVector SpawnLocation;
	FVector PatrolTarget;

	float FireCooldownTimer = 0.0f;
	float SinkTimer = 0.0f;
	float CurrentSpeedActual = 0.0f;
	float PatrolWaitTimer = 0.0f;

	UFUNCTION()
	void OnDeathDelegate();
};
