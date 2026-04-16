#pragma once

#include "CoreMinimal.h"
#include "ResourceTypes.generated.h"

UENUM(BlueprintType)
enum class EResourceType : uint8
{
	Gold    UMETA(DisplayName = "Gold"),
	Wood    UMETA(DisplayName = "Wood"),
	Metal   UMETA(DisplayName = "Metal")
};

USTRUCT(BlueprintType)
struct FResourceDrop
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	EResourceType ResourceType = EResourceType::Gold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	int32 Amount = 50;

	FResourceDrop()
		: ResourceType(EResourceType::Gold)
		, Amount(50)
	{}

	FResourceDrop(EResourceType InType, int32 InAmount)
		: ResourceType(InType)
		, Amount(InAmount)
	{}
};
