#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/ProjectileTypes.h"
#include "CannonComponent.generated.h"

class ACannonball;
class UNiagaraSystem;
class USoundBase;
class UCameraShakeBase;

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

	// Predict the trajectory of a single cannonball fired from SpawnLocation
	// along Direction using the current projectile physics.  Populates
	// OutPoints with world-space positions sampled at fixed time steps,
	// stopping when the ball passes Z = SeaLevelZ (ocean surface) or after
	// MaxSteps.  Used by the HUD to draw AC4-style aim-prediction arcs.
	UFUNCTION(BlueprintCallable, Category = "Cannon|Aim")
	void PredictBallisticArc(FVector SpawnLocation, FVector Direction,
		float SeaLevelZ, int32 MaxSteps, float StepSeconds,
		TArray<FVector>& OutPoints, FVector& OutImpactPoint) const;

	// Gather predicted impact points for every cannon on the given side, taking
	// socket positions (if available) and elevation into account.  Returns all
	// sampled trajectories in OutArcs (one entry per cannon).
	UFUNCTION(BlueprintCallable, Category = "Cannon|Aim")
	void GetAimPrediction(ECannonSide Side, float SeaLevelZ,
		TArray<FVector>& OutImpactPoints,
		TArray<FVector>& OutTrajectoryStart,
		TArray<FVector>& OutTrajectoryEnd) const;

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

	// Random horizontal spread per cannon — each ball deviates ±half this value
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannon", meta=(ClampMin="0.0", ClampMax="15.0"))
	float CannonSpreadAngle = 4.0f;

	// ---- FX ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannon|FX")
	UNiagaraSystem* MuzzleFlashFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannon|FX")
	USoundBase* FireSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannon|FX")
	TSubclassOf<UCameraShakeBase> FireCameraShake;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannon|FX")
	float FireCameraShakeScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannon|FX")
	float FireSoundVolume = 1.0f;

private:
	float LeftReloadTimer = 0.0f;
	float RightReloadTimer = 0.0f;
	bool bLeftReady = true;
	bool bRightReady = true;

	void SpawnCannonball(FVector SpawnLocation, FVector Direction, const FCannonballData& Data);
	void PlayFireFX(const FVector& Location, const FRotator& Rotation);
};
