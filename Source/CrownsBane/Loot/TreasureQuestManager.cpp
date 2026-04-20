// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "Loot/TreasureQuestManager.h"
#include "Loot/TreasureChest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

ATreasureQuestManager::ATreasureQuestManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

FGuid ATreasureQuestManager::IssueQuest(const FVector& Origin)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return FGuid();
	}

	if (ActiveQuests.Num() >= MaxActiveQuests)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TreasureQuestManager] Max active quests reached (%d)"), MaxActiveQuests);
		return FGuid();
	}

	// Pick random direction and distance.
	const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
	const float Dist = FMath::FRandRange(MinQuestDistance, MaxQuestDistance);
	const FVector ChestLoc(
		Origin.X + FMath::Cos(Angle) * Dist,
		Origin.Y + FMath::Sin(Angle) * Dist,
		SeaLevelZ
	);

	// бхопюбкемн рср:
	UClass* ClassToSpawn = TreasureChestClass ? TreasureChestClass.Get() : ATreasureChest::StaticClass();

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	ATreasureChest* Chest = World->SpawnActor<ATreasureChest>(ClassToSpawn, ChestLoc, FRotator::ZeroRotator, Params);

	if (!Chest)
	{
		UE_LOG(LogTemp, Error, TEXT("[TreasureQuestManager] Failed to spawn TreasureChest."));
		return FGuid();
	}

	FTreasureQuest NewQuest;
	NewQuest.QuestName = FText::FromString(FString::Printf(TEXT("Buried Treasure #%d"), ActiveQuests.Num() + 1));
	NewQuest.ChestLocation = ChestLoc;
	NewQuest.ChestActor = Chest;

	Chest->QuestId = NewQuest.QuestId;

	ActiveQuests.Add(NewQuest);
	OnQuestIssued.Broadcast(NewQuest);

	UE_LOG(LogTemp, Log, TEXT("[TreasureQuestManager] Issued quest %s at %s (%.0f cm away)"),
		*NewQuest.QuestId.ToString(), *ChestLoc.ToString(), Dist);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 6.f, FColor::Yellow,
			FString::Printf(TEXT("New treasure map! Chest marked %.0fm away"), Dist / 100.0f));
	}

	return NewQuest.QuestId;
}

void ATreasureQuestManager::CompleteQuest(FGuid QuestId)
{
	const int32 Removed = ActiveQuests.RemoveAll([QuestId](const FTreasureQuest& Q)
		{
			return Q.QuestId == QuestId;
		});

	if (Removed > 0)
	{
		OnQuestCompleted.Broadcast(QuestId);
		UE_LOG(LogTemp, Log, TEXT("[TreasureQuestManager] Quest %s completed."), *QuestId.ToString());
	}
}

bool ATreasureQuestManager::GetNearestActiveQuestLocation(const FVector& From, FVector& OutLocation, float& OutDistance) const
{
	float BestDistSq = TNumericLimits<float>::Max();
	bool bFound = false;

	for (const FTreasureQuest& Q : ActiveQuests)
	{
		if (!Q.ChestActor.IsValid())
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(From, Q.ChestLocation);
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			OutLocation = Q.ChestLocation;
			bFound = true;
		}
	}

	if (bFound)
	{
		OutDistance = FMath::Sqrt(BestDistSq);
	}
	return bFound;
}