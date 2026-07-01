// Copyright 2024 Crown's Bane. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Quests/MissionTypes.h"
#include "MissionManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMissionCompleted, FGuid, MissionId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMissionFailed,    FGuid, MissionId);

/**
 * Tracks generic missions with type/time/target/reward.  Player picks up
 * new missions from a MissionGiver actor or via BP; MissionManager runs
 * their timers and completion checks each Tick.
 *
 *   Race     — reach Target before TimeRemaining runs out
 *   Escort   — keep protected NPC (Target actor location tracked externally) alive to arrive
 *   Blockade — reach Target while not taking > BlockadeMaxDamage
 *   Siege    — reduce Target's HP to 0 (target = fort actor)
 *   Rescue   — reach prison ship then it becomes ally for 5 min
 *   StoryMain — narrative mission (banner-driven)
 */
UCLASS()
class CROWNSBANE_API AMissionManager : public AActor
{
	GENERATED_BODY()

public:
	AMissionManager();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Missions")
	FGuid StartMission(EMissionType Type, const FString& Title, const FString& Description,
		FVector Target, float TimeLimit, int32 RewardGold, int32 GoalProgress = 1);

	UFUNCTION(BlueprintCallable, Category = "Missions")
	bool CompleteMission(FGuid Id);

	UFUNCTION(BlueprintCallable, Category = "Missions")
	bool FailMission(FGuid Id);

	// Progress bump for scored missions (siege HP damage, escort survival ticks).
	UFUNCTION(BlueprintCallable, Category = "Missions")
	void AddProgress(FGuid Id, int32 Delta);

	UFUNCTION(BlueprintPure, Category = "Missions")
	const TArray<FMissionData>& GetActive() const { return Active; }

	UPROPERTY(BlueprintAssignable, Category = "Missions")
	FOnMissionCompleted OnMissionCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Missions")
	FOnMissionFailed OnMissionFailed;

	// Reach radius for waypoint-style missions (Race, Escort).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Missions")
	float WaypointReachRadius = 800.0f;

private:
	UPROPERTY() TArray<FMissionData> Active;
};
