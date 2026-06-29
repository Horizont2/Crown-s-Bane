// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "Audio/SoundManager.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"

USoundManager::USoundManager()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USoundManager::Play(ESoundCue Cue, float Volume)
{
	USoundBase** Found = Cues.Find(Cue);
	if (!Found || !*Found) return;
	const float V = Volume * SFXVolume * MasterVolume;
	UGameplayStatics::PlaySound2D(GetWorld(), *Found, V);
}

void USoundManager::PlayAtLocation(ESoundCue Cue, FVector Location, float Volume)
{
	USoundBase** Found = Cues.Find(Cue);
	if (!Found || !*Found) return;
	const float V = Volume * SFXVolume * MasterVolume;
	UGameplayStatics::PlaySoundAtLocation(GetWorld(), *Found, Location, V);
}
