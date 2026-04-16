#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WantedLevelManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWantedLevelChanged, int32, NewWantedLevel);

UCLASS()
class CROWNSBANE_API AWantedLevelManager : public AActor
{
	GENERATED_BODY()

public:
	AWantedLevelManager();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// Call this whenever an enemy ship is destroyed by the player
	UFUNCTION(BlueprintCallable, Category = "Wanted")
	void OnEnemyKilled();

	// Reset wanted level (called when docking)
	UFUNCTION(BlueprintCallable, Category = "Wanted")
	void ResetWantedLevel();

	UFUNCTION(BlueprintPure, Category = "Wanted")
	int32 GetWantedLevel() const { return CurrentWantedLevel; }

	UFUNCTION(BlueprintPure, Category = "Wanted")
	int32 GetKillCount() const { return KillsSinceLastDock; }

	// Kills required per star level (indices 0-4 for stars 1-5)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wanted")
	TArray<int32> KillsPerWantedLevel;

	// Maximum wanted level (star count)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wanted")
	int32 MaxWantedLevel = 5;

	// At this wanted level and above, Galleons are dispatched
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wanted")
	int32 GalleonDispatchLevel = 4;

	UPROPERTY(BlueprintAssignable, Category = "Wanted")
	FOnWantedLevelChanged OnWantedLevelChanged;

	UPROPERTY(BlueprintReadOnly, Category = "Wanted")
	int32 CurrentWantedLevel = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wanted")
	int32 KillsSinceLastDock = 0;

private:
	void UpdateWantedLevel();
	bool bGalleonDispatched = false;
};
