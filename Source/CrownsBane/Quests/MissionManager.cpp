// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "Quests/MissionManager.h"
#include "Player/CrownsBanePlayerController.h"
#include "Player/PlayerInventory.h"
#include "Loot/ResourceTypes.h"
#include "UI/CrownsBaneHUD.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

AMissionManager::AMissionManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AMissionManager::BeginPlay()
{
	Super::BeginPlay();
}

FGuid AMissionManager::StartMission(EMissionType Type, const FString& Title, const FString& Description,
	FVector Target, float TimeLimit, int32 RewardGold, int32 GoalProgress)
{
	FMissionData M;
	M.Type = Type;
	M.Title = Title;
	M.Description = Description;
	M.Target = Target;
	M.TimeLimit = TimeLimit;
	M.TimeRemaining = TimeLimit;
	M.RewardGold = RewardGold;
	M.GoalProgress = FMath::Max(1, GoalProgress);
	Active.Add(M);

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		if (ACrownsBaneHUD* HUD = Cast<ACrownsBaneHUD>(PC->GetHUD()))
		{
			HUD->ShowBanner(FString::Printf(TEXT("NEW MISSION — %s"), *Title), Description,
				FLinearColor(0.35f, 0.66f, 0.83f, 1.0f), 3.5f);
		}
	}
	return M.MissionId;
}

bool AMissionManager::CompleteMission(FGuid Id)
{
	for (int32 i = 0; i < Active.Num(); ++i)
	{
		if (Active[i].MissionId == Id)
		{
			const FMissionData M = Active[i];
			Active.RemoveAt(i);

			// Award reward via player inventory.
			if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
			{
				if (ACrownsBanePlayerController* CBPC = Cast<ACrownsBanePlayerController>(PC))
				{
					if (CBPC->PlayerInventory) CBPC->PlayerInventory->AddResource(EResourceType::Gold, M.RewardGold);
				}
				if (ACrownsBaneHUD* HUD = Cast<ACrownsBaneHUD>(PC->GetHUD()))
				{
					HUD->ShowMissionComplete(M.Title, M.RewardGold, 0, 0);
				}
			}
			OnMissionCompleted.Broadcast(Id);
			return true;
		}
	}
	return false;
}

bool AMissionManager::FailMission(FGuid Id)
{
	for (int32 i = 0; i < Active.Num(); ++i)
	{
		if (Active[i].MissionId == Id)
		{
			const FMissionData M = Active[i];
			Active.RemoveAt(i);
			if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
			{
				if (ACrownsBaneHUD* HUD = Cast<ACrownsBaneHUD>(PC->GetHUD()))
				{
					HUD->ShowBanner(FString::Printf(TEXT("MISSION FAILED — %s"), *M.Title),
						TEXT("Try again at a mission board."),
						FLinearColor(0.85f, 0.20f, 0.15f, 1.0f), 3.0f);
				}
			}
			OnMissionFailed.Broadcast(Id);
			return true;
		}
	}
	return false;
}

void AMissionManager::AddProgress(FGuid Id, int32 Delta)
{
	for (FMissionData& M : Active)
	{
		if (M.MissionId == Id)
		{
			M.CurrentProgress = FMath::Clamp(M.CurrentProgress + Delta, 0, M.GoalProgress);
			if (M.CurrentProgress >= M.GoalProgress) CompleteMission(Id);
			return;
		}
	}
}

void AMissionManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (Active.Num() == 0) return;

	APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!Player) return;

	// Iterate in reverse — completions can mutate the array.
	for (int32 i = Active.Num() - 1; i >= 0; --i)
	{
		FMissionData& M = Active[i];
		// Timer countdown for timed missions.
		if (M.TimeLimit > 0.0f)
		{
			M.TimeRemaining -= DeltaTime;
			if (M.TimeRemaining <= 0.0f)
			{
				FailMission(M.MissionId);
				continue;
			}
		}
		// Waypoint reached check.
		if (M.Type == EMissionType::Race || M.Type == EMissionType::Escort ||
		    M.Type == EMissionType::Blockade || M.Type == EMissionType::Rescue)
		{
			if (FVector::Dist2D(Player->GetActorLocation(), M.Target) <= WaypointReachRadius)
			{
				CompleteMission(M.MissionId);
			}
		}
	}
}
