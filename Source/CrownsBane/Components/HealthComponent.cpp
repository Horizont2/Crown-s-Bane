// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "Components/HealthComponent.h"
#include "GameFramework/Actor.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	CurrentHealth = MaxHealth;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;

	// Bind to the owner's damage event
	AActor* Owner = GetOwner();
	if (Owner)
	{
		Owner->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::HandleTakeAnyDamage);
	}
}

void UHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatedBy, AActor* DamageCauser)
{
	TakeDamage(Damage);
}

void UHealthComponent::TakeDamage(float DamageAmount)
{
	if (bIsDead || DamageAmount <= 0.0f)
	{
		return;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.0f, MaxHealth);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

	UE_LOG(LogTemp, Log, TEXT("HealthComponent: %s took %.1f damage. Health: %.1f/%.1f"),
		*GetOwner()->GetName(), DamageAmount, CurrentHealth, MaxHealth);

	if (CurrentHealth <= 0.0f)
	{
		bIsDead = true;
		UE_LOG(LogTemp, Log, TEXT("HealthComponent: %s has died."), *GetOwner()->GetName());
		OnDeath.Broadcast();
	}
}

void UHealthComponent::Heal(float HealAmount)
{
	if (bIsDead || HealAmount <= 0.0f)
	{
		return;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth + HealAmount, 0.0f, MaxHealth);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

	UE_LOG(LogTemp, Log, TEXT("HealthComponent: %s healed %.1f. Health: %.1f/%.1f"),
		*GetOwner()->GetName(), HealAmount, CurrentHealth, MaxHealth);
}

void UHealthComponent::FullHeal()
{
	if (bIsDead)
	{
		return;
	}
	CurrentHealth = MaxHealth;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void UHealthComponent::SetMaxHealth(float NewMaxHealth, bool bRefillHealth)
{
	MaxHealth = FMath::Max(1.0f, NewMaxHealth);
	if (bRefillHealth)
	{
		CurrentHealth = MaxHealth;
	}
	else
	{
		CurrentHealth = FMath::Clamp(CurrentHealth, 0.0f, MaxHealth);
	}
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void UHealthComponent::UpgradeMaxHealth(float BonusHealth)
{
	float OldMax = MaxHealth;
	MaxHealth += BonusHealth;
	// Also increase current health proportionally
	float HealthPercent = (OldMax > 0.0f) ? (CurrentHealth / OldMax) : 1.0f;
	CurrentHealth = MaxHealth * HealthPercent;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

	UE_LOG(LogTemp, Log, TEXT("HealthComponent: Max health upgraded to %.1f"), MaxHealth);
}

float UHealthComponent::GetHealthPercent() const
{
	if (MaxHealth <= 0.0f)
	{
		return 0.0f;
	}
	return CurrentHealth / MaxHealth;
}
