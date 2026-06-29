// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "Quests/BountyManager.h"
#include "Player/PlayerInventory.h"
#include "Loot/ResourceTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

ABountyManager::ABountyManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ABountyManager::BeginPlay()
{
	Super::BeginPlay();
	if (bAutoIssueStarter)
	{
		IssueDefaultBounty();
	}
}

FGuid ABountyManager::IssueBounty(const FString& Title, int32 TargetKills, int32 RewardGold)
{
	if (ActiveBounties.Num() >= MaxActiveBounties) return FGuid();

	FBountyQuest Q;
	Q.Title       = Title;
	Q.TargetKills = FMath::Max(1, TargetKills);
	Q.RewardGold  = FMath::Max(0, RewardGold);
	ActiveBounties.Add(Q);
	OnBountyIssued.Broadcast(Q);
	return Q.QuestId;
}

FGuid ABountyManager::IssueDefaultBounty()
{
	static const TCHAR* TitlePool[] = {
		TEXT("Privateer Contract"),
		TEXT("Crown's Letter of Marque"),
		TEXT("Tavern Whisper: Black Sails"),
		TEXT("Bounty: Coastal Raiders"),
		TEXT("Bounty: Convoy Hunters"),
	};
	const int32 PoolSize = sizeof(TitlePool) / sizeof(TitlePool[0]);
	const FString Title = TitlePool[FMath::RandRange(0, PoolSize - 1)];
	const int32 Kills = FMath::RandRange(FMath::Max(1, DefaultMinKills), FMath::Max(DefaultMinKills, DefaultMaxKills));
	return IssueBounty(Title, Kills, Kills * DefaultRewardPerKill);
}

void ABountyManager::NotifyEnemyKilled()
{
	bool bAnyCompleted = false;
	for (int32 i = ActiveBounties.Num() - 1; i >= 0; --i)
	{
		FBountyQuest& Q = ActiveBounties[i];
		Q.CurrentKills++;
		if (Q.CurrentKills >= Q.TargetKills)
		{
			if (UPlayerInventory* Inv = GetPlayerInventory())
			{
				Inv->AddResource(EResourceType::Gold, Q.RewardGold);
			}
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Yellow,
					FString::Printf(TEXT("BOUNTY COMPLETE: %s — +%d gold"), *Q.Title, Q.RewardGold));
			}
			const FGuid Id = Q.QuestId;
			ActiveBounties.RemoveAt(i);
			OnBountyCompleted.Broadcast(Id);
			bAnyCompleted = true;
		}
	}

	// After a completion, immediately issue a fresh bounty so players always
	// have something to chase.
	if (bAnyCompleted && ActiveBounties.Num() < MaxActiveBounties)
	{
		IssueDefaultBounty();
	}
}

UPlayerInventory* ABountyManager::GetPlayerInventory() const
{
	if (UWorld* W = GetWorld())
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(W, 0))
		{
			if (UPlayerInventory* Inv = PC->FindComponentByClass<UPlayerInventory>()) return Inv;
			if (APawn* P = PC->GetPawn())
			{
				if (UPlayerInventory* Inv = P->FindComponentByClass<UPlayerInventory>()) return Inv;
			}
		}
	}
	return nullptr;
}
