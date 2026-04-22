// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "Player/PlayerInventory.h"

UPlayerInventory::UPlayerInventory()
{
	PrimaryComponentTick.bCanEverTick = false;

	Gold = 0;
	Wood = 0;
	Metal = 0;
}

void UPlayerInventory::BeginPlay()
{
	Super::BeginPlay();
}

void UPlayerInventory::AddResource(EResourceType ResourceType, int32 Amount)
{
	if (Amount <= 0) return;

	// Clamp to cargo cap (0 = unlimited)
	auto Clamp = [this](int32 Current, int32 Add) -> int32
	{
		if (CargoCap <= 0) return Current + Add;
		return FMath::Min(Current + Add, CargoCap);
	};

	switch (ResourceType)
	{
	case EResourceType::Gold:
		Gold = Clamp(Gold, Amount);
		UE_LOG(LogTemp, Log, TEXT("PlayerInventory: +%d Gold. Total: %d"), Amount, Gold);
		break;
	case EResourceType::Wood:
		Wood = Clamp(Wood, Amount);
		UE_LOG(LogTemp, Log, TEXT("PlayerInventory: +%d Wood. Total: %d"), Amount, Wood);
		break;
	case EResourceType::Metal:
		Metal = Clamp(Metal, Amount);
		UE_LOG(LogTemp, Log, TEXT("PlayerInventory: +%d Metal. Total: %d"), Amount, Metal);
		break;
	case EResourceType::Ammo:
		AddAmmo(Amount);
		return; // AddAmmo fires its own broadcast
	}

	OnResourceChanged.Broadcast(ResourceType, GetResourceAmount(ResourceType));
}

void UPlayerInventory::AddAmmo(int32 Amount)
{
	if (Amount <= 0) return;
	CannonAmmo = FMath::Clamp(CannonAmmo + Amount, 0, MaxCannonAmmo);
	OnAmmoChanged.Broadcast(CannonAmmo, MaxCannonAmmo);
	UE_LOG(LogTemp, Log, TEXT("PlayerInventory: +%d ammo. Total: %d/%d"), Amount, CannonAmmo, MaxCannonAmmo);
}

bool UPlayerInventory::ConsumeAmmo(int32 Amount)
{
	if (Amount <= 0) return true;
	if (CannonAmmo < Amount) return false;
	CannonAmmo -= Amount;
	OnAmmoChanged.Broadcast(CannonAmmo, MaxCannonAmmo);
	return true;
}

void UPlayerInventory::UpgradeMaxAmmo(int32 Bonus)
{
	MaxCannonAmmo = FMath::Max(0, MaxCannonAmmo + Bonus);
	CannonAmmo = FMath::Min(CannonAmmo + Bonus, MaxCannonAmmo); // refill a bit as bonus
	OnAmmoChanged.Broadcast(CannonAmmo, MaxCannonAmmo);
}

void UPlayerInventory::UpgradeCargoCap(int32 Bonus)
{
	CargoCap = FMath::Max(0, CargoCap + Bonus);
}

bool UPlayerInventory::SpendResource(EResourceType ResourceType, int32 Amount)
{
	if (Amount <= 0) return true;

	if (!HasResource(ResourceType, Amount))
	{
		UE_LOG(LogTemp, Log, TEXT("PlayerInventory: Cannot spend %d of resource %d - insufficient funds."),
			Amount, (int32)ResourceType);
		return false;
	}

	switch (ResourceType)
	{
	case EResourceType::Gold:
		Gold -= Amount;
		UE_LOG(LogTemp, Log, TEXT("PlayerInventory: Spent %d Gold. Remaining: %d"), Amount, Gold);
		break;
	case EResourceType::Wood:
		Wood -= Amount;
		UE_LOG(LogTemp, Log, TEXT("PlayerInventory: Spent %d Wood. Remaining: %d"), Amount, Wood);
		break;
	case EResourceType::Metal:
		Metal -= Amount;
		UE_LOG(LogTemp, Log, TEXT("PlayerInventory: Spent %d Metal. Remaining: %d"), Amount, Metal);
		break;
	}

	OnResourceChanged.Broadcast(ResourceType, GetResourceAmount(ResourceType));
	return true;
}

bool UPlayerInventory::HasResource(EResourceType ResourceType, int32 Amount) const
{
	return GetResourceAmount(ResourceType) >= Amount;
}

int32 UPlayerInventory::GetResourceAmount(EResourceType ResourceType) const
{
	switch (ResourceType)
	{
	case EResourceType::Gold:  return Gold;
	case EResourceType::Wood:  return Wood;
	case EResourceType::Metal: return Metal;
	case EResourceType::Ammo:  return CannonAmmo;
	default:                   return 0;
	}
}
