#pragma once

#include "CoreMinimal.h"
#include "Loot/ResourceTypes.h"
#include "UpgradeTypes.generated.h"

UENUM(BlueprintType)
enum class EUpgradeCategory : uint8
{
	Hull        UMETA(DisplayName = "Hull (Health)"),
	Sails       UMETA(DisplayName = "Sails (Speed)"),
	Weapons     UMETA(DisplayName = "Weapons (Damage)"),
	CannonCount UMETA(DisplayName = "Cannon Count")
};

USTRUCT(BlueprintType)
struct FUpgradeLevel
{
	GENERATED_BODY()

	// Tier level (1-4)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	int32 Tier = 1;

	// Gold cost for this tier
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	int32 GoldCost = 100;

	// Wood cost (may be 0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	int32 WoodCost = 0;

	// Metal cost (may be 0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	int32 MetalCost = 0;

	// Stat bonus granted by this upgrade
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	float StatBonus = 0.0f;

	// Human-readable description
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	FString Description = TEXT("Upgrade");

	FUpgradeLevel()
		: Tier(1), GoldCost(100), WoodCost(0), MetalCost(0), StatBonus(0.0f), Description(TEXT(""))
	{}

	FUpgradeLevel(int32 InTier, int32 InGold, int32 InWood, int32 InMetal, float InBonus, const FString& InDesc)
		: Tier(InTier), GoldCost(InGold), WoodCost(InWood), MetalCost(InMetal), StatBonus(InBonus), Description(InDesc)
	{}
};
