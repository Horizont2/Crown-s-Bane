#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/ProjectileTypes.h"
#include "CannonComponent.generated.h"

class ACannonball;

UENUM(BlueprintType)
enum class ECannonSide : uint8
{
	Left    UMETA(DisplayName = "Port (Left)"),
	Right   UMETA(DisplayName = "Starboard (Right)")
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CROWNSBANE_API UCannonComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCannonComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Fire the broadside on the given side
	UFUNCTION(BlueprintCallable, Category = "Cannon")
	void FireBroadside(ECannonSide Side);

	// Upgrade cannon count (adds cannons per side)
	UFUNCTION(BlueprintCallable, Category = "Cannon")
	void UpgradeCannonCount(int32 NewCountPerSide);

	UFUNCTION(BlueprintPure, Category = "Cannon")
	bool CanFire(ECannonSide Side) const;

	UFUNCTION(BlueprintPure, Category = "Cannon")
	float GetReloadProgress(ECannonSide Side) const;

	UFUNCTION(BlueprintPure, Category = "Cannon")
	int32 GetCannonsPerSide() const { return CannonsPerSide; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannon")
	int32 CannonsPerSide = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannon")
	float ReloadTime = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannon")
	float DamagePerCannon = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannon")
	ECannonballType ActiveCannonballType = ECannonballType::Standard;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannon")
	TSubclassOf<ACannonball> CannonballClass;

	// Socket name prefix for cannon spawn points on the mesh
	// Expects sockets named: CannonLeft_0, CannonLeft_1, etc. / CannonRight_0, etc.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannon")
	FName LeftSocketPrefix = TEXT("CannonLeft_");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannon")
	FName RightSocketPrefix = TEXT("CannonRight_");

	// Elevation angle applied to cannon fire direction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannon")
	float ElevationAngle = 5.0f;

private:
	float LeftReloadTimer = 0.0f;
	float RightReloadTimer = 0.0f;
	bool bLeftReady = true;
	bool bRightReady = true;

	void SpawnCannonball(FVector SpawnLocation, FVector Direction, const FCannonballData& Data);
};
