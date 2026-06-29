#pragma once

#include "CoreMinimal.h"
#include "ProjectileTypes.generated.h"

UENUM(BlueprintType)
enum class ECannonballType : uint8
{
	Standard    UMETA(DisplayName = "Round Shot"),
	Chain       UMETA(DisplayName = "Chain Shot"),
	Grape       UMETA(DisplayName = "Grape Shot"),
	Heavy       UMETA(DisplayName = "Heavy Shot"),
	Explosive   UMETA(DisplayName = "Explosive Shell")
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

	// Splash radius for explosive shell (cm). 0 = direct hit only.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannonball")
	float SplashRadius = 0.0f;

	// Grape Shot: shots fired per cannon (1 = normal). Higher = shotgun spread.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannonball")
	int32 PelletsPerShot = 1;

	// Extra random yaw spread applied per pellet (degrees).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannonball")
	float ExtraSpreadAngle = 0.0f;

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

	static FCannonballData MakeGrape()
	{
		FCannonballData Data;
		Data.Type = ECannonballType::Grape;
		Data.BaseDamage = 6.0f;          // each pellet hits softly...
		Data.InitialSpeed = 2800.0f;
		Data.MaxSpeed = 2800.0f;
		Data.GravityScale = 1.5f;
		Data.PelletsPerShot = 5;          // ...but five pellets per cannon = murder up close
		Data.ExtraSpreadAngle = 14.0f;
		return Data;
	}

	static FCannonballData MakeHeavy()
	{
		FCannonballData Data;
		Data.Type = ECannonballType::Heavy;
		Data.BaseDamage = 55.0f;          // hurts a lot
		Data.InitialSpeed = 2200.0f;      // slower
		Data.MaxSpeed = 2200.0f;
		Data.GravityScale = 1.5f;         // arcs more
		return Data;
	}

	static FCannonballData MakeExplosive()
	{
		FCannonballData Data;
		Data.Type = ECannonballType::Explosive;
		Data.BaseDamage = 35.0f;
		Data.InitialSpeed = 2400.0f;
		Data.MaxSpeed = 2400.0f;
		Data.GravityScale = 1.3f;
		Data.SplashRadius = 600.0f;       // 6m AoE around impact
		return Data;
	}
};
