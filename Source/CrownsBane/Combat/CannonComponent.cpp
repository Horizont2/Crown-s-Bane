// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "Combat/CannonComponent.h"
#include "Combat/Cannonball.h"
#include "Combat/ProjectileTypes.h"
#include "Player/PlayerInventory.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/MeshComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Camera/CameraShakeBase.h"

UCannonComponent::UCannonComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCannonComponent::BeginPlay()
{
	Super::BeginPlay();
	LeftReloadTimer = 0.0f;
	RightReloadTimer = 0.0f;
	bLeftReady = true;
	bRightReady = true;
}

void UCannonComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bLeftReady)
	{
		LeftReloadTimer -= DeltaTime;
		if (LeftReloadTimer <= 0.0f) { LeftReloadTimer = 0.0f; bLeftReady = true; }
	}

	if (!bRightReady)
	{
		RightReloadTimer -= DeltaTime;
		if (RightReloadTimer <= 0.0f) { RightReloadTimer = 0.0f; bRightReady = true; }
	}
}

bool UCannonComponent::CanFire(ECannonSide Side) const
{
	return (Side == ECannonSide::Left) ? bLeftReady : bRightReady;
}

float UCannonComponent::GetReloadProgress(ECannonSide Side) const
{
	if (Side == ECannonSide::Left) return bLeftReady ? 1.0f : 1.0f - (LeftReloadTimer / ReloadTime);
	return bRightReady ? 1.0f : 1.0f - (RightReloadTimer / ReloadTime);
}

void UCannonComponent::UpdateAimTarget(ECannonSide Side, FVector TargetLoc)
{
	AimingSide = Side;
	AActor* Owner = GetOwner();
	if (!Owner) return;

	FVector OwnerLoc = Owner->GetActorLocation();
	FVector BaseDir = (Side == ECannonSide::Left) ? -Owner->GetActorRightVector() : Owner->GetActorRightVector();

	FVector ToTarget = TargetLoc - OwnerLoc;
	ToTarget.Z = 0.0f; // Працюємо в 2D площині для Азимута

	float Distance = ToTarget.Size();

	// 1. Розрахунок Азимута (Горизонтальний кут)
	if (Distance < 100.0f)
	{
		DynamicAzimuthDir = BaseDir;
	}
	else
	{
		FVector ToTargetDir = ToTarget.GetSafeNormal();
		// Вираховуємо кут між ідеальним перпендикуляром борту і ціллю
		float AngleRad = FMath::Acos(FMath::Clamp(FVector::DotProduct(BaseDir, ToTargetDir), -1.0f, 1.0f));

		// Обмежуємо поворот гармат (MaxAzimuthAngle)
		if (FMath::RadiansToDegrees(AngleRad) > MaxAzimuthAngle)
		{
			FVector Cross = FVector::CrossProduct(BaseDir, ToTargetDir);
			float Sign = (Cross.Z > 0.0f) ? 1.0f : -1.0f;
			DynamicAzimuthDir = BaseDir.RotateAngleAxis(Sign * MaxAzimuthAngle, FVector::UpVector);
		}
		else
		{
			DynamicAzimuthDir = ToTargetDir;
		}
	}

	// 2. Розрахунок Висоти (Elevation/Pitch)
	Distance = FMath::Clamp(Distance, 0.0f, MaxRange);

	FCannonballData Data = (ActiveCannonballType == ECannonballType::Chain) ? FCannonballData::MakeChain() : FCannonballData::MakeStandard();
	float V = Data.InitialSpeed;
	float g = 980.0f * Data.GravityScale;

	// Зворотня формула балістики: sin(2*theta) = (Distance * g) / V^2
	float Sin2Theta = (Distance * g) / (V * V);
	if (Sin2Theta >= 1.0f)
	{
		DynamicElevationAngle = 45.0f; // Максимальна дальність
	}
	else
	{
		DynamicElevationAngle = FMath::RadiansToDegrees(0.5f * FMath::Asin(Sin2Theta));
	}
}

void UCannonComponent::FireBroadside(ECannonSide Side)
{
	if (!CanFire(Side)) return;

	AActor* Owner = GetOwner();
	if (!Owner) return;

	if (APawn* OwnerPawn = Cast<APawn>(Owner))
	{
		if (AController* Ctrl = OwnerPawn->GetController())
		{
			if (Cast<APlayerController>(Ctrl))
			{
				UPlayerInventory* Inventory = OwnerPawn->FindComponentByClass<UPlayerInventory>();
				if (!Inventory) Inventory = Ctrl->FindComponentByClass<UPlayerInventory>();

				if (Inventory && !Inventory->ConsumeAmmo(CannonsPerSide)) return;
			}
		}
	}

	FCannonballData Data = (ActiveCannonballType == ECannonballType::Chain) ? FCannonballData::MakeChain() : FCannonballData::MakeStandard();
	Data.BaseDamage = DamagePerCannon;

	// Вибираємо динамічний вектор (якщо цілимося), або статичний
	FVector ElevatedDir;
	if (bIsAiming && Side == AimingSide)
	{
		float ElevRad = FMath::DegreesToRadians(DynamicElevationAngle);
		ElevatedDir = DynamicAzimuthDir * FMath::Cos(ElevRad) + FVector::UpVector * FMath::Sin(ElevRad);
	}
	else
	{
		FVector OwnerRight = Owner->GetActorRightVector();
		FVector FireDirection = (Side == ECannonSide::Left) ? -OwnerRight : OwnerRight;
		float ElevRad = FMath::DegreesToRadians(ElevationAngle);
		ElevatedDir = FireDirection * FMath::Cos(ElevRad) + FVector::UpVector * FMath::Sin(ElevRad);
	}
	ElevatedDir.Normalize();

	FName SocketPrefix = (Side == ECannonSide::Left) ? LeftSocketPrefix : RightSocketPrefix;
	UMeshComponent* MeshComp = Owner->FindComponentByClass<UStaticMeshComponent>();
	if (!MeshComp) MeshComp = Owner->FindComponentByClass<USkeletalMeshComponent>();

	bool bFiredFromSocket = false;
	const float HalfSpread = CannonSpreadAngle * 0.5f;

	auto ApplySpread = [&](FVector BaseDir) -> FVector
		{
			float YawJitter = FMath::FRandRange(-HalfSpread, HalfSpread);
			float PitchJitter = FMath::FRandRange(-HalfSpread * 0.3f, HalfSpread * 0.3f);
			FRotator Jitter(PitchJitter, YawJitter, 0.0f);
			return Jitter.RotateVector(BaseDir).GetSafeNormal();
		};

	if (MeshComp)
	{
		for (int32 i = 0; i < CannonsPerSide; ++i)
		{
			FName SocketName = FName(*FString::Printf(TEXT("%s%d"), *SocketPrefix.ToString(), i));
			if (MeshComp->DoesSocketExist(SocketName))
			{
				FVector SpawnLoc = MeshComp->GetSocketLocation(SocketName);
				FVector ShotDir = ApplySpread(ElevatedDir);
				SpawnCannonball(SpawnLoc, ShotDir, Data);
				PlayFireFX(SpawnLoc, ShotDir.Rotation());
				bFiredFromSocket = true;
			}
		}
	}

	if (!bFiredFromSocket)
	{
		float ShipHalfWidth = 300.0f;
		float CannonSpacing = 250.0f;
		FVector OwnerRight = Owner->GetActorRightVector();
		FVector BaseOffset = ((Side == ECannonSide::Left) ? -OwnerRight : OwnerRight) * ShipHalfWidth;

		for (int32 i = 0; i < CannonsPerSide; ++i)
		{
			float LengthOffset = (i - (CannonsPerSide - 1) * 0.5f) * CannonSpacing;
			FVector LengthDir = Owner->GetActorForwardVector();
			FVector SpawnLoc = Owner->GetActorLocation() + BaseOffset + LengthDir * LengthOffset + FVector(0.0f, 0.0f, 50.0f);
			FVector ShotDir = ApplySpread(ElevatedDir);
			SpawnCannonball(SpawnLoc, ShotDir, Data);
			PlayFireFX(SpawnLoc, ShotDir.Rotation());
		}
	}

	if (FireCameraShake)
	{
		if (APawn* OwnerPawn = Cast<APawn>(Owner))
		{
			if (APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController()))
			{
				PC->ClientStartCameraShake(FireCameraShake, FireCameraShakeScale);
			}
		}
	}

	if (Side == ECannonSide::Left) { bLeftReady = false; LeftReloadTimer = ReloadTime; }
	else { bRightReady = false; RightReloadTimer = ReloadTime; }
}

void UCannonComponent::SpawnCannonball(FVector SpawnLocation, FVector Direction, const FCannonballData& Data)
{
	UWorld* World = GetWorld();
	if (!World) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = Cast<APawn>(GetOwner());
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Виправлений безпечний запис замість тернарного оператора:
	TSubclassOf<ACannonball> ClassToSpawn = CannonballClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = ACannonball::StaticClass();
	}

	ACannonball* Ball = World->SpawnActor<ACannonball>(ClassToSpawn, SpawnLocation, Direction.Rotation(), SpawnParams);
	if (Ball) Ball->InitCannonball(Data, GetOwner());
}

void UCannonComponent::PlayFireFX(const FVector& Location, const FRotator& Rotation)
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (MuzzleFlashFX) UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, MuzzleFlashFX, Location, Rotation);
	if (FireSound) UGameplayStatics::PlaySoundAtLocation(World, FireSound, Location, FireSoundVolume);
}

void UCannonComponent::UpgradeCannonCount(int32 NewCountPerSide)
{
	CannonsPerSide = FMath::Clamp(NewCountPerSide, 2, 8);
}

void UCannonComponent::PredictBallisticArc(FVector SpawnLocation, FVector Direction, float SeaLevelZ, int32 MaxSteps, float StepSeconds, TArray<FVector>& OutPoints, FVector& OutImpactPoint) const
{
	OutPoints.Reset();
	OutPoints.Add(SpawnLocation);

	FCannonballData Data = (ActiveCannonballType == ECannonballType::Chain) ? FCannonballData::MakeChain() : FCannonballData::MakeStandard();

	FVector Pos = SpawnLocation;
	FVector Vel = Direction.GetSafeNormal() * Data.InitialSpeed;
	const float Gravity = 980.0f * Data.GravityScale;

	OutImpactPoint = Pos;

	for (int32 i = 0; i < MaxSteps; ++i)
	{
		Vel.Z -= Gravity * StepSeconds;
		Pos += Vel * StepSeconds;
		OutPoints.Add(Pos);

		if (Pos.Z <= SeaLevelZ)
		{
			OutImpactPoint = Pos;
			OutImpactPoint.Z = SeaLevelZ;
			break;
		}
		OutImpactPoint = Pos;
	}
}

void UCannonComponent::GetAimPrediction(ECannonSide Side, float SeaLevelZ, TArray<FVector>& OutImpactPoints, TArray<FVector>& OutTrajectoryStart, TArray<FVector>& OutTrajectoryEnd) const
{
	OutImpactPoints.Reset();
	OutTrajectoryStart.Reset();
	OutTrajectoryEnd.Reset();

	AActor* Owner = GetOwner();
	if (!Owner) return;

	// Використовуємо наш новий динамічний вектор прицілювання!
	FVector ElevatedDir;
	if (bIsAiming && Side == AimingSide)
	{
		float ElevRad = FMath::DegreesToRadians(DynamicElevationAngle);
		ElevatedDir = DynamicAzimuthDir * FMath::Cos(ElevRad) + FVector::UpVector * FMath::Sin(ElevRad);
	}
	else
	{
		const FVector OwnerRight = Owner->GetActorRightVector();
		FVector FireDirection = (Side == ECannonSide::Left) ? -OwnerRight : OwnerRight;
		const float ElevRad = FMath::DegreesToRadians(ElevationAngle);
		ElevatedDir = FireDirection * FMath::Cos(ElevRad) + FVector::UpVector * FMath::Sin(ElevRad);
	}
	ElevatedDir.Normalize();

	const FName SocketPrefix = (Side == ECannonSide::Left) ? LeftSocketPrefix : RightSocketPrefix;
	UMeshComponent* MeshComp = Owner->FindComponentByClass<UStaticMeshComponent>();
	if (!MeshComp) MeshComp = Owner->FindComponentByClass<USkeletalMeshComponent>();

	TArray<FVector> SpawnLocations;
	if (MeshComp)
	{
		for (int32 i = 0; i < CannonsPerSide; ++i)
		{
			const FName SocketName = FName(*FString::Printf(TEXT("%s%d"), *SocketPrefix.ToString(), i));
			if (MeshComp->DoesSocketExist(SocketName)) SpawnLocations.Add(MeshComp->GetSocketLocation(SocketName));
		}
	}

	if (SpawnLocations.Num() == 0)
	{
		const float ShipHalfWidth = 300.0f;
		const float CannonSpacing = 250.0f;
		FVector OwnerRight = Owner->GetActorRightVector();
		FVector BaseOffset = ((Side == ECannonSide::Left) ? -OwnerRight : OwnerRight) * ShipHalfWidth;
		const FVector LengthDir = Owner->GetActorForwardVector();
		for (int32 i = 0; i < CannonsPerSide; ++i)
		{
			const float LengthOffset = (i - (CannonsPerSide - 1) * 0.5f) * CannonSpacing;
			SpawnLocations.Add(Owner->GetActorLocation() + BaseOffset + LengthDir * LengthOffset + FVector(0.f, 0.f, 50.f));
		}
	}

	for (const FVector& Spawn : SpawnLocations)
	{
		TArray<FVector> Pts;
		FVector Impact;
		PredictBallisticArc(Spawn, ElevatedDir, SeaLevelZ, 40, 0.1f, Pts, Impact);
		OutImpactPoints.Add(Impact);

		for (int32 j = 0; j + 1 < Pts.Num(); ++j)
		{
			OutTrajectoryStart.Add(Pts[j]);
			OutTrajectoryEnd.Add(Pts[j + 1]);
		}
	}
}