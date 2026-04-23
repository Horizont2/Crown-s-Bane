// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "Combat/Cannonball.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/HealthComponent.h"
#include "Ship/ShipPawn.h"
#include "UI/CrownsBaneHUD.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "UObject/ConstructorHelpers.h"

ACannonball::ACannonball()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(15.0f);
	CollisionSphere->SetCollisionProfileName(TEXT("Projectile"));
	CollisionSphere->SetNotifyRigidBodyCollision(true);
	RootComponent = CollisionSphere;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionSphere;
	ProjectileMovement->InitialSpeed = 3000.0f;
	ProjectileMovement->MaxSpeed = 3000.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 1.0f;

	InitialLifeSpan = LifeSpan;
}

void ACannonball::BeginPlay()
{
	Super::BeginPlay();

	CollisionSphere->OnComponentHit.AddDynamic(this, &ACannonball::OnHit);

	// Set lifespan so cannonballs auto-destroy
	SetLifeSpan(LifeSpan);
}

void ACannonball::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ACannonball::InitCannonball(const FCannonballData& InData, AActor* InInstigator)
{
	CannonballData = InData;
	OwnerInstigator = InInstigator;

	ProjectileMovement->InitialSpeed = InData.InitialSpeed;
	ProjectileMovement->MaxSpeed = InData.MaxSpeed;
	ProjectileMovement->ProjectileGravityScale = InData.GravityScale;
}

void ACannonball::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (bHasHit)
	{
		return;
	}

	if (OtherActor == nullptr || OtherActor == this || OtherActor == OwnerInstigator)
	{
		return;
	}

	bHasHit = true;

	UE_LOG(LogTemp, Log, TEXT("Cannonball hit: %s"), *OtherActor->GetName());

	// Apply damage via UE damage system
	UGameplayStatics::ApplyDamage(
		OtherActor,
		CannonballData.BaseDamage,
		OwnerInstigator ? OwnerInstigator->GetInstigatorController() : nullptr,
		this,
		UDamageType::StaticClass()
	);

	// Apply chain shot slow effect
	if (CannonballData.Type == ECannonballType::Chain)
	{
		ApplySlowEffect(OtherActor);
	}

	// Queue a floating damage number on the HUD Б─■ only for player-originated shots
	// (instigator is player-controlled) so enemy hits on the player don't spam.
	const bool bHitShip = OtherActor->IsA(AShipPawn::StaticClass());
	if (UWorld* W = GetWorld())
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(W, 0))
		{
			const AActor* Inst = OwnerInstigator;
			const APawn*  InstPawn = Cast<APawn>(Inst);
			const bool bPlayerShot = InstPawn && Cast<APlayerController>(InstPawn->GetController());
			if (bPlayerShot)
			{
				if (ACrownsBaneHUD* HUD = Cast<ACrownsBaneHUD>(PC->GetHUD()))
				{
					HUD->AddFloatingDamage(Hit.ImpactPoint.IsZero() ? GetActorLocation() : Hit.ImpactPoint,
						CannonballData.BaseDamage, bHitShip);
				}
			}
		}
	}

	// Impact FX
	UWorld* World = GetWorld();
	if (World)
	{
		// бхопюбкемн рср:
		const FVector ImpactLoc = Hit.ImpactPoint.IsZero() ? GetActorLocation() : FVector(Hit.ImpactPoint);
		const FRotator ImpactRot = Hit.ImpactNormal.IsNearlyZero()
			? FRotator::ZeroRotator
			: Hit.ImpactNormal.Rotation();

		UNiagaraSystem* FX = bHitShip ? ImpactHullFX : ImpactWaterFX;
		USoundBase* SFX = bHitShip ? ImpactSound : WaterSplashSound;

		if (FX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, FX, ImpactLoc, ImpactRot);
		}
		if (SFX)
		{
			UGameplayStatics::PlaySoundAtLocation(World, SFX, ImpactLoc);
		}
	}

	Destroy();
}

void ACannonball::ApplySlowEffect(AActor* TargetActor)
{
	if (!TargetActor)
	{
		return;
	}

	AShipPawn* TargetShip = Cast<AShipPawn>(TargetActor);
	if (TargetShip)
	{
		TargetShip->ApplySpeedPenalty(CannonballData.SlowFraction, CannonballData.SlowDuration);
		UE_LOG(LogTemp, Log, TEXT("Chain shot slowed %s by %.0f%% for %.1fs"),
			*TargetActor->GetName(),
			CannonballData.SlowFraction * 100.0f,
			CannonballData.SlowDuration);
	}
}