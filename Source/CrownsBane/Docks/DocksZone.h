#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DocksZone.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class AUpgradeManager;

UENUM(BlueprintType)
enum class EDockType : uint8
{
	Merchant     UMETA(DisplayName = "Merchant Port"),       // best trade, neutral
	Naval        UMETA(DisplayName = "Naval Port"),          // heals + repairs, NO amnesty
	PirateHaven  UMETA(DisplayName = "Pirate Haven")         // cheap ammo, amnesty, expensive upgrades
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerEnterDocks);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerExitDocks);

UCLASS()
class CROWNSBANE_API ADocksZone : public AActor
{
	GENERATED_BODY()

public:
	ADocksZone();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* DocksVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* DocksMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Docks")
	AUpgradeManager* UpgradeManager;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Docks")
	EDockType DockType = EDockType::Merchant;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Docks")
	FString DockName = TEXT("Port");

	// Range around a Naval port within which patrol ships will spawn to chase
	// wanted players.  Only used when DockType == Naval.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Docks|Naval")
	float NavalPatrolRange = 8000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Docks|Naval")
	TSubclassOf<class AEnemyShipBase> NavalPatrolShipClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Docks|Naval")
	int32 PatrolShipsToSpawn = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Docks|Naval")
	float PatrolSpawnCooldown = 45.0f;

	float PatrolCooldownTimer = 0.0f;

	// Fraction of health to restore on docking (1.0 = full heal)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Docks")
	float HealFraction = 1.0f;

	// Safe zone radius - enemies will not spawn within this distance
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Docks")
	float SafeZoneRadius = 5000.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Docks")
	bool bPlayerInDocks = false;

	UPROPERTY(BlueprintAssignable, Category = "Docks")
	FOnPlayerEnterDocks OnPlayerEnterDocks;

	UPROPERTY(BlueprintAssignable, Category = "Docks")
	FOnPlayerExitDocks OnPlayerExitDocks;

	UFUNCTION(BlueprintPure, Category = "Docks")
	bool IsLocationInSafeZone(FVector Location) const;

protected:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	void HealPlayer(AActor* PlayerActor);
	void ResetWantedLevel();
	void NotifyPlayerController(bool bEntering);
};
