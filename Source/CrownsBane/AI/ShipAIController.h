#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "ShipAIController.generated.h"

class AEnemyShipBase;

UCLASS()
class CROWNSBANE_API AShipAIController : public AAIController
{
	GENERATED_BODY()

public:
	AShipAIController();

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void Tick(float DeltaTime) override;

public:
	UFUNCTION(BlueprintCallable, Category = "AI")
	AEnemyShipBase* GetControlledShip() const { return ControlledShip; }

	// Force a specific target pawn (for scripted encounters)
	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetTarget(APawn* NewTarget);

	UPROPERTY(BlueprintReadOnly, Category = "AI")
	APawn* TargetPawn;

private:
	AEnemyShipBase* ControlledShip;
};
