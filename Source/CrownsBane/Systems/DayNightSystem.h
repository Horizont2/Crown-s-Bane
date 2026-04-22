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

	// Full day-night cycle length in seconds
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DayNight", meta=(ClampMin="10"))
	float DayLengthSeconds = 600.0f;

	// Current time of day in hours (0–24). 6 = sunrise, 12 = noon, 18 = sunset, 0/24 = midnight.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DayNight", meta=(ClampMin="0", ClampMax="24"))
	float TimeOfDay = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DayNight")
	float DaySunIntensity = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DayNight")
	float NightMoonIntensity = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DayNight")
	FLinearColor NoonColor = FLinearColor(1.0f, 0.96f, 0.85f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DayNight")
	FLinearColor DawnColor = FLinearColor(1.0f, 0.55f, 0.35f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DayNight")
	FLinearColor NightColor = FLinearColor(0.15f, 0.2f, 0.45f);

	UPROPERTY(BlueprintAssignable, Category = "DayNight")
	FOnTimeOfDayChanged OnTimeOfDayChanged;

	UFUNCTION(BlueprintPure, Category = "DayNight")
	bool IsNight() const { return TimeOfDay < 6.0f || TimeOfDay >= 19.0f; }

	UFUNCTION(BlueprintCallable, Category = "DayNight")
	void SetTimeOfDay(float NewHours);

private:
	UPROPERTY() ADirectionalLight* SunLight = nullptr;

	void CacheSun();
	void ApplySunState();
};
