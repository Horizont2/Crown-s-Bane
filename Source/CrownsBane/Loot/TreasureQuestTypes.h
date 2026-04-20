// Copyright 2024 Crown's Bane. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TreasureQuestTypes.generated.h"

class ATreasureChest;

USTRUCT(BlueprintType)
struct FTreasureQuest
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Treasure")
	FGuid QuestId;

	UPROPERTY(BlueprintReadOnly, Category = "Treasure")
	FText QuestName;

	UPROPERTY(BlueprintReadOnly, Category = "Treasure")
	FVector ChestLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Treasure")
	TWeakObjectPtr<ATreasureChest> ChestActor;

	FTreasureQuest()
		: QuestId(FGuid::NewGuid())
	{}
};
