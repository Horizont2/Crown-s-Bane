// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "Systems/StormSystem.h"
#include "Systems/WindSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Pawn.h"

AStormSystem::AStormSystem()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AStormSystem::BeginPlay()
{
	Super::BeginPlay();

	CacheWindSystem();

	// Start in Clear phase with random countdown.
	CurrentPhase = EStormPhase::Clear;
	PhaseDuration = FMath::FRandRange(MinClearDuration, MaxClearDuration);
	PhaseTimeRemaining = PhaseDuration;
	CurrentIntensity = 0.0f;

	UE_LOG(LogTemp, Log, TEXT("[StormSystem] Initialized. Next storm in %.1fs"), PhaseTimeRemaining);
}

void AStormSystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// WindSystem may spawn after us; keep trying to cache.
	if (!CachedWind)
	{
		CacheWindSystem();
	}

	PhaseTimeRemaining -= DeltaTime;

	UpdateIntensity(DeltaTime);
	UpdateWindModulation();

	if (CurrentPhase == EStormPhase::Storm || CurrentPhase == EStormPhase::Dissipating)
	{
		TickLightning(DeltaTime);
	}

	if (PhaseTimeRemaining > 0.0f)
	{
		return;
	}

	// Advance phase.
	switch (CurrentPhase)
	{
	case EStormPhase::Clear:
		SetPhase(EStormPhase::BuildingUp);
		PhaseDuration = BuildupDuration;
		break;
	case EStormPhase::BuildingUp:
		SetPhase(EStormPhase::Storm);
		PhaseDuration = FMath::FRandRange(MinStormDuration, MaxStormDuration);
		break;
	case EStormPhase::Storm:
		SetPhase(EStormPhase::Dissipating);
		PhaseDuration = DissipateDuration;
		break;
	case EStormPhase::Dissipating:
		SetPhase(EStormPhase::Clear);
		PhaseDuration = FMath::FRandRange(MinClearDuration, MaxClearDuration);
		break;
	}

	PhaseTimeRemaining = PhaseDuration;
}

void AStormSystem::CacheWindSystem()
{
	UWorld* World = GetWorld();
	if (!World) return;
	for (TActorIterator<AWindSystem> It(World); It; ++It)
	{
		CachedWind = *It;
		if (CachedWind && !bBaseCached)
		{
			BaseWindStrength = CachedWind->WindStrength;
			BaseWindChangeRate = CachedWind->WindChangeRate;
			bBaseCached = true;
		}
		break;
	}
}

void AStormSystem::SetPhase(EStormPhase NewPhase)
{
	if (NewPhase == CurrentPhase) return;

	CurrentPhase = NewPhase;
	OnStormPhaseChanged.Broadcast(NewPhase);

	switch (NewPhase)
	{
	case EStormPhase::Clear:
		UE_LOG(LogTemp, Log, TEXT("[StormSystem] Weather clearing."));
		StopRainLoop();
		break;
	case EStormPhase::BuildingUp:
		UE_LOG(LogTemp, Log, TEXT("[StormSystem] Storm building up..."));
		EnsureRainLoopRunning();
		break;
	case EStormPhase::Storm:
		UE_LOG(LogTemp, Warning, TEXT("[StormSystem] STORM! Wind x%.1f, rate x%.1f"),
			StormWindStrengthMult, StormWindChangeRateMult);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan,
				TEXT("A storm is rolling in!"));
		}
		EnsureRainLoopRunning();
		break;
	case EStormPhase::Dissipating:
		UE_LOG(LogTemp, Log, TEXT("[StormSystem] Storm dissipating."));
		break;
	}
}

void AStormSystem::UpdateIntensity(float DeltaTime)
{
	float Target = 0.0f;
	switch (CurrentPhase)
	{
	case EStormPhase::Clear:       Target = 0.0f; break;
	case EStormPhase::BuildingUp:
	{
		// Ramp 0->1 across BuildupDuration
		const float Elapsed = PhaseDuration - PhaseTimeRemaining;
		Target = FMath::Clamp(Elapsed / FMath::Max(BuildupDuration, 0.01f), 0.0f, 1.0f);
		break;
	}
	case EStormPhase::Storm:       Target = 1.0f; break;
	case EStormPhase::Dissipating:
	{
		// Ramp 1->0 across DissipateDuration
		const float Elapsed = PhaseDuration - PhaseTimeRemaining;
		Target = FMath::Clamp(1.0f - (Elapsed / FMath::Max(DissipateDuration, 0.01f)), 0.0f, 1.0f);
		break;
	}
	}

	const float Prev = CurrentIntensity;
	// Mild smoothing so per-frame jitter is avoided.
	CurrentIntensity = FMath::FInterpTo(CurrentIntensity, Target, DeltaTime, 4.0f);

	if (!FMath::IsNearlyEqual(Prev, CurrentIntensity, 0.005f))
	{
		OnStormIntensityChanged.Broadcast(CurrentIntensity);
	}

	if (RainAudio)
	{
		RainAudio->SetVolumeMultiplier(CurrentIntensity * RainVolumeScale);
	}
}

void AStormSystem::UpdateWindModulation()
{
	if (!CachedWind || !bBaseCached) return;

	const float StrengthScale = FMath::Lerp(1.0f, StormWindStrengthMult, CurrentIntensity);
	const float RateScale = FMath::Lerp(1.0f, StormWindChangeRateMult, CurrentIntensity);

	CachedWind->WindStrength = BaseWindStrength * StrengthScale;
	CachedWind->WindChangeRate = BaseWindChangeRate * RateScale;
}

void AStormSystem::TickLightning(float DeltaTime)
{
	if (CurrentIntensity < 0.25f) return;

	LightningTimer -= DeltaTime;
	if (LightningTimer > 0.0f) return;

	SpawnLightning();

	// Shorter intervals at higher intensity.
	const float IntervalScale = FMath::Lerp(1.5f, 0.6f, CurrentIntensity);
	LightningTimer = FMath::FRandRange(MinLightningInterval, MaxLightningInterval) * IntervalScale;
}

void AStormSystem::SpawnLightning()
{
	UWorld* World = GetWorld();
	if (!World) return;

	APawn* Player = UGameplayStatics::GetPlayerPawn(World, 0);
	if (!Player) return;

	const FVector PlayerLoc = Player->GetActorLocation();
	const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
	const float Dist = FMath::FRandRange(LightningSpawnRadius * 0.4f, LightningSpawnRadius);
	const FVector StrikeLoc = PlayerLoc + FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 800.0f);

	if (LightningFlashFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, LightningFlashFX, StrikeLoc, FRotator::ZeroRotator);
	}

	if (ThunderSound && FMath::FRand() <= ThunderChance)
	{
		// Louder when nearer; attenuate by distance.
		const float DistSq = FVector::DistSquared(PlayerLoc, StrikeLoc);
		const float Atten = FMath::Clamp(1.0f - (DistSq / FMath::Square(LightningSpawnRadius * 1.25f)), 0.2f, 1.0f);
		UGameplayStatics::PlaySoundAtLocation(World, ThunderSound, StrikeLoc,
			ThunderVolumeScale * Atten * FMath::Max(CurrentIntensity, 0.3f));
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.4f, FColor::White, TEXT("*** LIGHTNING ***"));
	}
}

void AStormSystem::EnsureRainLoopRunning()
{
	if (RainAudio && RainAudio->IsPlaying()) return;
	if (!RainLoopSound) return;

	UWorld* World = GetWorld();
	if (!World) return;

	APawn* Player = UGameplayStatics::GetPlayerPawn(World, 0);
	if (!Player) return;

	// Spawn attached so rain follows the player.
	RainAudio = UGameplayStatics::SpawnSoundAttached(
		RainLoopSound,
		Player->GetRootComponent(),
		NAME_None,
		FVector(0.0f, 0.0f, RainHeightOffset),
		EAttachLocation::KeepRelativeOffset,
		/*bStopWhenAttachedToDestroyed=*/ true,
		/*VolumeMultiplier=*/ 0.0f,
		/*PitchMultiplier=*/ 1.0f
	);
}

void AStormSystem::StopRainLoop()
{
	if (RainAudio)
	{
		RainAudio->FadeOut(1.5f, 0.0f);
		RainAudio = nullptr;
	}
}

void AStormSystem::ForceStartStorm()
{
	SetPhase(EStormPhase::BuildingUp);
	PhaseDuration = BuildupDuration;
	PhaseTimeRemaining = PhaseDuration;
}

void AStormSystem::ForceClearWeather()
{
	SetPhase(EStormPhase::Dissipating);
	PhaseDuration = DissipateDuration;
	PhaseTimeRemaining = PhaseDuration;
}
