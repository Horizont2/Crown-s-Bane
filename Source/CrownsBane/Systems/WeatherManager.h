// Copyright 2024 Crown's Bane. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeatherManager.generated.h"

class AWindSystem;

UENUM(BlueprintType)
enum class EWeatherState : uint8
{
	Normal      UMETA(DisplayName = "Normal"),
	Fog         UMETA(DisplayName = "Fog"),
	Calm        UMETA(DisplayName = "Calm"),
	TradeWinds  UMETA(DisplayName = "Trade Winds")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeatherChanged, EWeatherState, NewState, EWeatherState, PrevState);

/**
 * Cycles the ocean between calm / fog / trade-wind windows so long
 * sailing sessions get natural variation.  StormSystem still owns
 * proper storms; this handles the "quiet weather" spectrum.
 */
UCLASS()
class CROWNSBANE_API AWeatherManager : public AActor
{
	GENERATED_BODY()

public:
	AWeatherManager();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float MinDuration = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float MaxDuration = 180.0f;

	// Chance weight per state on each roll (Normal 50%, others share the rest).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float WeightNormal = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float WeightFog = 0.2f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float WeightCalm = 0.15f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float WeightTradeWinds = 0.15f;

	UPROPERTY(BlueprintReadOnly, Category = "Weather")
	EWeatherState CurrentState = EWeatherState::Normal;

	UPROPERTY(BlueprintAssignable, Category = "Weather")
	FOnWeatherChanged OnWeatherChanged;

	UFUNCTION(BlueprintCallable, Category = "Weather")
	void SetState(EWeatherState NewState);

	UFUNCTION(BlueprintPure, Category = "Weather")
	bool IsFog()        const { return CurrentState == EWeatherState::Fog; }
	UFUNCTION(BlueprintPure, Category = "Weather")
	bool IsCalm()       const { return CurrentState == EWeatherState::Calm; }
	UFUNCTION(BlueprintPure, Category = "Weather")
	bool IsTradeWinds() const { return CurrentState == EWeatherState::TradeWinds; }

private:
	float TimeUntilChange = 60.0f;
	void RollNextState();
	void ApplyToWindSystem();
};
