#pragma once

#include "CoreMinimal.h"
#include "AI/EnemyShipBase.h"
#include "SloopShip.generated.h"

// Small, fast, low HP - 2 cannons per side, tries to keep distance
UCLASS()
class CROWNSBANE_API ASloopShip : public AEnemyShipBase
{
	GENERATED_BODY()

public:
	ASloopShip();

protected:
	virtual void BeginPlay() override;
	virtual void HandleStateAttack(float DeltaTime) override;
};
