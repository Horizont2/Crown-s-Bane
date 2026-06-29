// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "Player/ShipProgressionComponent.h"

UShipProgressionComponent::UShipProgressionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

int32 UShipProgressionComponent::GetXPForNextLevel() const
{
	return Level * Level * 100;
}

void UShipProgressionComponent::AwardXP(int32 Amount)
{
	if (Amount <= 0) return;
	XP += Amount;

	while (XP >= GetXPForNextLevel())
	{
		XP -= GetXPForNextLevel();
		++Level;
		OnShipLevelUp.Broadcast(Level, XP);
		// Perk choice granted every 5 levels.
		if (Level % 5 == 0)
		{
			bPerkChoicePending = true;
		}
	}
}

bool UShipProgressionComponent::ChoosePerk(EShipPerk Perk)
{
	if (!bPerkChoicePending) return false;
	if (UnlockedPerks.Contains(Perk)) return false;
	UnlockedPerks.Add(Perk);
	bPerkChoicePending = false;
	OnPerkUnlocked.Broadcast(Perk);
	return true;
}
