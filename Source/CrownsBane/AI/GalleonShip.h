#pragma once

#include "CoreMinimal.h"
#include "AI/EnemyShipBase.h"
#include "GalleonShip.generated.h"

// Slow, huge HP - 10 cannons per side, boss fight tactics
UCLASS()
class CROWNSBANE_API AGalleonShip : public AEnemyShipBase
{
	GENERATED_BODY()

public:
	AGalleonShip();

protected:
	virtual void BeginPlay() override;
	virtual void HandleStateChase(float DeltaTime) override;
	virtual void HandleStateAttack(float DeltaTime) override;

	// Boss phase: enrage when below 30% health
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Galleon")
	float EnrageHealthThreshold = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Galleon")
	float EnragedSpeedMultiplier = 1.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Galleon")
	float EnragedFireCooldownMultiplier = 0.5f;

	bool bIsEnraged = false;

private:
	void CheckEnrage();
	float BaseFireCooldown = 6.0f;
	float BaseChaseSpeed = 350.0f;
};
