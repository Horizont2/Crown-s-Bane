// Copyright 2024 Crown's Bane. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShipProgressionComponent.generated.h"

UENUM(BlueprintType)
enum class EShipPerk : uint8
{
	None         UMETA(DisplayName = "None"),
	EagleEye     UMETA(DisplayName = "Eagle Eye"),         // lock-on range x2
	IronHull     UMETA(DisplayName = "Iron Hull"),         // +15% MaxHP
	Cutthroat    UMETA(DisplayName = "Cutthroat"),         // boarding loot x3
	StormCaptain UMETA(DisplayName = "Storm Captain"),     // immune to storm wind chaos
	Marksman     UMETA(DisplayName = "Marksman"),          // +10% crit
	ReloadMaster UMETA(DisplayName = "Reload Master"),     // -1s reload
	WindReader   UMETA(DisplayName = "Wind Reader"),       // -50% wind drift on shots
	Buccaneer    UMETA(DisplayName = "Buccaneer")          // +25% all gold gains
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnShipLevelUp, int32, NewLevel, int32, NewXP);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPerkUnlocked, EShipPerk, Perk);

/**
 * Ship XP / level / perk progression — attach to PlayerController.
 * XP curve: Lvl^2 * 100.  Every 5 levels unlocks a perk choice
 * (player selects via UI by pressing 1-8 on the perk panel).
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CROWNSBANE_API UShipProgressionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShipProgressionComponent();

	UPROPERTY(BlueprintReadOnly, Category = "Progression")
	int32 Level = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Progression")
	int32 XP = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Progression")
	TArray<EShipPerk> UnlockedPerks;

	// Pending perk choice — set when a level-up grants one, cleared on selection.
	UPROPERTY(BlueprintReadOnly, Category = "Progression")
	bool bPerkChoicePending = false;

	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetXPForNextLevel() const;

	UFUNCTION(BlueprintCallable, Category = "Progression")
	void AwardXP(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Progression")
	bool ChoosePerk(EShipPerk Perk);

	UFUNCTION(BlueprintPure, Category = "Progression")
	bool HasPerk(EShipPerk Perk) const { return UnlockedPerks.Contains(Perk); }

	UPROPERTY(BlueprintAssignable, Category = "Progression")
	FOnShipLevelUp OnShipLevelUp;

	UPROPERTY(BlueprintAssignable, Category = "Progression")
	FOnPerkUnlocked OnPerkUnlocked;
};
