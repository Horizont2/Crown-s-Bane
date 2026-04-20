// Copyright 2024 Crown's Bane. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Loot/TreasureQuestTypes.h"
#include "TreasureQuestManager.generated.h"

class ATreasureChest;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTreasureQuestIssued, const FTreasureQuest&, NewQuest);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTreasureQuestCompleted, FGuid, QuestId);

/**
 * Tracks active treasure quests. When a TreasureMapPickup is collected it calls
 * IssueQuest(...) which spawns a TreasureChest at a procedurally chosen sea
 * location and registers the quest so the HUD can draw a compass arrow to it.
 */
UCLASS()
class CROWNSBANE_API ATreasureQuestManager : public AActor
{
	GENERATED_BODY()

public:
	ATreasureQuestManager();

	// Spawn a chest at a random location within [MinQuestDistance, MaxQuestDistance]
	// of the origin and register the quest.
	UFUNCTION(BlueprintCallable, Category = "Treasure")
	FGuid IssueQuest(const FVector& Origin);

	UFUNCTION(BlueprintCallable, Category = "Treasure")
	void CompleteQuest(FGuid QuestId);

	UFUNCTION(BlueprintPure, Category = "Treasure")
	const TArray<FTreasureQuest>& GetActiveQuests() const { return ActiveQuests; }

	// Returns the location of the nearest active quest chest to a world location.
	// Returns false if there are no active quests.
	UFUNCTION(BlueprintCallable, Category = "Treasure")
	bool GetNearestActiveQuestLocation(const FVector& From, FVector& OutLocation, float& OutDistance) const;

	// Blueprint class used when spawning chests. Designers assign BP_TreasureChest here.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treasure")
	TSubclassOf<ATreasureChest> TreasureChestClass;

	// Minimum distance from origin to place the chest.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treasure")
	float MinQuestDistance = 6000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treasure")
	float MaxQuestDistance = 18000.0f;

	// Sea-level Z used when spawning the chest (keeps it floating properly).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treasure")
	float SeaLevelZ = 0.0f;

	// Max concurrent quests
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treasure", meta=(ClampMin="1"))
	int32 MaxActiveQuests = 3;

	UPROPERTY(BlueprintAssignable, Category = "Treasure|Events")
	FOnTreasureQuestIssued OnQuestIssued;

	UPROPERTY(BlueprintAssignable, Category = "Treasure|Events")
	FOnTreasureQuestCompleted OnQuestCompleted;

private:
	UPROPERTY() TArray<FTreasureQuest> ActiveQuests;
};
