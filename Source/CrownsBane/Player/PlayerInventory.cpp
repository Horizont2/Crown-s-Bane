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

	switch (ResourceType)
	{
	case EResourceType::Gold:
		Gold += Amount;
		UE_LOG(LogTemp, Log, TEXT("PlayerInventory: +%d Gold. Total: %d"), Amount, Gold);
		break;
	case EResourceType::Wood:
		Wood += Amount;
		UE_LOG(LogTemp, Log, TEXT("PlayerInventory: +%d Wood. Total: %d"), Amount, Wood);
		break;
	case EResourceType::Metal:
		Metal += Amount;
		UE_LOG(LogTemp, Log, TEXT("PlayerInventory: +%d Metal. Total: %d"), Amount, Metal);
		break;
	}

	OnResourceChanged.Broadcast(ResourceType, GetResourceAmount(ResourceType));
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
	default:                   return 0;
	}
}
