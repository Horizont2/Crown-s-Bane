// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "Systems/WeatherManager.h"
#include "Systems/WindSystem.h"
#include "UI/CrownsBaneHUD.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"

AWeatherManager::AWeatherManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AWeatherManager::BeginPlay()
{
	Super::BeginPlay();
	TimeUntilChange = FMath::FRandRange(MinDuration, MaxDuration);
}

void AWeatherManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	TimeUntilChange -= DeltaTime;
	if (TimeUntilChange <= 0.0f)
	{
		RollNextState();
		TimeUntilChange = FMath::FRandRange(MinDuration, MaxDuration);
	}
}

void AWeatherManager::RollNextState()
{
	const float Total = FMath::Max(0.001f, WeightNormal + WeightFog + WeightCalm + WeightTradeWinds);
	const float R = FMath::FRand() * Total;
	EWeatherState Next = EWeatherState::Normal;
	float Acc = WeightNormal;
	if      (R < Acc)                    Next = EWeatherState::Normal;
	else if (R < (Acc += WeightFog))     Next = EWeatherState::Fog;
	else if (R < (Acc += WeightCalm))    Next = EWeatherState::Calm;
	else                                  Next = EWeatherState::TradeWinds;
	SetState(Next);
}

void AWeatherManager::SetState(EWeatherState NewState)
{
	if (NewState == CurrentState) return;
	const EWeatherState Prev = CurrentState;
	CurrentState = NewState;
	OnWeatherChanged.Broadcast(NewState, Prev);
	ApplyToWindSystem();

	if (UWorld* W = GetWorld())
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(W, 0))
		{
			if (ACrownsBaneHUD* HUD = Cast<ACrownsBaneHUD>(PC->GetHUD()))
			{
				const TCHAR* Msg = TEXT("");
				FLinearColor Tint = FLinearColor::White;
				switch (NewState)
				{
				case EWeatherState::Fog:        Msg = TEXT("☁ Fog rolls in — visibility falling"); Tint = FLinearColor(0.7f, 0.75f, 0.8f, 1.f);   break;
				case EWeatherState::Calm:       Msg = TEXT("~ Sea grows calm — no wind");           Tint = FLinearColor(0.6f, 0.8f, 0.9f, 1.f);    break;
				case EWeatherState::TradeWinds: Msg = TEXT("↝ Trade winds picking up");             Tint = FLinearColor(0.4f, 0.85f, 0.5f, 1.f);   break;
				case EWeatherState::Normal:
				default:                        Msg = TEXT("Weather returns to normal");           Tint = FLinearColor(0.85f, 0.85f, 0.85f, 1.f); break;
				}
				HUD->PushResourceToast(Msg, Tint);
			}
		}
	}
}

void AWeatherManager::ApplyToWindSystem()
{
	if (UWorld* W = GetWorld())
	{
		for (TActorIterator<AWindSystem> It(W); It; ++It)
		{
			if (AWindSystem* Wind = *It)
			{
				switch (CurrentState)
				{
				case EWeatherState::Calm:       Wind->WindStrength = 0.05f; break;
				case EWeatherState::TradeWinds: Wind->WindStrength = 1.5f;  break;
				case EWeatherState::Fog:        Wind->WindStrength = 0.4f;  break;
				case EWeatherState::Normal:
				default:                        Wind->WindStrength = 1.0f;  break;
				}
				break;
			}
		}
	}
}
