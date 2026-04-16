#pragma once

#include "CoreMinimal.h"
#include "ProjectileTypes.generated.h"

UENUM(BlueprintType)
enum class ECannonballType : uint8
{
	Standard    UMETA(DisplayName = "Standard Cannonball"),
	Chain       UMETA(DisplayName = "Chain Shot")
};

USTRUCT(BlueprintType)
struct FCannonballData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannonball")
	ECannonballType Type = ECannonballType::Standard;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannonball")
	float BaseDamage = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannonball")
	float InitialSpeed = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannonball")
	float MaxSpeed = 3000.0f;

	// Chain Shot: slows the hit ship by this fraction (0.5 = 50% speed reduction)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannonball")
	float SlowFraction = 0.5f;

	// Chain Shot slow duration in seconds
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannonball")
	float SlowDuration = 5.0f;

	// Gravity scale for projectile
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannonball")
	float GravityScale = 1.0f;

	FCannonballData()
	{
		Type = ECannonballType::Standard;
		BaseDamage = 25.0f;
		InitialSpeed = 3000.0f;
		MaxSpeed = 3000.0f;
		SlowFraction = 0.5f;
		SlowDuration = 5.0f;
		GravityScale = 1.0f;
	}

	static FCannonballData MakeStandard()
	{
		FCannonballData Data;
		Data.Type = ECannonballType::Standard;
		Data.BaseDamage = 25.0f;
		Data.InitialSpeed = 3000.0f;
		Data.MaxSpeed = 3000.0f;
		Data.GravityScale = 1.0f;
		return Data;
	}

	static FCannonballData MakeChain()
	{
		FCannonballData Data;
		Data.Type = ECannonballType::Chain;
		Data.BaseDamage = 10.0f;
		Data.InitialSpeed = 2500.0f;
		Data.MaxSpeed = 2500.0f;
		Data.SlowFraction = 0.5f;
		Data.SlowDuration = 5.0f;
		Data.GravityScale = 1.2f;
		return Data;
	}
};
