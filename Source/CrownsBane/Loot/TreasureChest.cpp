// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "Loot/TreasureChest.h"
#include "Loot/TreasureQuestManager.h"
#include "Loot/ResourceTypes.h"
#include "Player/PlayerInventory.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

ATreasureChest::ATreasureChest()
{
	PrimaryActorTick.bCanEverTick = true;

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	PickupSphere->InitSphereRadius(PickupRadius);
	PickupSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	RootComponent = PickupSphere;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ATreasureChest::BeginPlay()
{
	Super::BeginPlay();

	SpawnZ = GetActorLocation().Z;
	PickupSphere->SetSphereRadius(PickupRadius);
	PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &ATreasureChest::OnOverlapBegin);

	if (BeaconFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			BeaconFX,
			RootComponent,
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			/*bAutoDestroy=*/ false
		);
	}

	UE_LOG(LogTemp, Log, TEXT("[TreasureChest] Spawned at %s (Quest %s)"),
		*GetActorLocation().ToString(), *QuestId.ToString());
}

void ATreasureChest::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bLooted) return;

	BobTimer += DeltaTime;
	const float BobOffset = FMath::Sin(BobTimer * BobFrequency * 2.0f * PI) * BobAmplitude;
	FVector Loc = GetActorLocation();
	Loc.Z = SpawnZ + BobOffset;
	SetActorLocation(Loc);

	// Gentle rotation for visual appeal
	AddActorLocalRotation(FRotator(0.f, 8.0f * DeltaTime, 0.f));
}

void ATreasureChest::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (bLooted || !OtherActor) return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC) return;

	APawn* PlayerPawn = PC->GetPawn();
	if (OtherActor != PlayerPawn) return;

	UPlayerInventory* Inventory = PlayerPawn->FindComponentByClass<UPlayerInventory>();
	if (!Inventory)
	{
		Inventory = PC->FindComponentByClass<UPlayerInventory>();
	}

	if (!Inventory)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TreasureChest] No PlayerInventory found."));
		return;
	}

	bLooted = true;
	GrantRewards(Inventory);

	const FVector Loc = GetActorLocation();
	if (OpenFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), OpenFX, Loc, FRotator::ZeroRotator);
	}
	if (OpenSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), OpenSound, Loc);
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 6.f, FColor::Yellow,
			FString::Printf(TEXT("Treasure recovered! +%d Gold, +%d Wood, +%d Metal"),
				GoldReward, WoodReward, MetalReward));
	}

	// Notify the quest manager so it can clear the active quest.
	for (TActorIterator<ATreasureQuestManager> It(GetWorld()); It; ++It)
	{
		if (ATreasureQuestManager* Mgr = *It)
		{
			Mgr->CompleteQuest(QuestId);
			break;
		}
	}

	Destroy();
}

void ATreasureChest::GrantRewards(UPlayerInventory* Inventory)
{
	if (!Inventory) return;
	if (GoldReward > 0)  Inventory->AddResource(EResourceType::Gold, GoldReward);
	if (WoodReward > 0)  Inventory->AddResource(EResourceType::Wood, WoodReward);
	if (MetalReward > 0) Inventory->AddResource(EResourceType::Metal, MetalReward);
}
