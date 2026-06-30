// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "Docks/DocksZone.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/HealthComponent.h"
#include "Systems/WantedLevelManager.h"
#include "Player/CrownsBanePlayerController.h"
#include "Ship/ShipPawn.h"
#include "UI/CrownsBaneHUD.h"
#include "AI/EnemyShipBase.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

ADocksZone::ADocksZone()
{
	PrimaryActorTick.bCanEverTick = true;

	DocksVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("DocksVolume"));
	DocksVolume->SetBoxExtent(FVector(2000.0f, 2000.0f, 500.0f));
	DocksVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	RootComponent = DocksVolume;

	DocksMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DocksMesh"));
	DocksMesh->SetupAttachment(RootComponent);
	DocksMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ADocksZone::BeginPlay()
{
	Super::BeginPlay();

	DocksVolume->OnComponentBeginOverlap.AddDynamic(this, &ADocksZone::OnOverlapBegin);
	DocksVolume->OnComponentEndOverlap.AddDynamic(this, &ADocksZone::OnOverlapEnd);

	UE_LOG(LogTemp, Log, TEXT("DocksZone: Docks initialized at %s"), *GetActorLocation().ToString());
}

bool ADocksZone::IsLocationInSafeZone(FVector Location) const
{
	return FVector::Dist(Location, GetActorLocation()) <= SafeZoneRadius;
}

void ADocksZone::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor) return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC || PC->GetPawn() != OtherActor) return;

	bPlayerInDocks = true;
	UE_LOG(LogTemp, Log, TEXT("DocksZone: Player entered docks."));

	HealPlayer(OtherActor);
	// Amnesty: only PirateHaven and Merchant clear wanted level.
	// Naval ports do NOT clear it — captains there report you to the fleet.
	if (DockType != EDockType::Naval)
	{
		ResetWantedLevel();
	}
	NotifyPlayerController(true);

	// Repair sails on dock entry.
	if (AShipPawn* Ship = Cast<AShipPawn>(OtherActor))
	{
		Ship->SailIntegrity = FMath::Min(1.0f, Ship->SailIntegrity + Ship->SailRegenAtDock);
	}

	// Auto-save snapshot — and notify HUD.
	APlayerController* PCNow = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (ACrownsBanePlayerController* CBPC = Cast<ACrownsBanePlayerController>(PCNow))
	{
		CBPC->AutoSaveGame(GetActorLocation(), GetActorRotation());
		if (ACrownsBaneHUD* HUD = Cast<ACrownsBaneHUD>(CBPC->GetHUD()))
		{
			HUD->PushResourceToast(TEXT("✓ Game saved"), FLinearColor(0.4f, 1.0f, 0.5f, 1.0f));

			// Welcome banner with dock type + name.
			const TCHAR* TypeStr = TEXT("Port");
			FLinearColor Tint(1.0f, 0.85f, 0.30f, 1.0f);
			switch (DockType)
			{
			case EDockType::Merchant:    TypeStr = TEXT("MERCHANT PORT");  Tint = FLinearColor(1.0f, 0.85f, 0.30f, 1.0f); break;
			case EDockType::Naval:       TypeStr = TEXT("NAVAL PORT");     Tint = FLinearColor(0.5f, 0.7f, 1.0f, 1.0f); break;
			case EDockType::PirateHaven: TypeStr = TEXT("PIRATE HAVEN");   Tint = FLinearColor(1.0f, 0.5f, 0.3f, 1.0f); break;
			}
			const FString Subtitle = (DockType == EDockType::Naval)
				? TEXT("Watch yer back — no amnesty here.")
				: TEXT("Wanted level cleared.");
			HUD->ShowBanner(FString::Printf(TEXT("%s — %s"), TypeStr, *DockName), Subtitle, Tint, 3.5f);
		}
	}

	OnPlayerEnterDocks.Broadcast();
}

void ADocksZone::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor) return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC || PC->GetPawn() != OtherActor) return;

	bPlayerInDocks = false;
	UE_LOG(LogTemp, Log, TEXT("DocksZone: Player left docks."));

	NotifyPlayerController(false);
	OnPlayerExitDocks.Broadcast();
}

void ADocksZone::HealPlayer(AActor* PlayerActor)
{
	UHealthComponent* Health = PlayerActor->FindComponentByClass<UHealthComponent>();
	if (!Health)
	{
		UE_LOG(LogTemp, Warning, TEXT("DocksZone: No HealthComponent on player!"));
		return;
	}

	if (HealFraction >= 1.0f)
	{
		Health->FullHeal();
		UE_LOG(LogTemp, Log, TEXT("DocksZone: Player fully healed."));
	}
	else
	{
		float HealAmount = Health->GetMaxHealth() * HealFraction;
		Health->Heal(HealAmount);
		UE_LOG(LogTemp, Log, TEXT("DocksZone: Player healed for %.1f HP."), HealAmount);
	}
}

void ADocksZone::ResetWantedLevel()
{
	TArray<AActor*> Managers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AWantedLevelManager::StaticClass(), Managers);
	if (Managers.Num() > 0)
	{
		AWantedLevelManager* WLM = Cast<AWantedLevelManager>(Managers[0]);
		if (WLM)
		{
			WLM->ResetWantedLevel();
			UE_LOG(LogTemp, Log, TEXT("DocksZone: Wanted level reset."));
		}
	}
}

void ADocksZone::NotifyPlayerController(bool bEntering)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC) return;

	ACrownsBanePlayerController* CrownPC = Cast<ACrownsBanePlayerController>(PC);
	if (CrownPC)
	{
		if (bEntering)
		{
			CrownPC->OnEnterDocks(this);
		}
		else
		{
			CrownPC->OnExitDocks();
		}
	}
}

void ADocksZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	PatrolCooldownTimer = FMath::Max(0.0f, PatrolCooldownTimer - DeltaTime);

	if (DockType != EDockType::Naval) return;
	if (PatrolCooldownTimer > 0.0f) return;
	if (!NavalPatrolShipClass) return;

	UWorld* W = GetWorld();
	if (!W) return;

	APawn* Player = UGameplayStatics::GetPlayerPawn(W, 0);
	if (!Player) return;

	// Read wanted level from the manager.
	int32 Wanted = 0;
	for (TActorIterator<AWantedLevelManager> It(W); It; ++It)
	{
		if (*It) { Wanted = (*It)->GetWantedLevel(); break; }
	}
	if (Wanted < 1) return;

	const float D2 = FVector::DistSquared(Player->GetActorLocation(), GetActorLocation());
	if (D2 > NavalPatrolRange * NavalPatrolRange) return;

	// Spawn patrol ships in a small arc between the dock and the player.
	const FVector DirToPlayer = (Player->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	for (int32 i = 0; i < PatrolShipsToSpawn; ++i)
	{
		const float Offset = (i - (PatrolShipsToSpawn - 1) * 0.5f) * 600.0f;
		const FVector SpawnLoc = GetActorLocation() + DirToPlayer * 2200.0f + FVector(0, Offset, 0);
		W->SpawnActor<AEnemyShipBase>(NavalPatrolShipClass, SpawnLoc, DirToPlayer.Rotation(), Params);
	}

	PatrolCooldownTimer = PatrolSpawnCooldown;

	// HUD banner.
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(W, 0))
	{
		if (ACrownsBaneHUD* HUD = Cast<ACrownsBaneHUD>(PC->GetHUD()))
		{
			HUD->ShowBanner(
				FString::Printf(TEXT("⚠ NAVAL PATROL — %s"), *DockName),
				TEXT("Lay low or fight your way out."),
				FLinearColor(0.5f, 0.7f, 1.0f, 1.0f), 3.5f);
		}
	}
}
