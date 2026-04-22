// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "Systems/DayNightSystem.h"
#include "Engine/DirectionalLight.h"
#include "Components/DirectionalLightComponent.h"
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

	if (!SunLight)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DayNightSystem] No DirectionalLight found in level — day/night disabled."));
	}
}

void ADayNightSystem::ApplySunState()
{
	if (!SunLight) return;

	// Map time of day to sun pitch: 6h = sunrise (pitch=0), 12h = noon (pitch=-90),
	// 18h = sunset (pitch=0), 0h = midnight (pitch=+90, below horizon).
	const float Angle = (TimeOfDay / 24.0f) * 360.0f - 90.0f;
	SunLight->SetActorRotation(FRotator(Angle, 40.0f, 0.0f));

	// Day/night state
	const bool bIsNight = IsNight();
	UDirectionalLightComponent* Comp = Cast<UDirectionalLightComponent>(SunLight->GetLightComponent());
	if (!Comp) return;

	// Smooth blend between noon and night. Use cosine-like curve.
	// t = 0 at midnight, 1 at noon, via smoothstep
	const float HoursFromNoon = FMath::Abs(FMath::Fmod(TimeOfDay + 12.0f, 24.0f) - 12.0f);
	const float DayFactor = FMath::Clamp(1.0f - HoursFromNoon / 8.0f, 0.0f, 1.0f);

	const float Intensity = FMath::Lerp(NightMoonIntensity, DaySunIntensity, DayFactor);
	Comp->SetIntensity(Intensity);

	// Color: dawn/dusk transition around 6h and 18h
	FLinearColor TargetColor;
	if (TimeOfDay >= 5.0f && TimeOfDay <= 8.0f)
	{
		// Dawn
		const float A = (TimeOfDay - 5.0f) / 3.0f;
		TargetColor = FMath::Lerp(NightColor, DawnColor, FMath::Clamp(A * 0.7f, 0.f, 0.7f));
		TargetColor = FMath::Lerp(TargetColor, NoonColor, FMath::Clamp((A - 0.5f) * 2.0f, 0.f, 1.f));
	}
	else if (TimeOfDay >= 17.0f && TimeOfDay <= 20.0f)
	{
		// Dusk
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
}
