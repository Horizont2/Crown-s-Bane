// Copyright 2024 Crown's Bane. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DynamicMusicManager.generated.h"

UENUM(BlueprintType)
enum class EMusicState : uint8
{
	Silent       UMETA(DisplayName = "Silent"),
	Ambient      UMETA(DisplayName = "Ambient — calm sailing"),
	Tension      UMETA(DisplayName = "Tension — enemies nearby"),
	Battle       UMETA(DisplayName = "Battle — engaged"),
	Storm        UMETA(DisplayName = "Storm — weather peak"),
	Victory      UMETA(DisplayName = "Victory — enemies just died")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMusicStateChanged, EMusicState, NewState, EMusicState, PreviousState);

/**
 * Picks a music state every Tick based on player surroundings: nearby enemies,
 * combat recency, storm intensity.  BP-side music actors listen to
 * OnMusicStateChanged and cross-fade their tracks accordingly.  All actual
 * audio assets are bound in BP — this actor is pure logic.
 */
UCLASS()
class CROWNSBANE_API ADynamicMusicManager : public AActor
{
	GENERATED_BODY()

public:
	ADynamicMusicManager();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// How frequently the music state is re-evaluated (seconds).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
	float EvaluationInterval = 0.5f;

	// Distance within which an enemy ship raises tension (cm).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
	float TensionRadius = 5000.0f;

	// Distance within which combat counts as engaged (cm).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
	float BattleRadius = 2500.0f;

	// How long after the last damage event the Battle state persists (seconds).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
	float BattleCooldown = 8.0f;

	// How long the Victory cue plays after the last enemy dies (seconds).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
	float VictoryDuration = 6.0f;

	// Storm intensity threshold above which the Storm state takes over (0..1).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
	float StormThreshold = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "Music")
	EMusicState CurrentState = EMusicState::Silent;

	UPROPERTY(BlueprintAssignable, Category = "Music")
	FOnMusicStateChanged OnMusicStateChanged;

	// Called by combat code on any hit so we can transition into Battle.
	UFUNCTION(BlueprintCallable, Category = "Music")
	void NotifyCombatHit();

	// Called by enemy death to start the Victory cue.
	UFUNCTION(BlueprintCallable, Category = "Music")
	void NotifyEnemyKilled();

private:
	void EvaluateState();

	float TimeSinceLastEval = 0.0f;
	float TimeSinceLastHit  = 999.0f;
	float TimeSinceLastKill = 999.0f;
};
