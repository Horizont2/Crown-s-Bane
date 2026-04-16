// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "Player/CrownsBaneGameInstance.h"
#include "Kismet/GameplayStatics.h"

void UCrownsBaneGameInstance::Init()
{
	Super::Init();
	EnsureSaveLoaded();
}

void UCrownsBaneGameInstance::EnsureSaveLoaded()
{
	if (!CurrentSave)
	{
		CurrentSave = Cast<UCrownsBaneSaveGame>(
			UGameplayStatics::LoadGameFromSlot(UCrownsBaneSaveGame::SlotName, UCrownsBaneSaveGame::UserIndex));

		if (!CurrentSave)
		{
			CurrentSave = Cast<UCrownsBaneSaveGame>(
				UGameplayStatics::CreateSaveGameObject(UCrownsBaneSaveGame::StaticClass()));
			UE_LOG(LogTemp, Log, TEXT("GameInstance: No save found, created fresh save."));
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("GameInstance: Save loaded. Gold=%d Wood=%d Metal=%d"),
				CurrentSave->Resources.Gold,
				CurrentSave->Resources.Wood,
				CurrentSave->Resources.Metal);
		}
	}
}

void UCrownsBaneGameInstance::SaveGame()
{
	if (!CurrentSave) return;
	UGameplayStatics::SaveGameToSlot(CurrentSave, UCrownsBaneSaveGame::SlotName, UCrownsBaneSaveGame::UserIndex);
	UE_LOG(LogTemp, Log, TEXT("GameInstance: Progress saved."));
}

UCrownsBaneSaveGame* UCrownsBaneGameInstance::LoadGame()
{
	EnsureSaveLoaded();
	return CurrentSave;
}

void UCrownsBaneGameInstance::DeleteSave()
{
	UGameplayStatics::DeleteGameInSlot(UCrownsBaneSaveGame::SlotName, UCrownsBaneSaveGame::UserIndex);
	CurrentSave = Cast<UCrownsBaneSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UCrownsBaneSaveGame::StaticClass()));
	UE_LOG(LogTemp, Log, TEXT("GameInstance: Save deleted, fresh start."));
}

void UCrownsBaneGameInstance::AddResources(int32 Gold, int32 Wood, int32 Metal)
{
	if (!CurrentSave) return;
	CurrentSave->Resources.Gold  += Gold;
	CurrentSave->Resources.Wood  += Wood;
	CurrentSave->Resources.Metal += Metal;
	CurrentSave->TotalGoldEarned += Gold;
	SaveGame();
}

bool UCrownsBaneGameInstance::SpendResources(int32 Gold, int32 Wood, int32 Metal)
{
	if (!CurrentSave) return false;

	if (CurrentSave->Resources.Gold  < Gold  ||
		CurrentSave->Resources.Wood  < Wood  ||
		CurrentSave->Resources.Metal < Metal)
	{
		return false;
	}

	CurrentSave->Resources.Gold  -= Gold;
	CurrentSave->Resources.Wood  -= Wood;
	CurrentSave->Resources.Metal -= Metal;
	SaveGame();
	return true;
}

void UCrownsBaneGameInstance::RecordShipSunk()
{
	if (!CurrentSave) return;
	CurrentSave->TotalShipsSunk++;
	SaveGame();
}

void UCrownsBaneGameInstance::SetUpgradeLevel(EUpgradeCategory Category, int32 Level)
{
	if (!CurrentSave) return;
	switch (Category)
	{
	case EUpgradeCategory::Hull:        CurrentSave->Upgrades.HullLevel        = Level; break;
	case EUpgradeCategory::Sails:       CurrentSave->Upgrades.SailsLevel       = Level; break;
	case EUpgradeCategory::Weapons:     CurrentSave->Upgrades.WeaponsLevel     = Level; break;
	case EUpgradeCategory::CannonCount: CurrentSave->Upgrades.CannonCountLevel = Level; break;
	}
	SaveGame();
}

int32 UCrownsBaneGameInstance::GetUpgradeLevel(EUpgradeCategory Category) const
{
	if (!CurrentSave) return 0;
	switch (Category)
	{
	case EUpgradeCategory::Hull:        return CurrentSave->Upgrades.HullLevel;
	case EUpgradeCategory::Sails:       return CurrentSave->Upgrades.SailsLevel;
	case EUpgradeCategory::Weapons:     return CurrentSave->Upgrades.WeaponsLevel;
	case EUpgradeCategory::CannonCount: return CurrentSave->Upgrades.CannonCountLevel;
	}
	return 0;
}
