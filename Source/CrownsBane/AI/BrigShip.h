#pragma once

#include "CoreMinimal.h"
#include "AI/EnemyShipBase.h"
#include "BrigShip.generated.h"

// Medium, balanced - 4 cannons per side, closes in for broadside
UCLASS()
class CROWNSBANE_API ABrigShip : public AEnemyShipBase
{
	GENERATED_BODY()

public:
	ABrigShip();

protected:
	virtual void BeginPlay() override;
	virtual void HandleStateAttack(float DeltaTime) override;
};
