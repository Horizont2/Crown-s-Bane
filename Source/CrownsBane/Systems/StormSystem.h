// Copyright 2024 Crown's Bane. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StormSystem.generated.h"

class AWindSystem;
class UNiagaraSystem;
class USoundBase;
class UAudioComponent;

UENUM(BlueprintType)
enum class EStormPhase : uint8
{
	Clear       UMETA(DisplayName = "Clear"),
	BuildingUp  UMETA(DisplayName = "Building Up"),
	Storm       UMETA(DisplayName = "Storm"),
	Dissipating UMETA(DisplayName = "Dissipating")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStormPhaseChanged, EStormPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStormIntensityChanged, float, NewIntensity);

/**
 * Manages periodic storms that modify the WindSystem, spawn rain/lightning FX,
 * and play ambient thunder. Storms cycle: Clear -> BuildingUp -> Storm -> Dissipating -> Clear.
 */
UCLASS()
class CROWNSBANE_API AStormSystem : public AActor
{
	GENERATED_BODY()

public:
	AStormSystem();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// ---- Timing ----
	// Average seconds between storms (at Clear phase)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storm|Timing")
	float MinClearDuration = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storm|Timing")
	float MaxClearDuration = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storm|Timing")
	float BuildupDuration = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storm|Timing")
	float MinStormDuration = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storm|Timing")
	float MaxStormDuration = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storm|Timing")
	float DissipateDuration = 12.0f;

	// ---- Wind Modulation ----
	// Wind strength multiplier at full storm (1.0 = normal, 2.5 = 2.5x stronger)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storm|Wind", meta=(ClampMin="1.0"))
	float StormWindStrengthMult = 2.5f;

	// Wind-change-rate multiplier at full storm (faster direction shifts)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storm|Wind", meta=(ClampMin="1.0"))
	float StormWindChangeRateMult = 4.0f;

	// ---- Lightning ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storm|Lightning")
	float MinLightningInterval = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storm|Lightning")
	float MaxLightningInterval = 12.0f;

	// Chance to spawn thunder sound when lightning flashes (during full storm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storm|Lightning", meta=(ClampMin="0.0", ClampMax="1.0"))
	float ThunderChance = 0.85f;

	// ---- FX Assets ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storm|FX")
	UNiagaraSystem* RainFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storm|FX")
	UNiagaraSystem* LightningFlashFX = nullptr;

	// Height offset above player where rain/storm FX is attached
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storm|FX")
	float RainHeightOffset = 600.0f;

	// Horizontal spread radius for lightning strikes around player
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storm|FX")
	float LightningSpawnRadius = 4000.0f;

	// ---- Audio ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storm|Audio")
	USoundBase* RainLoopSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storm|Audio")
	USoundBase* ThunderSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storm|Audio", meta=(ClampMin="0.0"))
	float RainVolumeScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storm|Audio", meta=(ClampMin="0.0"))
	float ThunderVolumeScale = 1.0f;

	// ---- State ----
	UPROPERTY(BlueprintReadOnly, Category = "Storm|State")
	EStormPhase CurrentPhase = EStormPhase::Clear;

	// Normalized intensity 0..1 (eases in/out across BuildingUp / Dissipating)
	UPROPERTY(BlueprintReadOnly, Category = "Storm|State")
	float CurrentIntensity = 0.0f;

	UPROPERTY(BlueprintAssignable, Category = "Storm|Events")
	FOnStormPhaseChanged OnStormPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "Storm|Events")
	FOnStormIntensityChanged OnStormIntensityChanged;

	UFUNCTION(BlueprintCallable, Category = "Storm")
	void ForceStartStorm();

	UFUNCTION(BlueprintCallable, Category = "Storm")
	void ForceClearWeather();

	UFUNCTION(BlueprintPure, Category = "Storm")
	EStormPhase GetStormPhase() const { return CurrentPhase; }

	UFUNCTION(BlueprintPure, Category = "Storm")
	float GetStormIntensity() const { return CurrentIntensity; }

	UFUNCTION(BlueprintPure, Category = "Storm")
	bool IsStormActive() const { return CurrentPhase != EStormPhase::Clear; }

private:
	void CacheWindSystem();
	void SetPhase(EStormPhase NewPhase);
	void UpdateIntensity(float DeltaTime);
	void UpdateWindModulation();
	void TickLightning(float DeltaTime);
	void SpawnLightning();
	void EnsureRainLoopRunning();
	void StopRainLoop();

	UPROPERTY() AWindSystem* CachedWind = nullptr;
	UPROPERTY() UAudioComponent* RainAudio = nullptr;

	float BaseWindStrength = 1.0f;
	float BaseWindChangeRate = 3.0f;
	bool bBaseCached = false;

	float PhaseTimeRemaining = 0.0f;
	float PhaseDuration = 0.0f;
	float LightningTimer = 0.0f;
};
