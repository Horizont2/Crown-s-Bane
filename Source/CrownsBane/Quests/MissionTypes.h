// Copyright 2024 Crown's Bane. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MissionTypes.generated.h"

UENUM(BlueprintType)
enum class EMissionType : uint8
{
	Race          UMETA(DisplayName = "Race"),
	Escort        UMETA(DisplayName = "Escort"),
	Blockade      UMETA(DisplayName = "Blockade Run"),
	Siege         UMETA(DisplayName = "Siege"),
	Rescue        UMETA(DisplayName = "Rescue"),
	StoryMain     UMETA(DisplayName = "Main Quest")
};

USTRUCT(BlueprintType)
struct FMissionData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	FGuid MissionId;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	EMissionType Type = EMissionType::Race;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	FString Title;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	FString Description;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	FVector Target = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	float TimeLimit = 0.0f;   // 0 = untimed

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	float TimeRemaining = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	int32 RewardGold = 200;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	int32 CurrentProgress = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	int32 GoalProgress = 1;

	FMissionData() : MissionId(FGuid::NewGuid()) {}
};
