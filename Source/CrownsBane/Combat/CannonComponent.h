// Copyright 2024 Crown's Bane. All Rights Reserved.

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

	UFUNCTION(BlueprintCallable, Category = "Cannon")
	void FireBroadside(ECannonSide Side);

	UFUNCTION(BlueprintCallable, Category = "Cannon")
	void UpgradeCannonCount(int32 NewCountPerSide);

	UFUNCTION(BlueprintPure, Category = "Cannon")
	bool CanFire(ECannonSide Side) const;

	UFUNCTION(BlueprintPure, Category = "Cannon")
	float GetReloadProgress(ECannonSide Side) const;

	UFUNCTION(BlueprintPure, Category = "Cannon")
	int32 GetCannonsPerSide() const { return CannonsPerSide; }

	// ---- Dynamic Aiming (AC4 Style) ----
	UFUNCTION(BlueprintCallable, Category = "Cannon|Aim")
	void UpdateAimTarget(ECannonSide Side, FVector TargetLoc);

	UFUNCTION(BlueprintCallable, Category = "Cannon|Aim")
	void SetIsAiming(bool bAim) { bIsAiming = bAim; }

	UFUNCTION(BlueprintPure, Category = "Cannon|Aim")
	bool IsAiming() const { return bIsAiming; }

	UFUNCTION(BlueprintPure, Category = "Cannon|Aim")
	ECannonSide GetAimingSide() const { return AimingSide; }

	UFUNCTION(BlueprintCallable, Category = "Cannon|Aim")
	void PredictBallisticArc(FVector SpawnLocation, FVector Direction,
		float SeaLevelZ, int32 MaxSteps, float StepSeconds,
		TArray<FVector>& OutPoints, FVector& OutImpactPoint) const;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannon")
	FName LeftSocketPrefix = TEXT("CannonLeft_");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannon")
	FName RightSocketPrefix = TEXT("CannonRight_");

	// ---- Fallback spawn positions (used when the mesh has no sockets) ----
	// If 0, auto-computed from mesh bounds at BeginPlay.  Override in BP if you
	// know exact numbers for your ship model.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannon|Fallback")
	float FallbackShipHalfWidth = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannon|Fallback")
	float FallbackCannonSpacing = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannon|Fallback")
	float FallbackCannonHeight = 50.0f;

	// Called once at BeginPlay to auto-compute FallbackShipHalfWidth /
	// FallbackCannonSpacing from the owner mesh's Bounds if they were left at 0.
	void AutoComputeFallbackFromMeshBounds();

	// Default Elevation (used when NOT aiming)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannon")
	float ElevationAngle = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannon", meta = (ClampMin = "0.0", ClampMax = "15.0"))
	float CannonSpreadAngle = 4.0f;

	// Maximum horizontal turning angle for cannons
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannon|Aim")
	float MaxAzimuthAngle = 40.0f;

	// Max distance the cannons can aim dynamically
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannon|Aim")
	float MaxRange = 9000.0f;

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

	// Dynamic Aim State
	bool bIsAiming = false;
	ECannonSide AimingSide = ECannonSide::Right;
	float DynamicElevationAngle = 5.0f;
	FVector DynamicAzimuthDir = FVector::ForwardVector;

	void SpawnCannonball(FVector SpawnLocation, FVector Direction, const FCannonballData& Data);
	void PlayFireFX(const FVector& Location, const FRotator& Rotation);
};