// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "Upgrades/UpgradeManager.h"
#include "Player/PlayerInventory.h"
#include "Ship/ShipPawn.h"
#include "Combat/CannonComponent.h"
#include "Components/HealthComponent.h"

AUpgradeManager::AUpgradeManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AUpgradeManager::BeginPlay()
{
	Super::BeginPlay();

	// Initialize all tiers to 0
	UpgradeTiers.Add(EUpgradeCategory::Hull, 0);
	UpgradeTiers.Add(EUpgradeCategory::Sails, 0);
	UpgradeTiers.Add(EUpgradeCategory::Weapons, 0);
	UpgradeTiers.Add(EUpgradeCategory::CannonCount, 0);

	InitUpgradeTables();

	UE_LOG(LogTemp, Log, TEXT("UpgradeManager: Initialized with %d categories, %d tiers each."),
		UpgradeTiers.Num(), MaxTier);
}

void AUpgradeManager::InitUpgradeTables()
{
	// Hull upgrades: increase MaxHealth
	// Tier 1: +50 HP | Tier 2: +75 HP | Tier 3: +100 HP | Tier 4: +150 HP
	TArray<FUpgradeLevel> HullLevels;
	HullLevels.Add(FUpgradeLevel(1, 100,  10,  0,  50.0f, TEXT("Reinforced Planks: +50 Hull HP")));
	HullLevels.Add(FUpgradeLevel(2, 250,  25,  10, 75.0f, TEXT("Iron Fittings: +75 Hull HP")));
	HullLevels.Add(FUpgradeLevel(3, 500,  50,  25, 100.0f, TEXT("Copper Sheathing: +100 Hull HP")));
	HullLevels.Add(FUpgradeLevel(4, 900,  80,  50, 150.0f, TEXT("Indestructible Keel: +150 Hull HP")));
	UpgradeTables.Add(EUpgradeCategory::Hull, HullLevels);

	// Sails upgrades: increase MaxSpeed
	// Tier 1: +150 speed | Tier 2: +200 | Tier 3: +250 | Tier 4: +300
	TArray<FUpgradeLevel> SailLevels;
	SailLevels.Add(FUpgradeLevel(1, 120,  20,  0,  150.0f, TEXT("Canvas Sails: +150 speed")));
	SailLevels.Add(FUpgradeLevel(2, 280,  40,  0,  200.0f, TEXT("Silk Sails: +200 speed")));
	SailLevels.Add(FUpgradeLevel(3, 550,  70,  15, 250.0f, TEXT("Lateen Rig: +250 speed")));
	SailLevels.Add(FUpgradeLevel(4, 1000, 100, 30, 300.0f, TEXT("Ghost Sails: +300 speed")));
	UpgradeTables.Add(EUpgradeCategory::Sails, SailLevels);

	// Weapons upgrades: increase DamagePerCannon
	// Tier 1: +5 dmg | Tier 2: +8 | Tier 3: +12 | Tier 4: +20
	TArray<FUpgradeLevel> WeaponLevels;
	WeaponLevels.Add(FUpgradeLevel(1, 150,  0,  10, 5.0f,  TEXT("Iron Shot: +5 cannon damage")));
	WeaponLevels.Add(FUpgradeLevel(2, 320,  0,  25, 8.0f,  TEXT("Steel Ball: +8 cannon damage")));
	WeaponLevels.Add(FUpgradeLevel(3, 600,  0,  45, 12.0f, TEXT("Explosive Shot: +12 cannon damage")));
	WeaponLevels.Add(FUpgradeLevel(4, 1100, 0,  80, 20.0f, TEXT("Devil's Powder: +20 cannon damage")));
	UpgradeTables.Add(EUpgradeCategory::Weapons, WeaponLevels);

	// CannonCount upgrades: 2->4->6->8 per side
	// Each tier adds 2 cannons
	TArray<FUpgradeLevel> CannonLevels;
	CannonLevels.Add(FUpgradeLevel(1, 200,  30,  20, 4.0f, TEXT("Extra Cannons: 4 per side")));
	CannonLevels.Add(FUpgradeLevel(2, 450,  60,  40, 6.0f, TEXT("Heavy Battery: 6 per side")));
	CannonLevels.Add(FUpgradeLevel(3, 800,  90,  70, 8.0f, TEXT("Full Broadside: 8 per side")));
	CannonLevels.Add(FUpgradeLevel(4, 1400, 120, 100, 8.0f, TEXT("Master Gunner: 8 per side, -20% reload")));
	UpgradeTables.Add(EUpgradeCategory::CannonCount, CannonLevels);
}

int32 AUpgradeManager::GetCurrentTier(EUpgradeCategory Category) const
{
	const int32* Tier = UpgradeTiers.Find(Category);
	return Tier ? *Tier : 0;
}

bool AUpgradeManager::GetNextUpgradeData(EUpgradeCategory Category, FUpgradeLevel& OutData) const
{
	int32 CurrentTier = GetCurrentTier(Category);
	if (CurrentTier >= MaxTier)
	{
		return false; // Already maxed
	}

	const TArray<FUpgradeLevel>* Table = UpgradeTables.Find(Category);
	if (!Table || Table->Num() == 0)
	{
		return false;
	}

	// Next tier to purchase is CurrentTier (0-indexed in array)
	if (CurrentTier >= Table->Num())
	{
		return false;
	}

	OutData = (*Table)[CurrentTier];
	return true;
}

bool AUpgradeManager::CanAffordNextUpgrade(EUpgradeCategory Category, const UPlayerInventory* Inventory) const
{
	if (!Inventory) return false;

	FUpgradeLevel NextLevel;
	if (!GetNextUpgradeData(Category, NextLevel))
	{
		return false;
	}

	return Inventory->HasResource(EResourceType::Gold, NextLevel.GoldCost)
		&& Inventory->HasResource(EResourceType::Wood, NextLevel.WoodCost)
		&& Inventory->HasResource(EResourceType::Metal, NextLevel.MetalCost);
}

bool AUpgradeManager::PurchaseUpgrade(EUpgradeCategory Category, UPlayerInventory* Inventory, AShipPawn* Ship)
{
	if (!Inventory || !Ship)
	{
		UE_LOG(LogTemp, Warning, TEXT("UpgradeManager: PurchaseUpgrade called with null Inventory or Ship!"));
		return false;
	}

	int32 CurrentTier = GetCurrentTier(Category);
	if (CurrentTier >= MaxTier)
	{
		UE_LOG(LogTemp, Log, TEXT("UpgradeManager: Category %d already at max tier."), (int32)Category);
		return false;
	}

	FUpgradeLevel NextLevel;
	if (!GetNextUpgradeData(Category, NextLevel))
	{
		return false;
	}

	// Check and spend resources
	if (!Inventory->HasResource(EResourceType::Gold, NextLevel.GoldCost)
		|| !Inventory->HasResource(EResourceType::Wood, NextLevel.WoodCost)
		|| !Inventory->HasResource(EResourceType::Metal, NextLevel.MetalCost))
	{
		UE_LOG(LogTemp, Log, TEXT("UpgradeManager: Cannot afford upgrade - insufficient resources."));
		return false;
	}

	Inventory->SpendResource(EResourceType::Gold, NextLevel.GoldCost);
	if (NextLevel.WoodCost > 0)  Inventory->SpendResource(EResourceType::Wood, NextLevel.WoodCost);
	if (NextLevel.MetalCost > 0) Inventory->SpendResource(EResourceType::Metal, NextLevel.MetalCost);

	// Apply the upgrade effect
	int32 NewTier = CurrentTier + 1;
	UpgradeTiers[Category] = NewTier;

	ApplyUpgradeEffect(Category, NewTier, Ship);

	UE_LOG(LogTemp, Log, TEXT("UpgradeManager: Purchased %s - Tier %d. %s"),
		*UEnum::GetValueAsString(Category), NewTier, *NextLevel.Description);

	OnUpgradePurchased.Broadcast(Category, NewTier);
	return true;
}

void AUpgradeManager::ApplyUpgradeEffect(EUpgradeCategory Category, int32 Tier, AShipPawn* Ship)
{
	if (!Ship) return;

	const TArray<FUpgradeLevel>* Table = UpgradeTables.Find(Category);
	if (!Table || Tier <= 0 || Tier > Table->Num()) return;

	const FUpgradeLevel& Level = (*Table)[Tier - 1];

	switch (Category)
	{
	case EUpgradeCategory::Hull:
		if (Ship->HealthComponent)
		{
			Ship->HealthComponent->UpgradeMaxHealth(Level.StatBonus);
		}
		break;

	case EUpgradeCategory::Sails:
		Ship->UpgradeMaxSpeed(Level.StatBonus);
		break;

	case EUpgradeCategory::Weapons:
		if (Ship->CannonComponent)
		{
			Ship->CannonComponent->DamagePerCannon += Level.StatBonus;
			UE_LOG(LogTemp, Log, TEXT("UpgradeManager: Cannon damage now %.1f per cannon."),
				Ship->CannonComponent->DamagePerCannon);
		}
		break;

	case EUpgradeCategory::CannonCount:
		if (Ship->CannonComponent)
		{
			int32 NewCount = (int32)Level.StatBonus;
			Ship->CannonComponent->UpgradeCannonCount(NewCount);

			// Tier 4: also reduce reload time by 20%
			if (Tier == 4)
			{
				Ship->CannonComponent->ReloadTime *= 0.8f;
				UE_LOG(LogTemp, Log, TEXT("UpgradeManager: Master Gunner - reload time reduced to %.1f"),
					Ship->CannonComponent->ReloadTime);
			}
		}
		break;
	}
}
