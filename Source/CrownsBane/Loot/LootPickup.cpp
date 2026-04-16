// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "Loot/LootPickup.h"
#include "Player/PlayerInventory.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

ALootPickup::ALootPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	PickupSphere->InitSphereRadius(PickupRadius);
	PickupSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	RootComponent = PickupSphere;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Auto-despawn
	InitialLifeSpan = DespawnTime;
}

void ALootPickup::BeginPlay()
{
	Super::BeginPlay();

	SpawnZ = GetActorLocation().Z;
	PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &ALootPickup::OnOverlapBegin);
	SetLifeSpan(DespawnTime);

	UE_LOG(LogTemp, Log, TEXT("LootPickup: Spawned %d %s at %s"),
		ResourceDrop.Amount,
		ResourceDrop.ResourceType == EResourceType::Gold ? TEXT("Gold") :
		ResourceDrop.ResourceType == EResourceType::Wood ? TEXT("Wood") : TEXT("Metal"),
		*GetActorLocation().ToString());
}

void ALootPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bPickedUp) return;

	// Bob up and down to simulate floating on water
	BobTimer += DeltaTime;
	float BobOffset = FMath::Sin(BobTimer * BobFrequency * 2.0f * PI) * BobAmplitude;

	FVector Loc = GetActorLocation();
	Loc.Z = SpawnZ + BobOffset;
	SetActorLocation(Loc);
}

void ALootPickup::SetResourceDrop(EResourceType Type, int32 Amount)
{
	ResourceDrop.ResourceType = Type;
	ResourceDrop.Amount = Amount;
}

void ALootPickup::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (bPickedUp) return;
	if (!OtherActor) return;

	// Check if this is the player pawn
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC) return;

	APawn* PlayerPawn = PC->GetPawn();
	if (OtherActor != PlayerPawn) return;

	// Find the PlayerInventory component on the player pawn
	UPlayerInventory* Inventory = PlayerPawn->FindComponentByClass<UPlayerInventory>();
	if (!Inventory)
	{
		// Try finding it on the controller
		Inventory = PC->FindComponentByClass<UPlayerInventory>();
	}

	if (Inventory)
	{
		Inventory->AddResource(ResourceDrop.ResourceType, ResourceDrop.Amount);
		bPickedUp = true;

		UE_LOG(LogTemp, Log, TEXT("LootPickup: Player collected %d %s."),
			ResourceDrop.Amount,
			ResourceDrop.ResourceType == EResourceType::Gold ? TEXT("Gold") :
			ResourceDrop.ResourceType == EResourceType::Wood ? TEXT("Wood") : TEXT("Metal"));

		Destroy();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("LootPickup: No PlayerInventory found on player!"));
	}
}
