// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "Combat/Cannonball.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/HealthComponent.h"
#include "Ship/ShipPawn.h"
#include "Kismet/GameplayStatics.h"
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
