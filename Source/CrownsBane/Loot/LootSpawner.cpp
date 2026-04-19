// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "Loot/LootSpawner.h"
#include "Loot/LootPickup.h"
#include "Engine/World.h"

ALootSpawner::ALootSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ALootSpawner::BeginPlay()
{
	Super::BeginPlay();

	// If spawned without a specific call, default to sloop loot at spawn location
	SpawnLootForShipType(EEnemyShipType::Sloop, GetActorLocation());
	Destroy();
}

void ALootSpawner::SpawnLootForShipType(EEnemyShipType ShipType, FVector Location)
{
	switch (ShipType)
	{
	case EEnemyShipType::Sloop:
		SpawnSloopLoot(Location);
		break;
	case EEnemyShipType::Brig:
		SpawnBrigLoot(Location);
		break;
	case EEnemyShipType::Galleon:
		SpawnGalleonLoot(Location);
		break;
	}
}

ALootPickup* ALootSpawner::SpawnPickup(FVector Location, EResourceType ResourceType, int32 Amount)
{
	if (Amount <= 0) return nullptr;

	UWorld* World = GetWorld();
	if (!World) return nullptr;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	TSubclassOf<ALootPickup> ClassToSpawn = LootPickupClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = ALootPickup::StaticClass();
	}
	ALootPickup* Pickup = World->SpawnActor<ALootPickup>(ClassToSpawn, Location, FRotator::ZeroRotator, SpawnParams);
	if (Pickup)
	{
		Pickup->SetResourceDrop(ResourceType, Amount);
	}
	return Pickup;
}

FVector ALootSpawner::GetRandomLootOffset() const
{
	return FVector(
		FMath::FRandRange(-250.0f, 250.0f),
		FMath::FRandRange(-250.0f, 250.0f),
		0.0f
	);
}

void ALootSpawner::SpawnSloopLoot(FVector Location)
{
	int32 Gold = FMath::RandRange(SloopGoldMin, SloopGoldMax);
	SpawnPickup(Location + GetRandomLootOffset(), EResourceType::Gold, Gold);

	// 60% chance to drop wood
	if (FMath::FRand() < 0.6f)
	{
		int32 Wood = FMath::RandRange(SloopWoodMin, SloopWoodMax);
		SpawnPickup(Location + GetRandomLootOffset(), EResourceType::Wood, Wood);
	}

	UE_LOG(LogTemp, Log, TEXT("LootSpawner: Sloop loot spawned - %d gold."), Gold);
}

void ALootSpawner::SpawnBrigLoot(FVector Location)
{
	int32 Gold = FMath::RandRange(BrigGoldMin, BrigGoldMax);
	SpawnPickup(Location + GetRandomLootOffset(), EResourceType::Gold, Gold);

	int32 Wood = FMath::RandRange(BrigWoodMin, BrigWoodMax);
	SpawnPickup(Location + GetRandomLootOffset(), EResourceType::Wood, Wood);

	// 50% chance to drop metal
	if (FMath::FRand() < 0.5f)
	{
		int32 Metal = FMath::RandRange(BrigMetalMin, BrigMetalMax);
		SpawnPickup(Location + GetRandomLootOffset(), EResourceType::Metal, Metal);
	}

	UE_LOG(LogTemp, Log, TEXT("LootSpawner: Brig loot spawned - %d gold, %d wood."), Gold, Wood);
}

void ALootSpawner::SpawnGalleonLoot(FVector Location)
{
	int32 Gold = FMath::RandRange(GalleonGoldMin, GalleonGoldMax);
	SpawnPickup(Location + GetRandomLootOffset(), EResourceType::Gold, Gold);

	// Multiple gold drops spread around
	int32 BonusGold = FMath::RandRange(GalleonGoldMin / 2, GalleonGoldMax / 2);
	SpawnPickup(Location + GetRandomLootOffset(), EResourceType::Gold, BonusGold);

	int32 Wood = FMath::RandRange(GalleonWoodMin, GalleonWoodMax);
	SpawnPickup(Location + GetRandomLootOffset(), EResourceType::Wood, Wood);

	int32 Metal = FMath::RandRange(GalleonMetalMin, GalleonMetalMax);
	SpawnPickup(Location + GetRandomLootOffset(), EResourceType::Metal, Metal);

	UE_LOG(LogTemp, Log, TEXT("LootSpawner: Galleon loot spawned - %d gold, %d wood, %d metal."),
		Gold + BonusGold, Wood, Metal);
}
