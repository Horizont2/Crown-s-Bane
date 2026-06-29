// Copyright 2024 Crown's Bane. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SoundManager.generated.h"

class USoundBase;

UENUM(BlueprintType)
enum class ESoundCue : uint8
{
	CannonFire   UMETA(DisplayName = "Cannon Fire"),
	Impact       UMETA(DisplayName = "Hit Impact"),
	WaterSplash  UMETA(DisplayName = "Water Splash"),
	DamageTaken  UMETA(DisplayName = "Took Damage"),
	LevelUp      UMETA(DisplayName = "Level Up"),
	PerkUnlock   UMETA(DisplayName = "Perk Unlocked"),
	QuestDone    UMETA(DisplayName = "Quest Complete"),
	UIClick      UMETA(DisplayName = "UI Click"),
	UIOpen       UMETA(DisplayName = "UI Open"),
	BoardingWin  UMETA(DisplayName = "Boarding Won"),
	StormStart   UMETA(DisplayName = "Storm Approaching")
};

/**
 * Lightweight audio cue dispatcher.  Attach to PlayerController.  Combat
 * and UI code calls Play(ESoundCue, Volume) without knowing about the
 * actual sound assets — designers wire them in via the TMap in the
 * Details panel.  Master/SFX/Music volumes layered on every cue.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CROWNSBANE_API USoundManager : public UActorComponent
{
	GENERATED_BODY()

public:
	USoundManager();

	UFUNCTION(BlueprintCallable, Category = "Audio")
	void Play(ESoundCue Cue, float Volume = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Audio")
	void PlayAtLocation(ESoundCue Cue, FVector Location, float Volume = 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Cues")
	TMap<ESoundCue, USoundBase*> Cues;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Mix", meta=(ClampMin="0.0", ClampMax="2.0"))
	float MasterVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Mix", meta=(ClampMin="0.0", ClampMax="2.0"))
	float SFXVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Mix", meta=(ClampMin="0.0", ClampMax="2.0"))
	float MusicVolume = 0.8f;
};
