#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Upgrades/UpgradeTypes.h"
#include "CrownsBaneSaveGame.generated.h"

USTRUCT(BlueprintType)
struct FSavedResources
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite) int32 Gold  = 0;
	UPROPERTY(BlueprintReadWrite) int32 Wood  = 0;
	UPROPERTY(BlueprintReadWrite) int32 Metal = 0;
};

USTRUCT(BlueprintType)
struct FSavedUpgrades
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite) int32 HullLevel        = 0;
	UPROPERTY(BlueprintReadWrite) int32 SailsLevel       = 0;
	UPROPERTY(BlueprintReadWrite) int32 WeaponsLevel     = 0;
	UPROPERTY(BlueprintReadWrite) int32 CannonCountLevel = 0;
};

UCLASS()
class CROWNSBANE_API UCrownsBaneSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static const FString SlotName;
	static const int32   UserIndex;

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FSavedResources Resources;

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FSavedUpgrades Upgrades;

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	int32 TotalShipsSunk = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	float TotalGoldEarned = 0.0f;
};
