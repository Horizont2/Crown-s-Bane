// Copyright 2024 Crown's Bane. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DayNightSystem.generated.h"

class ADirectionalLight;
class ASkyLight;
class AExponentialHeightFog;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimeOfDayChanged, float, NewTimeOfDay);

/**
 * Rotates a level's DirectionalLight over a configurable day length and
 * dims intensity at night. Designers drop a DirectionalLight into the level
 * and the system finds it automatically; no manual wiring needed.
 */
UCLASS()
class CROWNSBANE_API ADayNightSystem : public AActor
{
	GENERATED_BODY()

public:
	ADayNightSystem();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// Full day-night cycle length in seconds (default 10 minutes).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DayNight", meta=(ClampMin="10"))
	float DayLengthSeconds = 600.0f;

	// Start time (hours).  9.0 = mid-morning — player always begins their session
	// in clear daylight before the night cycle rolls in.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DayNight", meta=(ClampMin="0", ClampMax="24"))
	float TimeOfDay = 9.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DayNight")
	float DaySunIntensity = 10.0f;

	// Realistic moonlight intensity — still dim but unmistakably visible.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DayNight")
	float NightMoonIntensity = 2.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DayNight")
	FLinearColor NoonColor = FLinearColor(1.0f, 0.96f, 0.85f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DayNight")
	FLinearColor DawnColor = FLinearColor(1.0f, 0.55f, 0.35f);

	// Cool bluish moonlight — brighter than pitch black so the world stays legible.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DayNight")
	FLinearColor NightColor = FLinearColor(0.55f, 0.7f, 1.0f);

	// Sky/ambient tints (applied to sky-light when one is present)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DayNight|Sky")
	float DaySkyIntensity = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DayNight|Sky")
	float NightSkyIntensity = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DayNight|Sky")
	FLinearColor NightSkyColor = FLinearColor(0.15f, 0.22f, 0.45f);

	UPROPERTY(BlueprintAssignable, Category = "DayNight")
	FOnTimeOfDayChanged OnTimeOfDayChanged;

	UFUNCTION(BlueprintPure, Category = "DayNight")
	bool IsNight() const { return TimeOfDay < 6.0f || TimeOfDay >= 19.0f; }

	UFUNCTION(BlueprintCallable, Category = "DayNight")
	void SetTimeOfDay(float NewHours);

	// Useful for HUD: 0..1 day factor (0 = midnight, 1 = noon).
	UFUNCTION(BlueprintPure, Category = "DayNight")
	float GetDayFactor() const;

private:
	UPROPERTY() ADirectionalLight* SunLight = nullptr;
	UPROPERTY() ASkyLight*         SkyLight = nullptr;

	void CacheSun();
	void ApplySunState();
};
