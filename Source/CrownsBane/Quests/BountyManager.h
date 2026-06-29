// Copyright 2024 Crown's Bane. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BountyManager.generated.h"

class UPlayerInventory;

USTRUCT(BlueprintType)
struct FBountyQuest
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Bounty")
	FGuid QuestId;

	UPROPERTY(BlueprintReadOnly, Category = "Bounty")
	FString Title;

	// Number of enemy ships the player must sink to complete the bounty.
	UPROPERTY(BlueprintReadOnly, Category = "Bounty")
	int32 TargetKills = 3;

	UPROPERTY(BlueprintReadOnly, Category = "Bounty")
	int32 CurrentKills = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Bounty")
	int32 RewardGold = 300;

	FBountyQuest() : QuestId(FGuid::NewGuid()) {}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBountyIssued, const FBountyQuest&, NewBounty);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBountyCompleted, FGuid, QuestId);

/**
 * Tracks active "kill N enemies" bounties.  Players gain a configurable
 * gold reward when each bounty is completed.  The HUD reads
 * GetActiveBounties() and renders them in the same quest log overlay
 * as treasure hunts.  EnemyShipBase calls NotifyEnemyKilled on death.
 */
UCLASS()
class CROWNSBANE_API ABountyManager : public AActor
{
	GENERATED_BODY()

public:
	ABountyManager();

protected:
	virtual void BeginPlay() override;

public:
	// Add a new bounty with custom params; returns its QuestId.
	UFUNCTION(BlueprintCallable, Category = "Bounty")
	FGuid IssueBounty(const FString& Title, int32 TargetKills, int32 RewardGold);

	// Convenience: spawn a "Hunt N enemies for G gold" bounty using ints from BP.
	UFUNCTION(BlueprintCallable, Category = "Bounty")
	FGuid IssueDefaultBounty();

	UFUNCTION(BlueprintCallable, Category = "Bounty")
	void NotifyEnemyKilled();

	UFUNCTION(BlueprintPure, Category = "Bounty")
	const TArray<FBountyQuest>& GetActiveBounties() const { return ActiveBounties; }

	UFUNCTION(BlueprintPure, Category = "Bounty")
	int32 GetActiveCount() const { return ActiveBounties.Num(); }

	// Maximum concurrent active bounties.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty", meta=(ClampMin="1"))
	int32 MaxActiveBounties = 3;

	// Auto-issue a starter bounty on BeginPlay.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty")
	bool bAutoIssueStarter = true;

	// Reward range for IssueDefaultBounty.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty")
	int32 DefaultMinKills = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty")
	int32 DefaultMaxKills = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty")
	int32 DefaultRewardPerKill = 120;

	UPROPERTY(BlueprintAssignable, Category = "Bounty")
	FOnBountyIssued OnBountyIssued;

	UPROPERTY(BlueprintAssignable, Category = "Bounty")
	FOnBountyCompleted OnBountyCompleted;

private:
	UPROPERTY() TArray<FBountyQuest> ActiveBounties;

	UPlayerInventory* GetPlayerInventory() const;
};
