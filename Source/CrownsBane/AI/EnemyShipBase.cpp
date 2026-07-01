// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "AI/EnemyShipBase.h"
#include "Combat/CannonComponent.h"
#include "Components/HealthComponent.h"
#include "Loot/LootSpawner.h"
#include "Systems/WantedLevelManager.h"
#include "Systems/DynamicMusicManager.h"
#include "Quests/BountyManager.h"
#include "UI/CrownsBaneHUD.h"
#include "Player/CrownsBanePlayerController.h"
#include "Player/ShipProgressionComponent.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"

AEnemyShipBase::AEnemyShipBase()
{
	PrimaryActorTick.bCanEverTick = true;

	ShipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShipMesh"));
	RootComponent = ShipMesh;
	ShipMesh->SetCollisionProfileName(TEXT("Pawn"));
	ShipMesh->SetSimulatePhysics(false);

	CannonComponent = CreateDefaultSubobject<UCannonComponent>(TEXT("CannonComponent"));
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

	DamageSmokeFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("DamageSmokeFX"));
	DamageSmokeFX->SetupAttachment(RootComponent);
	DamageSmokeFX->bAutoActivate = false;
	DamageSmokeFX->SetRelativeLocation(FVector(0.f, 0.f, 150.f));

	DamageFireFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("DamageFireFX"));
	DamageFireFX->SetupAttachment(RootComponent);
	DamageFireFX->bAutoActivate = false;
	DamageFireFX->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
}

void AEnemyShipBase::BeginPlay()
{
	Super::BeginPlay();

	SpawnLocation = GetActorLocation();
	PatrolTarget = SpawnLocation;

	if (HealthComponent)
	{
		HealthComponent->OnDeath.AddDynamic(this, &AEnemyShipBase::OnDeathDelegate);
		HealthComponent->OnHealthChanged.AddDynamic(this, &AEnemyShipBase::OnHealthChangedHandler);
	}

	if (DamageSmokeFX && SmokeAsset) DamageSmokeFX->SetAsset(SmokeAsset);
	if (DamageFireFX && FireAsset)  DamageFireFX->SetAsset(FireAsset);

	FVector RandomDir = FVector(FMath::FRandRange(-1.0f, 1.0f), FMath::FRandRange(-1.0f, 1.0f), 0.0f).GetSafeNormal();
	PatrolTarget = SpawnLocation + RandomDir * PatrolRadius * FMath::FRandRange(0.3f, 1.0f);
}

void AEnemyShipBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CurrentState == EShipAIState::Sink)
	{
		HandleStateSink(DeltaTime);
		return;
	}

	if (FireCooldownTimer > 0.0f)
	{
		FireCooldownTimer -= DeltaTime;
	}

	TimeSinceLastEvasive += DeltaTime;

	// Boss intro: show banner once when player enters DetectionRange of a named enemy.
	if (bNamedEnemy && !bIntroShown && IsPlayerInRange(DetectionRange))
	{
		CheckAndShowNamedIntro();
	}

	// Surrender + Evasive are sticky states — handle them first so the normal
	// state machine doesn't immediately override.
	if (CurrentState == EShipAIState::Surrender)
	{
		// Drift to a stop and stay there.  Player can still board for bonus loot.
		CurrentSpeedActual = FMath::FInterpTo(CurrentSpeedActual, 0.0f, DeltaTime, 1.5f);
		return;
	}

	if (CurrentState == EShipAIState::Evasive)
	{
		EvasiveTimeRemaining -= DeltaTime;
		// Sharp turn at full speed in the chosen direction.
		AddActorLocalRotation(FRotator(0.0f, EvasiveTurnSign * TurnRate * 2.5f * DeltaTime, 0.0f));
		CurrentSpeedActual = FMath::FInterpTo(CurrentSpeedActual, ChaseSpeed * 1.1f, DeltaTime, 2.0f);
		AddActorWorldOffset(GetActorForwardVector() * CurrentSpeedActual * DeltaTime, false);
		if (EvasiveTimeRemaining <= 0.0f)
		{
			TransitionToState(EShipAIState::Attack);
		}
		return;
	}

	const float HealthPct = HealthComponent ? HealthComponent->GetHealthPercent() : 1.0f;
	const bool bShouldRetreat = bCanRetreat && (RetreatHealthThreshold > 0.0f) && (HealthPct <= RetreatHealthThreshold);

	if (bShouldRetreat)
	{
		if (CurrentState != EShipAIState::Retreat) TransitionToState(EShipAIState::Retreat);
	}
	else if (CurrentState == EShipAIState::Retreat && !bShouldRetreat)
	{
		TransitionToState(EShipAIState::Patrol);
	}
	else
	{
		bool bCanEngagePlayer = bIgnoreWantedLevel || bHasAggro;
		if (!bCanEngagePlayer)
		{
			int32 WantedLevel = 0;
			for (TActorIterator<AWantedLevelManager> It(GetWorld()); It; ++It)
			{
				if (*It) { WantedLevel = (*It)->GetWantedLevel(); break; }
			}
			bCanEngagePlayer = (WantedLevel > 0);
		}

		if (bCanEngagePlayer && IsPlayerInRange(DetectionRange))
		{
			if (IsPlayerInRange(AttackRange))
			{
				if (CurrentState != EShipAIState::Attack) TransitionToState(EShipAIState::Attack);
			}
			else
			{
				if (CurrentState != EShipAIState::Chase) TransitionToState(EShipAIState::Chase);
			}
		}
		else
		{
			if (CurrentState != EShipAIState::Patrol) TransitionToState(EShipAIState::Patrol);
		}
	}

	switch (CurrentState)
	{
	case EShipAIState::Patrol:   HandleStatePatrol(DeltaTime);   break;
	case EShipAIState::Chase:    HandleStateChase(DeltaTime);    break;
	case EShipAIState::Attack:   HandleStateAttack(DeltaTime);   break;
	case EShipAIState::Retreat:  HandleStateRetreat(DeltaTime);  break;
	case EShipAIState::Sink:     HandleStateSink(DeltaTime);     break;
	}
}

float AEnemyShipBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float Actual = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (DamageAmount > 0.0f) bHasAggro = true;

	// Hit-flash: briefly tint the ship mesh bright white to signal impact.
	if (DamageAmount > 0.0f && ShipMesh)
	{
		ShipMesh->SetScalarParameterValueOnMaterials(TEXT("HitFlash"), 1.0f);
		if (UWorld* W = GetWorld())
		{
			FTimerHandle H;
			W->GetTimerManager().SetTimer(H, FTimerDelegate::CreateLambda([WMesh = TWeakObjectPtr<UStaticMeshComponent>(ShipMesh)]()
			{
				if (WMesh.IsValid()) WMesh->SetScalarParameterValueOnMaterials(TEXT("HitFlash"), 0.0f);
			}), 0.08f, false);
		}
	}

	// Take-a-hit reaction: chance to start an evasive maneuver (sharp turn).
	if (DamageAmount > 0.0f && bCanEvade && TimeSinceLastEvasive >= EvasiveCooldown
	    && CurrentState != EShipAIState::Sink
	    && CurrentState != EShipAIState::Surrender)
	{
		TimeSinceLastEvasive = 0.0f;
		EvasiveTimeRemaining = EvasiveDuration;
		EvasiveTurnSign = FMath::FRand() < 0.5f ? -1.0f : 1.0f;
		TransitionToState(EShipAIState::Evasive);
	}

	// Surrender check: if pushed below SurrenderHealthThreshold and rolled the chance.
	if (HealthComponent && HealthComponent->IsAlive()
	    && HealthComponent->GetHealthPercent() <= SurrenderHealthThreshold
	    && CurrentState != EShipAIState::Surrender
	    && CurrentState != EShipAIState::Sink
	    && FMath::FRand() < SurrenderChance)
	{
		TransitionToState(EShipAIState::Surrender);
	}

	return Actual;
}

void AEnemyShipBase::TransitionToState(EShipAIState NewState)
{
	if (CurrentState == NewState) return;
	CurrentState = NewState;
}

bool AEnemyShipBase::IsPlayerInRange(float Range) const
{
	APawn* Player = GetPlayerPawn();
	if (!Player) return false;
	return FVector::DistSquared(GetActorLocation(), Player->GetActorLocation()) <= (Range * Range);
}

APawn* AEnemyShipBase::GetPlayerPawn() const
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	return PC ? PC->GetPawn() : nullptr;
}

bool AEnemyShipBase::IsBroadsideAligned(ECannonSide& OutSide) const
{
	APawn* Player = GetPlayerPawn();
	if (!Player) return false;

	FVector ToPlayer = (Player->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	float DotRight = FVector::DotProduct(ToPlayer, GetActorRightVector());
	float DotForward = FVector::DotProduct(ToPlayer, GetActorForwardVector());

	if (FMath::Abs(DotRight) > FMath::Abs(DotForward))
	{
		OutSide = (DotRight > 0.0f) ? ECannonSide::Right : ECannonSide::Left;
		return true;
	}
	return false;
}

void AEnemyShipBase::TryFireAtPlayer()
{
	if (!CannonComponent || FireCooldownTimer > 0.0f) return;

	ECannonSide Side;
	if (IsBroadsideAligned(Side) && CannonComponent->CanFire(Side))
	{
		APawn* Player = GetPlayerPawn();
		if (Player)
		{
			// ШІ стріляє на випередження! 
			float Dist = FVector::Dist(GetActorLocation(), Player->GetActorLocation());
			float TimeToImpact = Dist / 3000.0f; // Базова швидкість ядра

			FVector TargetVel = Player->GetVelocity();
			FVector PredictedLoc = Player->GetActorLocation() + (TargetVel * TimeToImpact);
			PredictedLoc.Z = 0.0f;

			CannonComponent->SetIsAiming(true);
			CannonComponent->UpdateAimTarget(Side, PredictedLoc);
		}

		CannonComponent->FireBroadside(Side);
		CannonComponent->SetIsAiming(false); // Скидаємо стан після пострілу
		FireCooldownTimer = FireCooldown;
	}
}

void AEnemyShipBase::UpdateDamageFX()
{
	if (!HealthComponent) return;
	const float Pct = HealthComponent->GetHealthPercent();
	const bool bAlive = HealthComponent->IsAlive();

	if (DamageSmokeFX)
	{
		const bool bOn = bAlive && (Pct < SmokeHPThreshold);
		if (bOn && !DamageSmokeFX->IsActive()) DamageSmokeFX->Activate(true);
		if (!bOn && DamageSmokeFX->IsActive())  DamageSmokeFX->Deactivate();
	}
	if (DamageFireFX)
	{
		const bool bOn = bAlive && (Pct < FireHPThreshold);
		if (bOn && !DamageFireFX->IsActive()) DamageFireFX->Activate(true);
		if (!bOn && DamageFireFX->IsActive())  DamageFireFX->Deactivate();
	}
}

void AEnemyShipBase::HandleStatePatrol(float DeltaTime)
{
	const float DistToTarget = FVector::Dist(GetActorLocation(), PatrolTarget);
	if (DistToTarget < 300.0f)
	{
		PatrolWaitTimer += DeltaTime;
		if (PatrolWaitTimer >= 3.0f)
		{
			PatrolWaitTimer = 0.0f;
			FVector RandomDir = FVector(FMath::FRandRange(-1.0f, 1.0f), FMath::FRandRange(-1.0f, 1.0f), 0.0f).GetSafeNormal();
			PatrolTarget = SpawnLocation + RandomDir * PatrolRadius * FMath::FRandRange(0.3f, 1.0f);
		}
		MoveToward(PatrolTarget, 0.0f, DeltaTime); // Плавна зупинка
	}
	else
	{
		PatrolWaitTimer = 0.0f;
		TurnToward(PatrolTarget, DeltaTime);
		MoveToward(PatrolTarget, PatrolSpeed, DeltaTime);
	}
}

void AEnemyShipBase::HandleStateChase(float DeltaTime)
{
	APawn* Player = GetPlayerPawn();
	if (!Player) return;

	TurnToward(Player->GetActorLocation(), DeltaTime);
	MoveToward(Player->GetActorLocation(), ChaseSpeed, DeltaTime);
}

void AEnemyShipBase::HandleStateAttack(float DeltaTime)
{
	APawn* Player = GetPlayerPawn();
	if (!Player) return;

	const float Dist = FVector::Dist(GetActorLocation(), Player->GetActorLocation());
	FVector ToPlayerDir = (Player->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	ToPlayerDir.Z = 0.f;

	const float DotRight = FVector::DotProduct(ToPlayerDir, GetActorRightVector());

	if (FMath::Abs(DotRight) < 0.5f)
	{
		FVector RightVec = GetActorRightVector();
		FVector TurnTarget = GetActorLocation() + RightVec * (DotRight >= 0.0f ? 1.0f : -1.0f) * 2000.0f;
		TurnToward(TurnTarget, DeltaTime);
	}
	else
	{
		if (Dist < PreferredEngagementDistance - 200.0f)
		{
			FVector SideDir = (DotRight > 0.0f) ? GetActorRightVector() : -GetActorRightVector();
			FVector BackTarget = GetActorLocation() - GetActorForwardVector() * 500.0f + SideDir * 300.0f;
			TurnToward(BackTarget, DeltaTime);
		}
		else
		{
			TurnToward(Player->GetActorLocation(), DeltaTime);
		}
	}

	MoveToward(Player->GetActorLocation(), AttackSpeed, DeltaTime);
	TryFireAtPlayer();
}

void AEnemyShipBase::HandleStateRetreat(float DeltaTime)
{
	APawn* Player = GetPlayerPawn();

	if (!Player || !IsPlayerInRange(DetectionRange * 2.0f))
	{
		TransitionToState(EShipAIState::Patrol);
		return;
	}

	const FVector FromPlayer = (GetActorLocation() - Player->GetActorLocation()).GetSafeNormal2D();
	const FVector ToSpawn = (SpawnLocation - GetActorLocation()).GetSafeNormal2D();
	FVector FleeDir = (FromPlayer * 0.7f + ToSpawn * 0.3f).GetSafeNormal();

	const FVector FleeTarget = GetActorLocation() + FleeDir * 3000.0f;
	TurnToward(FleeTarget, DeltaTime);
	MoveToward(FleeTarget, RetreatSpeed, DeltaTime);
}

void AEnemyShipBase::HandleStateSink(float DeltaTime)
{
	SinkTimer += DeltaTime;
	const float Alpha = FMath::Clamp(SinkTimer / 6.0f, 0.0f, 1.0f);
	const float TargetPitch = FMath::InterpEaseOut(0.0f, 75.0f, Alpha, 2.5f);
	const float TargetRoll = FMath::InterpEaseOut(0.0f, 20.0f, Alpha, 2.0f);
	const float ZDescent = -FMath::InterpEaseIn(0.0f, 450.0f, Alpha, 2.0f) * DeltaTime;

	AddActorWorldOffset(FVector(0.0f, 0.0f, ZDescent));

	const FRotator Current = GetActorRotation();
	const FRotator Target(
		FMath::FInterpTo(Current.Pitch, TargetPitch, DeltaTime, 1.2f),
		Current.Yaw,
		FMath::FInterpTo(Current.Roll, TargetRoll, DeltaTime, 1.0f)
	);
	SetActorRotation(Target);

	static const float SplashTrigger = 3.5f;
	if (SinkTimer >= SplashTrigger && SinkTimer - DeltaTime < SplashTrigger)
	{
		if (DeathExplosionAsset)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(), DeathExplosionAsset, GetActorLocation(), FRotator::ZeroRotator);
		}
	}

	if (SinkTimer >= 6.0f) Destroy();
}

void AEnemyShipBase::MoveToward(FVector TargetLocation, float TargetSpeed, float DeltaTime)
{
	// Фізика руху: плавне прискорення та гальмування
	float Dist = FVector::Dist2D(TargetLocation, GetActorLocation());
	float DesiredSpeed = (Dist < 100.0f) ? 0.0f : TargetSpeed;

	CurrentSpeedActual = FMath::FInterpTo(CurrentSpeedActual, DesiredSpeed, DeltaTime, AccelerationInterpSpeed);

	FVector MoveDir = GetActorForwardVector();
	MoveDir.Z = 0.0f;
	AddActorWorldOffset(MoveDir * CurrentSpeedActual * DeltaTime, true);
}

void AEnemyShipBase::TurnToward(FVector TargetLocation, float DeltaTime)
{
	FVector ToTarget = (TargetLocation - GetActorLocation()).GetSafeNormal2D();

	// Система уникнення перешкод (Obstacle Avoidance)
	FHitResult Hit;
	FVector Start = GetActorLocation();
	FVector End = Start + GetActorForwardVector() * AvoidanceRayLength;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		// Якщо попереду перешкода, штучний інтелект відхиляється від неї
		FVector AvoidNormal = Hit.ImpactNormal.GetSafeNormal2D();
		ToTarget = (ToTarget + AvoidNormal * 2.0f).GetSafeNormal2D();
	}

	// Фізика повороту: плавне входження в поворот (інерція штурвалу)
	FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	float DotRight = FVector::DotProduct(GetActorRightVector(), ToTarget);
	float DotForward = FVector::DotProduct(Forward, ToTarget);
	float TurnDir = (DotRight > 0.0f) ? 1.0f : -1.0f;
	float AngleToTarget = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(DotForward, -1.0f, 1.0f)));

	float TargetYawSpeed = TurnDir * FMath::Min(TurnRate, AngleToTarget / DeltaTime);
	CurrentYawSpeed = FMath::FInterpTo(CurrentYawSpeed, TargetYawSpeed, DeltaTime, TurnInterpSpeed);

	AddActorLocalRotation(FRotator(0.0f, CurrentYawSpeed * DeltaTime, 0.0f));
}

void AEnemyShipBase::OnDeathDelegate() { OnDeath(); }
void AEnemyShipBase::OnHealthChangedHandler(float CurrentHealth, float MaxHealth) { UpdateDamageFX(); }

void AEnemyShipBase::OnDeath()
{
	TransitionToState(EShipAIState::Sink);

	const FVector Loc = GetActorLocation();
	if (DeathExplosionAsset) UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), DeathExplosionAsset, Loc, GetActorRotation());
	if (DeathSound) UGameplayStatics::PlaySoundAtLocation(GetWorld(), DeathSound, Loc);

	if (DamageSmokeFX) DamageSmokeFX->Deactivate();
	if (DamageFireFX)  DamageFireFX->Deactivate();

	// Kill feed entry on the player's HUD.
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		if (ACrownsBanePlayerController* CBPC = Cast<ACrownsBanePlayerController>(PC))
		{
			// SLOW-MO KILL — brief cinematic pause when player fells an enemy.
			if (UWorld* W = GetWorld())
			{
				UGameplayStatics::SetGlobalTimeDilation(W, 0.4f);
				FTimerHandle H;
				W->GetTimerManager().SetTimer(H, FTimerDelegate::CreateLambda([WW = TWeakObjectPtr<UWorld>(W)]() {
					if (WW.IsValid()) UGameplayStatics::SetGlobalTimeDilation(WW.Get(), 1.0f);
				}), 0.24f, false); // 0.24s in dilated time = ~0.6s real
			}
			CBPC->StatBumpShipsSunk();
			// XP per kill: 50 base, scaled by max HP / 100 to reward tougher kills.
			if (CBPC->Progression && HealthComponent)
			{
				int32 KillXP = FMath::Max(50, FMath::FloorToInt(HealthComponent->GetMaxHealth() * 0.5f));
				KillXP = FMath::CeilToInt(KillXP * FMath::Max(1.0f, XPMultiplier));
				CBPC->Progression->AwardXP(KillXP);
			}
			// Named kill banner + radial burst.
			if (bNamedEnemy)
			{
				if (ACrownsBaneHUD* HUD = Cast<ACrownsBaneHUD>(CBPC->GetHUD()))
				{
					const FString T = NamedTitle.IsEmpty() ? TEXT("LEGEND FELLED") : NamedTitle + TEXT(" FALLEN");
					HUD->ShowBanner(T, TEXT("A legend lies at the bottom of the sea."), NamedTint, 4.5f);
					HUD->TriggerRadialBurst(NamedTint, 1.4f);
				}
			}
			// Faction reputation: assume all enemies are naval — every kill
			// loses 10 Naval rep and gains 5 Pirate rep.
			CBPC->BumpFaction(-10.f, +5.f);
		}
		if (ACrownsBaneHUD* HUD = Cast<ACrownsBaneHUD>(PC->GetHUD()))
		{
			FString TypeName = TEXT("Ship");
			const FString N = GetClass()->GetName();
			if      (N.Contains(TEXT("Sloop")))   TypeName = TEXT("Sloop");
			else if (N.Contains(TEXT("Brig")))    TypeName = TEXT("Brig");
			else if (N.Contains(TEXT("Galleon"))) TypeName = TEXT("Galleon");
			HUD->PushKillFeed(FString::Printf(TEXT("You sunk a %s"), *TypeName));
		}
	}

	// Bounty / music tracking — both managers are optional so this is opt-in
	// (drop one in the level to enable).
	{
		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABountyManager::StaticClass(), Found);
		if (Found.Num() > 0) Cast<ABountyManager>(Found[0])->NotifyEnemyKilled();
	}
	{
		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADynamicMusicManager::StaticClass(), Found);
		if (Found.Num() > 0) Cast<ADynamicMusicManager>(Found[0])->NotifyEnemyKilled();
	}

	TArray<AActor*> Managers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AWantedLevelManager::StaticClass(), Managers);
	if (Managers.Num() > 0)
	{
		if (AWantedLevelManager* WLM = Cast<AWantedLevelManager>(Managers[0])) WLM->OnEnemyKilled();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (LootSpawnerClass) GetWorld()->SpawnActor<AActor>(LootSpawnerClass, Loc, FRotator::ZeroRotator, SpawnParams);
	else GetWorld()->SpawnActor<ALootSpawner>(ALootSpawner::StaticClass(), Loc, FRotator::ZeroRotator, SpawnParams);

	if (CannonComponent) CannonComponent->Deactivate();
}
void AEnemyShipBase::CheckAndShowNamedIntro()
{
	bIntroShown = true;
	if (UWorld* W = GetWorld())
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(W, 0))
		{
			if (ACrownsBaneHUD* HUD = Cast<ACrownsBaneHUD>(PC->GetHUD()))
			{
				const FString T = NamedTitle.IsEmpty() ? TEXT("LEGENDARY CAPTAIN") : NamedTitle;
				const FString S = NamedSubtitle.IsEmpty() ? TEXT("Prepare yer cannons.") : NamedSubtitle;
				HUD->ShowBanner(T, S, NamedTint, 3.5f);
				HUD->TriggerRadialBurst(NamedTint, 1.0f);
			}
		}
	}
}
