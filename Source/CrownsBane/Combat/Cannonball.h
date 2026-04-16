#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Combat/ProjectileTypes.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Cannonball.generated.h"

class UProjectileMovementComponent;
class USphereComponent;

UCLASS()
class CROWNSBANE_API ACannonball : public AActor
{
	GENERATED_BODY()

public:
	ACannonball();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// Initialize with data and instigator
	UFUNCTION(BlueprintCallable, Category = "Cannonball")
	void InitCannonball(const FCannonballData& InData, AActor* InInstigator);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UProjectileMovementComponent* ProjectileMovement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannonball")
	FCannonballData CannonballData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cannonball")
	float LifeSpan = 6.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Cannonball")
	AActor* OwnerInstigator;

protected:
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse,
		const FHitResult& Hit);

	void ApplySlowEffect(AActor* TargetActor);

	bool bHasHit = false;
};
