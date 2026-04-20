// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "Loot/TreasureMapPickup.h"
#include "Loot/TreasureQuestManager.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

ATreasureMapPickup::ATreasureMapPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	PickupSphere->InitSphereRadius(PickupRadius);
	PickupSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	RootComponent = PickupSphere;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	InitialLifeSpan = DespawnTime;
}

void ATreasureMapPickup::BeginPlay()
{
	Super::BeginPlay();

	SpawnZ = GetActorLocation().Z;
	PickupSphere->SetSphereRadius(PickupRadius);
	PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &ATreasureMapPickup::OnOverlapBegin);
	SetLifeSpan(DespawnTime);
}

void ATreasureMapPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (bCollected) return;

	BobTimer += DeltaTime;
	const float BobOffset = FMath::Sin(BobTimer * BobFrequency * 2.0f * PI) * BobAmplitude;
	FVector Loc = GetActorLocation();
	Loc.Z = SpawnZ + BobOffset;
	SetActorLocation(Loc);

	AddActorLocalRotation(FRotator(0.f, 30.f * DeltaTime, 0.f));
}

void ATreasureMapPickup::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (bCollected || !OtherActor) return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC) return;

	APawn* PlayerPawn = PC->GetPawn();
	if (OtherActor != PlayerPawn) return;

	// Locate the quest manager.
	ATreasureQuestManager* Mgr = nullptr;
	for (TActorIterator<ATreasureQuestManager> It(GetWorld()); It; ++It)
	{
		Mgr = *It;
		break;
	}

	if (!Mgr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TreasureMapPickup] No TreasureQuestManager in level."));
		return;
	}

	bCollected = true;

	const FGuid Issued = Mgr->IssueQuest(PlayerPawn->GetActorLocation());
	if (!Issued.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TreasureMapPickup] IssueQuest failed (capacity full?)"));
	}

	if (RevealFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), RevealFX, GetActorLocation(), FRotator::ZeroRotator);
	}
	if (RevealSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), RevealSound, GetActorLocation());
	}

	Destroy();
}
