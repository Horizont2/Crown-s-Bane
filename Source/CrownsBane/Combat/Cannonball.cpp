// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "Combat/Cannonball.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/HealthComponent.h"
#include "Ship/ShipPawn.h"
#include "AI/EnemyShipBase.h"
#include "UI/CrownsBaneHUD.h"
#include "Systems/WindSystem.h"
#include "Player/CrownsBanePlayerController.h"
#include "Player/ShipProgressionComponent.h"
#include "Audio/SoundManager.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "UObject/ConstructorHelpers.h"

ACannonball::ACannonball()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(15.0f);

	// The project's "Projectile" collision profile is misconfigured (CollisionEnabled=NoCollision).
	// Override explicitly so cannonballs actually report OnHit against ships and the world.
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Camera,    ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	CollisionSphere->SetNotifyRigidBodyCollision(true);
	CollisionSphere->SetGenerateOverlapEvents(false);
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

	// Wind drift — apply a small lateral velocity nudge based on world wind direction.
	// Looks up the level's AWindSystem once-ish (cached null check) so we don't search every tick.
	if (!ProjectileMovement) return;
	static const FName WindClassName = TEXT("WindSystem");
	UWorld* W = GetWorld();
	if (!W) return;
	AWindSystem* Wind = nullptr;
	for (TActorIterator<AWindSystem> It(W); It; ++It) { Wind = *It; break; }
	if (Wind)
	{
		const FVector WindDir   = Wind->GetWindDirection();
		float WindMag   = Wind->GetWindStrength();
		// WindReader perk halves drift on player shots.
		if (OwnerInstigator)
		{
			if (APawn* IP = Cast<APawn>(OwnerInstigator))
			{
				if (ACrownsBanePlayerController* CBPC = Cast<ACrownsBanePlayerController>(IP->GetController()))
				{
					if (CBPC->Progression && CBPC->Progression->HasPerk(EShipPerk::WindReader))
					{
						WindMag *= 0.5f;
					}
				}
			}
		}
		// 80 cm/s^2 max lateral push at full wind — perceptible across long shots, not silly at close range.
		const FVector LateralAccel = WindDir * 80.0f * WindMag;
		ProjectileMovement->Velocity += LateralAccel * DeltaTime;
	}
}

void ACannonball::InitCannonball(const FCannonballData& InData, AActor* InInstigator)
{
	CannonballData = InData;
	OwnerInstigator = InInstigator;

	// CRITICAL: ignore collision with the firing ship so the ball can leave the
	// hull without immediately getting stuck against the ship's own collider.
	if (InInstigator && CollisionSphere)
	{
		CollisionSphere->IgnoreActorWhenMoving(InInstigator, true);
		// Ignore attached child actors (cannon meshes etc.) too.
		TArray<AActor*> Attached;
		InInstigator->GetAttachedActors(Attached);
		for (AActor* A : Attached)
		{
			if (A) CollisionSphere->IgnoreActorWhenMoving(A, true);
		}
	}

	ProjectileMovement->InitialSpeed = InData.InitialSpeed;
	ProjectileMovement->MaxSpeed = InData.MaxSpeed;
	ProjectileMovement->ProjectileGravityScale = InData.GravityScale;
	// Kick velocity in the fire direction so ProjectileMovement doesn't wait a frame.
	ProjectileMovement->Velocity = GetActorForwardVector() * InData.InitialSpeed;
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

	// �����ު�� ��������� FVECTOR ���� ��� ��� �Ѳ�� ����ֲ�
	const FVector ImpactLoc = Hit.ImpactPoint.IsZero() ? GetActorLocation() : FVector(Hit.ImpactPoint);

	AController* InstCtrl = OwnerInstigator ? OwnerInstigator->GetInstigatorController() : nullptr;

	// Critical-hit roll: 15% baseline, +5% if hit Z is above target's mid-line
	// (a high impact = mast/rigging hit, more likely to dismount something).
	const FVector OtherCenter = OtherActor->GetActorLocation();
	const bool bHighHit = (ImpactLoc.Z - OtherCenter.Z) > 50.0f;
	float CritChance = bHighHit ? 0.20f : 0.15f;

	// Marksman perk: +10% crit chance on player shots.
	if (OwnerInstigator)
	{
		if (APawn* IP = Cast<APawn>(OwnerInstigator))
		{
			if (ACrownsBanePlayerController* CBPC = Cast<ACrownsBanePlayerController>(IP->GetController()))
			{
				if (CBPC->Progression && CBPC->Progression->HasPerk(EShipPerk::Marksman))
				{
					CritChance += 0.10f;
				}
			}
		}
	}

	const bool bCrit = FMath::FRand() < CritChance;
	const float FinalDamage = CannonballData.BaseDamage * (bCrit ? 2.0f : 1.0f);

	// Direct hit damage.
	UGameplayStatics::ApplyDamage(
		OtherActor,
		FinalDamage,
		InstCtrl,
		this,
		UDamageType::StaticClass()
	);

	// Knockback: small impulse along the cannonball's velocity direction, scaled
	// by damage.  Skips the player ship (we don't want the player getting
	// punted around their own collision body).
	const bool bHitShipForKB = OtherActor->IsA(AShipPawn::StaticClass()) ||
		OtherActor->IsA(AEnemyShipBase::StaticClass());
	if (bHitShipForKB && OtherActor != OwnerInstigator)
	{
		const FVector Dir = GetVelocity().GetSafeNormal2D();
		if (!Dir.IsZero())
		{
			const float KBStrength = FinalDamage * 8.0f;
			OtherActor->AddActorWorldOffset(Dir * KBStrength * 0.01f, false);
		}
	}

	// Apply chain shot slow effect
	if (CannonballData.Type == ECannonballType::Chain)
	{
		ApplySlowEffect(OtherActor);
	}

	// Explosive splash: deal half-damage to every actor within SplashRadius of impact.
	if (CannonballData.SplashRadius > 0.0f && CannonballData.Type == ECannonballType::Explosive)
	{
		const FVector Center = ImpactLoc;
		const float R = CannonballData.SplashRadius;
		const float R2 = R * R;
		TArray<AActor*> Overlapped;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), Overlapped);
		for (AActor* A : Overlapped)
		{
			if (!A || A == OtherActor || A == OwnerInstigator || A == this) continue;
			if (FVector::DistSquared(A->GetActorLocation(), Center) > R2) continue;
			const float Falloff = 1.0f - FMath::Sqrt(FVector::DistSquared(A->GetActorLocation(), Center) / R2);
			UGameplayStatics::ApplyDamage(A, CannonballData.BaseDamage * 0.55f * Falloff, InstCtrl, this, UDamageType::StaticClass());
		}
	}

	// Queue a floating damage number on the HUD
	const bool bHitShip = OtherActor->IsA(AShipPawn::StaticClass());
	if (UWorld* W = GetWorld())
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(W, 0))
		{
			const AActor* Inst = OwnerInstigator;
			const APawn* InstPawn = Cast<APawn>(Inst);
			const bool bPlayerShot = InstPawn && Cast<APlayerController>(InstPawn->GetController());
			if (bPlayerShot)
			{
				if (ACrownsBaneHUD* HUD = Cast<ACrownsBaneHUD>(PC->GetHUD()))
				{
					HUD->AddFloatingDamage(ImpactLoc, FinalDamage, bHitShip);
					HUD->TriggerHitMarker(bHitShip && bCrit ? true : bHitShip);
					if (bHitShip) HUD->RegisterPlayerHit();
				}
				// XP: 5 per hit, +5 bonus for crits.
				if (bHitShip)
				{
					if (ACrownsBanePlayerController* CBPC = Cast<ACrownsBanePlayerController>(PC))
					{
						if (CBPC->Progression) CBPC->Progression->AwardXP(bCrit ? 10 : 5);
						if (CBPC->SoundManager)
						{
							CBPC->SoundManager->PlayAtLocation(ESoundCue::Impact, ImpactLoc, 1.0f);
						}
					}
				}
				// Hit-stop on confirmed ship hits only (skip water hits).
				if (bHitShip)
				{
					if (AShipPawn* PlayerShip = Cast<AShipPawn>(PC->GetPawn()))
					{
						PlayerShip->TriggerHitStop();
					}
				}
			}
		}

		// Impact FX
		const FRotator ImpactRot = Hit.ImpactNormal.IsNearlyZero()
			? FRotator::ZeroRotator
			: Hit.ImpactNormal.Rotation();

		UNiagaraSystem* FX = bHitShip ? ImpactHullFX : ImpactWaterFX;
		USoundBase* SFX = bHitShip ? ImpactSound : WaterSplashSound;

		if (FX)
		{
			// ������������� ��� ������������ ImpactLoc
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(W, FX, ImpactLoc, ImpactRot);
		}
		if (SFX)
		{
			// ������������� ��� ������������ ImpactLoc
			UGameplayStatics::PlaySoundAtLocation(W, SFX, ImpactLoc);
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