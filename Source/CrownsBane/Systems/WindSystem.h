#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WindSystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWindDirectionChanged, FVector, NewWindDirection);

UCLASS()
class CROWNSBANE_API AWindSystem : public AActor
{
	GENERATED_BODY()

public:
	AWindSystem();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// Returns a value in [-1, 1]:
	//   +1.0 = full tailwind (ship facing same as wind)
	//   -1.0 = full headwind (ship facing into wind)
	//   Used by ShipPawn as: speed *= (1 + dot * WindInfluenceFactor)
	UFUNCTION(BlueprintCallable, Category = "Wind")
	float GetWindSpeedMultiplier(FVector ShipForward) const;

	UFUNCTION(BlueprintPure, Category = "Wind")
	FVector GetWindDirection() const { return CurrentWindDirection; }

	UFUNCTION(BlueprintPure, Category = "Wind")
	float GetWindStrength() const { return WindStrength; }

	// Rate at which wind direction rotates (degrees per second)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind")
	float WindChangeRate = 3.0f;

	// Wind strength scale (0 = no wind effect, 1 = full 20% bonus/penalty)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind")
	float WindStrength = 1.0f;

	// How often to pick a new target wind direction (seconds)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind")
	float WindShiftInterval = 30.0f;

	UPROPERTY(BlueprintAssignable, Category = "Wind")
	FOnWindDirectionChanged OnWindDirectionChanged;

	UPROPERTY(BlueprintReadOnly, Category = "Wind")
	FVector CurrentWindDirection;

private:
	FVector TargetWindDirection;
	float WindShiftTimer = 0.0f;

	void PickNewTargetWindDirection();
};
