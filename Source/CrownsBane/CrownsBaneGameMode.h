#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CrownsBaneGameMode.generated.h"

class AWantedLevelManager;
class AWindSystem;
class AEnemySpawner;

UCLASS(minimalapi)
class ACrownsBaneGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACrownsBaneGameMode();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Systems")
	TSubclassOf<AWantedLevelManager> WantedLevelManagerClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Systems")
	TSubclassOf<AWindSystem> WindSystemClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Systems")
	TSubclassOf<AEnemySpawner> EnemySpawnerClass;

	UPROPERTY(BlueprintReadOnly, Category = "Game Systems")
	AWantedLevelManager* WantedLevelManager;

	UPROPERTY(BlueprintReadOnly, Category = "Game Systems")
	AWindSystem* WindSystem;

	UPROPERTY(BlueprintReadOnly, Category = "Game Systems")
	AEnemySpawner* EnemySpawner;

	UFUNCTION(BlueprintCallable, Category = "Game Systems")
	AWantedLevelManager* GetWantedLevelManager() const { return WantedLevelManager; }

	UFUNCTION(BlueprintCallable, Category = "Game Systems")
	AWindSystem* GetWindSystem() const { return WindSystem; }

	UFUNCTION(BlueprintCallable, Category = "Game Systems")
	AEnemySpawner* GetEnemySpawner() const { return EnemySpawner; }
};
