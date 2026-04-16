#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Player/CrownsBaneSaveGame.h"
#include "CrownsBaneGameInstance.generated.h"

UCLASS()
class CROWNSBANE_API UCrownsBaneGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// Call on game start and after any change to write progress to disk
	UFUNCTION(BlueprintCallable, Category = "Save")
	void SaveGame();

	// Call on game start to restore progress
	UFUNCTION(BlueprintCallable, Category = "Save")
	UCrownsBaneSaveGame* LoadGame();

	// Delete save (New Game)
	UFUNCTION(BlueprintCallable, Category = "Save")
	void DeleteSave();

	// Active save data — always up-to-date during a session
	UPROPERTY(BlueprintReadOnly, Category = "Save")
	UCrownsBaneSaveGame* CurrentSave = nullptr;

	// Called by loot / upgrade systems to mutate and auto-save
	UFUNCTION(BlueprintCallable, Category = "Save")
	void AddResources(int32 Gold, int32 Wood, int32 Metal);

	UFUNCTION(BlueprintCallable, Category = "Save")
	bool SpendResources(int32 Gold, int32 Wood, int32 Metal);

	UFUNCTION(BlueprintCallable, Category = "Save")
	void RecordShipSunk();

	UFUNCTION(BlueprintCallable, Category = "Save")
	void SetUpgradeLevel(EUpgradeCategory Category, int32 Level);

	UFUNCTION(BlueprintPure, Category = "Save")
	int32 GetUpgradeLevel(EUpgradeCategory Category) const;

protected:
	virtual void Init() override;

private:
	void EnsureSaveLoaded();
};
