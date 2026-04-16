#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Loot/ResourceTypes.h"
#include "PlayerInventory.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnResourceChanged, EResourceType, ResourceType, int32, NewAmount);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CROWNSBANE_API UPlayerInventory : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerInventory();

protected:
	virtual void BeginPlay() override;

public:
	// Add resource to inventory
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AddResource(EResourceType ResourceType, int32 Amount);

	// Spend resource - returns true if successful
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool SpendResource(EResourceType ResourceType, int32 Amount);

	// Check if player has enough of a resource
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool HasResource(EResourceType ResourceType, int32 Amount) const;

	// Get current amount of a resource
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetResourceAmount(EResourceType ResourceType) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetGold() const { return Gold; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetWood() const { return Wood; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetMetal() const { return Metal; }

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnResourceChanged OnResourceChanged;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 Gold = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 Wood = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 Metal = 0;
};
