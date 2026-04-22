// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "Combat/CannonComponent.h"
#include "Combat/Cannonball.h"
#include "Combat/ProjectileTypes.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/MeshComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Camera/CameraShakeBase.h"
#include "DrawDebugHelpers.h"

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

void UCannonComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Tick down reload timers
	if (!bLeftReady)
	{
		LeftReloadTimer -= DeltaTime;
		if (LeftReloadTimer <= 0.0f)
		{
			LeftReloadTimer = 0.0f;
			bLeftReady = true;
			UE_LOG(LogTemp, Log, TEXT("CannonComponent: Port (left) cannons reloaded."));
		}
	}

	if (!bRightReady)
	{
		RightReloadTimer -= DeltaTime;
		if (RightReloadTimer <= 0.0f)
		{
			RightReloadTimer = 0.0f;
			bRightReady = true;
			UE_LOG(LogTemp, Log, TEXT("CannonComponent: Starboard (right) cannons reloaded."));
		}
	}
}

bool UCannonComponent::CanFire(ECannonSide Side) const
{
	return (Side == ECannonSide::Left) ? bLeftReady : bRightReady;
}

float UCannonComponent::GetReloadProgress(ECannonSide Side) const
{
	if (Side == ECannonSide::Left)
	{
		if (bLeftReady) return 1.0f;
		return 1.0f - (LeftReloadTimer / ReloadTime);
	}
	else
	{
		if (bRightReady) return 1.0f;
		return 1.0f - (RightReloadTimer / ReloadTime);
	}
}

void UCannonComponent::FireBroadside(ECannonSide Side)
{
	if (!CanFire(Side))
	{
		UE_LOG(LogTemp, Log, TEXT("CannonComponent: %s broadside not ready."),
			Side == ECannonSide::Left ? TEXT("Port") : TEXT("Starboard"));
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("CannonComponent: Firing %s broadside (%d cannons)."),
		Side == ECannonSide::Left ? TEXT("port") : TEXT("starboard"), CannonsPerSide);

	// Build cannonball data based on active type
	FCannonballData Data;
	if (ActiveCannonballType == ECannonballType::Chain)
	{
		Data = FCannonballData::MakeChain();
	}
	else
	{
		Data = FCannonballData::MakeStandard();
	}
	Data.BaseDamage = DamagePerCannon;

	// Determine the lateral direction (+right or +left of ship forward)
	FVector OwnerRight = Owner->GetActorRightVector();
	FVector FireDirection = (Side == ECannonSide::Left) ? -OwnerRight : OwnerRight;

	// Apply elevation angle upward
	FVector Up = FVector::UpVector;
	float ElevRad = FMath::DegreesToRadians(ElevationAngle);
	FVector ElevatedDir = FireDirection * FMath::Cos(ElevRad) + Up * FMath::Sin(ElevRad);
	ElevatedDir.Normalize();

	// Try to find cannon sockets on mesh components
	FName SocketPrefix = (Side == ECannonSide::Left) ? LeftSocketPrefix : RightSocketPrefix;
	UMeshComponent* MeshComp = Owner->FindComponentByClass<UStaticMeshComponent>();
	if (!MeshComp)
	{
		MeshComp = Owner->FindComponentByClass<USkeletalMeshComponent>();
	}

	FRotator SpawnRot = ElevatedDir.Rotation();

	bool bFiredFromSocket = false;
	const float HalfSpread = CannonSpreadAngle * 0.5f;

	auto ApplySpread = [&](FVector BaseDir) -> FVector
	{
		float YawJitter   = FMath::FRandRange(-HalfSpread, HalfSpread);
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
				FVector ShotDir  = ApplySpread(ElevatedDir);
				SpawnCannonball(SpawnLoc, ShotDir, Data);
				PlayFireFX(SpawnLoc, ShotDir.Rotation());
				bFiredFromSocket = true;
			}
		}
	}

	// Fallback: spawn from owner location offset by ship width
	if (!bFiredFromSocket)
	{
		float ShipHalfWidth = 300.0f;
		float CannonSpacing = 250.0f;
		FVector BaseOffset = FireDirection * ShipHalfWidth;

		for (int32 i = 0; i < CannonsPerSide; ++i)
		{
			float LengthOffset = (i - (CannonsPerSide - 1) * 0.5f) * CannonSpacing;
			FVector LengthDir = Owner->GetActorForwardVector();
			FVector SpawnLoc = Owner->GetActorLocation() + BaseOffset + LengthDir * LengthOffset
				+ FVector(0.0f, 0.0f, 50.0f);
			FVector ShotDir  = ApplySpread(ElevatedDir);
			SpawnCannonball(SpawnLoc, ShotDir, Data);
			PlayFireFX(SpawnLoc, ShotDir.Rotation());
		}
	}

	// Camera shake — only once per broadside, played on the instigator PC.
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

	// Start reload
	if (Side == ECannonSide::Left)
	{
		bLeftReady = false;
		LeftReloadTimer = ReloadTime;
	}
	else
	{
		bRightReady = false;
		RightReloadTimer = ReloadTime;
	}
}

void UCannonComponent::SpawnCannonball(FVector SpawnLocation, FVector Direction, const FCannonballData& Data)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = Cast<APawn>(GetOwner());
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	FRotator SpawnRotation = Direction.Rotation();

	TSubclassOf<ACannonball> ClassToSpawn = CannonballClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = ACannonball::StaticClass();
	}
	ACannonball* Ball = World->SpawnActor<ACannonball>(ClassToSpawn, SpawnLocation, SpawnRotation, SpawnParams);
	if (Ball)
	{
		Ball->InitCannonball(Data, GetOwner());
	}
}

void UCannonComponent::PlayFireFX(const FVector& Location, const FRotator& Rotation)
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (MuzzleFlashFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, MuzzleFlashFX, Location, Rotation);
	}
	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(World, FireSound, Location, FireSoundVolume);
	}
}

void UCannonComponent::UpgradeCannonCount(int32 NewCountPerSide)
{
	// Clamp to 2-8 range matching design specs
	CannonsPerSide = FMath::Clamp(NewCountPerSide, 2, 8);
	UE_LOG(LogTemp, Log, TEXT("CannonComponent: Upgraded to %d cannons per side."), CannonsPerSide);
}
