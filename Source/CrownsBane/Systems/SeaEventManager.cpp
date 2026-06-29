// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "Systems/SeaEventManager.h"
#include "Loot/LootPickup.h"
#include "Loot/TreasureQuestManager.h"
#include "AI/EnemyShipBase.h"
#include "UI/CrownsBaneHUD.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"

ASeaEventManager::ASeaEventManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ASeaEventManager::BeginPlay()
{
	Super::BeginPlay();
	TimeUntilNext = FMath::FRandRange(MinInterval, MaxInterval);
}

void ASeaEventManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	TimeUntilNext -= DeltaTime;
	if (TimeUntilNext <= 0.0f)
	{
		SpawnRandomEvent();
		TimeUntilNext = FMath::FRandRange(MinInterval, MaxInterval);
	}
}

void ASeaEventManager::SpawnRandomEvent()
{
	const FVector Loc = PickSpawnLocation();
	const int32 Roll = FMath::RandRange(0, 99);
	if      (Roll < 35) SpawnEvent_Wreckage(Loc);
	else if (Roll < 55) SpawnEvent_Fisherman(Loc);
	else if (Roll < 75) SpawnEvent_TradeConvoy(Loc);
	else                SpawnEvent_BottleMessage(Loc);
}

void ASeaEventManager::ForceSpawn(ESeaEventType Type)
{
	const FVector Loc = PickSpawnLocation();
	switch (Type)
	{
	case ESeaEventType::FloatingWreckage: SpawnEvent_Wreckage(Loc); break;
	case ESeaEventType::Fisherman:        SpawnEvent_Fisherman(Loc); break;
	case ESeaEventType::TradeConvoy:      SpawnEvent_TradeConvoy(Loc); break;
	case ESeaEventType::BottleMessage:    SpawnEvent_BottleMessage(Loc); break;
	}
}

FVector ASeaEventManager::PickSpawnLocation() const
{
	UWorld* W = GetWorld();
	if (!W) return FVector::ZeroVector;
	APawn* Player = UGameplayStatics::GetPlayerPawn(W, 0);
	if (!Player) return GetActorLocation();

	const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
	const float Dist  = FMath::FRandRange(MinSpawnDistance, MaxSpawnDistance);
	const FVector PL = Player->GetActorLocation();
	return FVector(PL.X + FMath::Cos(Angle) * Dist,
	               PL.Y + FMath::Sin(Angle) * Dist,
	               SeaLevelZ);
}

void ASeaEventManager::SpawnEvent_Wreckage(const FVector& Loc)
{
	UWorld* W = GetWorld();
	if (!W || !WreckageLootClass) return;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	W->SpawnActor<ALootPickup>(WreckageLootClass, Loc, FRotator::ZeroRotator, Params);
	Toast(ESeaEventType::FloatingWreckage, Loc);
}

void ASeaEventManager::SpawnEvent_Fisherman(const FVector& Loc)
{
	UWorld* W = GetWorld();
	if (!W || !FishermanShipClass) return;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	W->SpawnActor<AEnemyShipBase>(FishermanShipClass, Loc, FRotator::ZeroRotator, Params);
	Toast(ESeaEventType::Fisherman, Loc);
}

void ASeaEventManager::SpawnEvent_TradeConvoy(const FVector& Loc)
{
	UWorld* W = GetWorld();
	if (!W || !ConvoyFlagshipClass) return;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// Flagship in center, 2 escorts flanking.
	W->SpawnActor<AEnemyShipBase>(ConvoyFlagshipClass, Loc, FRotator::ZeroRotator, Params);
	if (ConvoyEscortClass)
	{
		W->SpawnActor<AEnemyShipBase>(ConvoyEscortClass, Loc + FVector(800, 600, 0),
			FRotator::ZeroRotator, Params);
		W->SpawnActor<AEnemyShipBase>(ConvoyEscortClass, Loc + FVector(800, -600, 0),
			FRotator::ZeroRotator, Params);
	}
	Toast(ESeaEventType::TradeConvoy, Loc);
}

void ASeaEventManager::SpawnEvent_BottleMessage(const FVector& Loc)
{
	UWorld* W = GetWorld();
	if (!W) return;
	// Trigger a new treasure quest if a TreasureQuestManager exists.
	for (TActorIterator<ATreasureQuestManager> It(W); It; ++It)
	{
		if (ATreasureQuestManager* QM = *It)
		{
			QM->IssueQuest(Loc);
			Toast(ESeaEventType::BottleMessage, Loc);
			return;
		}
	}
}

void ASeaEventManager::Toast(ESeaEventType Type, const FVector& Loc) const
{
	UWorld* W = GetWorld();
	if (!W) return;
	APawn* Player = UGameplayStatics::GetPlayerPawn(W, 0);
	const FVector PL = Player ? Player->GetActorLocation() : FVector::ZeroVector;

	// Compass bearing from player to event.
	const FVector Dir = (Loc - PL).GetSafeNormal2D();
	const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X));
	const float B = FMath::Fmod(Yaw + 360.f, 360.f);
	FString Cardinal =
		(B < 22.5f  || B >= 337.5f) ? TEXT("east") :
		(B < 67.5f)                 ? TEXT("south-east") :
		(B < 112.5f)                ? TEXT("south") :
		(B < 157.5f)                ? TEXT("south-west") :
		(B < 202.5f)                ? TEXT("west") :
		(B < 247.5f)                ? TEXT("north-west") :
		(B < 292.5f)                ? TEXT("north") :
		                              TEXT("north-east");

	const TCHAR* TypeName = TEXT("Event");
	FLinearColor Tint(1.0f, 1.0f, 1.0f, 1.0f);
	switch (Type)
	{
	case ESeaEventType::FloatingWreckage: TypeName = TEXT("⚓ Floating wreckage"); Tint = FLinearColor(0.9f, 0.7f, 0.3f, 1.f); break;
	case ESeaEventType::Fisherman:        TypeName = TEXT("⚓ Fisherman ship");    Tint = FLinearColor(0.7f, 0.9f, 1.0f, 1.f); break;
	case ESeaEventType::TradeConvoy:      TypeName = TEXT("⚓ Trade convoy");      Tint = FLinearColor(1.0f, 0.5f, 0.2f, 1.f); break;
	case ESeaEventType::BottleMessage:    TypeName = TEXT("⚓ Message in bottle");  Tint = FLinearColor(0.6f, 1.0f, 0.6f, 1.f); break;
	}

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(W, 0))
	{
		if (ACrownsBaneHUD* HUD = Cast<ACrownsBaneHUD>(PC->GetHUD()))
		{
			HUD->PushResourceToast(FString::Printf(TEXT("%s spotted %s"), TypeName, *Cardinal), Tint);
		}
	}
}
