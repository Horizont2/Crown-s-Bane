// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "Systems/DayNightSystem.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"

ADayNightSystem::ADayNightSystem()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ADayNightSystem::BeginPlay()
{
	Super::BeginPlay();
	CacheSun();
	ApplySunState();
}

void ADayNightSystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!SunLight) CacheSun();

	// 24 game-hours per DayLengthSeconds
	const float HoursPerSecond = 24.0f / FMath::Max(DayLengthSeconds, 1.0f);
	TimeOfDay += DeltaTime * HoursPerSecond;
	if (TimeOfDay >= 24.0f) TimeOfDay -= 24.0f;

	ApplySunState();
	OnTimeOfDayChanged.Broadcast(TimeOfDay);
}

void ADayNightSystem::SetTimeOfDay(float NewHours)
{
	TimeOfDay = FMath::Fmod(FMath::Max(0.0f, NewHours), 24.0f);
	ApplySunState();
	OnTimeOfDayChanged.Broadcast(TimeOfDay);
}

void ADayNightSystem::CacheSun()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// Pick the first DirectionalLight as the sun.
	for (TActorIterator<ADirectionalLight> It(World); It; ++It)
	{
		SunLight = *It;
		if (SunLight) break;
	}

	for (TActorIterator<ASkyLight> It(World); It; ++It)
	{
		SkyLight = *It;
		if (SkyLight) break;
	}

	if (!SunLight)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DayNightSystem] No DirectionalLight found in level — day/night disabled."));
	}
}

float ADayNightSystem::GetDayFactor() const
{
	const float HoursFromNoon = FMath::Abs(FMath::Fmod(TimeOfDay + 12.0f, 24.0f) - 12.0f);
	return FMath::Clamp(1.0f - HoursFromNoon / 8.0f, 0.0f, 1.0f);
}

void ADayNightSystem::ApplySunState()
{
	if (!SunLight) return;

	// Correct sun arc: sinusoidal altitude.
	//   T=0  (midnight) → altitude = -90  (sun below horizon, pitch=+90)
	//   T=6  (sunrise)  → altitude =   0  (horizon,            pitch=  0)
	//   T=12 (noon)     → altitude = +90  (overhead,           pitch=-90)
	//   T=18 (sunset)   → altitude =   0
	//
	// Pitch = -altitude.  Using cos(T/24 · 2π)·90 gives the correct sign.
	const float Pitch = FMath::Cos(TimeOfDay / 24.0f * 2.0f * PI) * 90.0f;
	SunLight->SetActorRotation(FRotator(Pitch, 40.0f, 0.0f));

	const bool bIsNight = IsNight();
	UDirectionalLightComponent* Comp = SunLight->GetComponent();
	if (!Comp) return;

	const float DayFactor = GetDayFactor();

	// At night, rather than dimming to near-zero (which makes the scene pitch-black
	// because the sun is below the horizon and contributes no direct light),
	// FLIP the light to point downward with a dim blue intensity — simulating a moon.
	float Intensity = FMath::Lerp(NightMoonIntensity, DaySunIntensity, DayFactor);

	if (bIsNight && Pitch > 0.0f) // sun is below horizon
	{
		// Override rotation so the "moon" shines from above at a fixed high angle.
		SunLight->SetActorRotation(FRotator(-55.0f, 40.0f, 0.0f));
		Intensity = NightMoonIntensity;
	}

	Comp->SetIntensity(Intensity);

	// Color blending across dawn/day/dusk/night
	FLinearColor TargetColor;
	if (TimeOfDay >= 5.0f && TimeOfDay <= 8.0f)
	{
		const float A = (TimeOfDay - 5.0f) / 3.0f;
		TargetColor = FMath::Lerp(NightColor, DawnColor, FMath::Clamp(A * 0.7f, 0.f, 0.7f));
		TargetColor = FMath::Lerp(TargetColor, NoonColor, FMath::Clamp((A - 0.5f) * 2.0f, 0.f, 1.f));
	}
	else if (TimeOfDay >= 17.0f && TimeOfDay <= 20.0f)
	{
		const float A = (TimeOfDay - 17.0f) / 3.0f;
		TargetColor = FMath::Lerp(NoonColor, DawnColor, FMath::Clamp(A, 0.f, 1.f));
		TargetColor = FMath::Lerp(TargetColor, NightColor, FMath::Clamp((A - 0.7f) * 3.3f, 0.f, 1.f));
	}
	else if (bIsNight)
	{
		TargetColor = NightColor;
	}
	else
	{
		TargetColor = NoonColor;
	}
	Comp->SetLightColor(TargetColor);

	// Sky light: dim at night but keep ambient fill so the ocean/ships stay readable.
	if (SkyLight)
	{
		if (USkyLightComponent* Sky = SkyLight->GetLightComponent())
		{
			const float SkyIntensity = FMath::Lerp(NightSkyIntensity, DaySkyIntensity, DayFactor);
			Sky->SetIntensity(SkyIntensity);
			Sky->SetLightColor(bIsNight ? NightSkyColor : FLinearColor::White);
		}
	}
}
