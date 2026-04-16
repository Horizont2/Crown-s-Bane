// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "Docks/DocksZone.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/HealthComponent.h"
#include "Systems/WantedLevelManager.h"
#include "Player/CrownsBanePlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

ADocksZone::ADocksZone()
{
	PrimaryActorTick.bCanEverTick = false;

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
	ResetWantedLevel();
	NotifyPlayerController(true);

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
