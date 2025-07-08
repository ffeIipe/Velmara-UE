#include "Components/AttributeComponent.h"

UAttributeComponent::UAttributeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bIsDraining = false;

	ShieldMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShieldMeshComponent"));
	ShieldMeshComponent->SetCollisionProfileName(FName("Custom"));
	ShieldMeshComponent->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Block);
	CurrentShieldHealth = MaxShieldHealth;
}

void UAttributeComponent::ReceiveDamage(float Damage)
{
	if (IsAlive())
	{
		Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
	}
}

float UAttributeComponent::GetHealthPercent()
{
	return Health / MaxHealth;
}

bool UAttributeComponent::IsAlive()
{
	if (Health <= 0)
	{
		if (OnEntityDead.IsBound())
		{
			OnEntityDead.Broadcast();
		}

		return false;
	}
	else
	{
		return true;
	}
}

float UAttributeComponent::GetEnergyPercent()
{
	return Energy / 100.f;
}

void UAttributeComponent::IncreaseEnergy(float Amount)
{
	Energy = FMath::Clamp(Energy + Amount, 0.f, 100.f);
}


bool UAttributeComponent::ItHasEnergy()
{
	return Energy > 1.f;
}

bool UAttributeComponent::ItHasFullEnergy()
{
	return Energy >= 99.9f;
}

void UAttributeComponent::AttachShield(USceneComponent* InParent, FName SocketName)
{
	ShieldMeshComponent->SetupAttachment(InParent, SocketName);
}

void UAttributeComponent::DettachShield()
{
	if (ShieldMeshComponent && !bIsDisarmed)
	{
		if (OnDettachShield.IsBound())
		{
			OnDettachShield.Broadcast();
		}

		ShieldMeshComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		ShieldMeshComponent->SetSimulatePhysics(true);
		ShieldMeshComponent->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
		//DissolveShield();
		bIsDisarmed = true;
	}
}

bool UAttributeComponent::IsShielded()
{
	if (CurrentShieldHealth > .01f)
	{
		bIsDisarmed = false;
		return true;
	}
	else
	{
		DettachShield();
		return false;
	}
}

void UAttributeComponent::ResetAttributes()
{
	Health = MaxHealth;
	CurrentShieldHealth = MaxShieldHealth;
	Energy = 5.f;
}

void UAttributeComponent::StartDecreaseEnergy()
{
	if (!bIsDraining)
	{
		StopRegenerateTick();

		bIsDraining = true;

		GetWorld()->GetTimerManager().SetTimer(EnergyDecreaseTimerHandle, this, &UAttributeComponent::DrainTick, .1f, true);
	}
}

void UAttributeComponent::StopDecreaseEnergy()
{
	GetWorld()->GetTimerManager().ClearTimer(EnergyDecreaseTimerHandle);
	bIsDraining = false;
}

void UAttributeComponent::DrainTick()
{
	if (ItHasEnergy())
	{
		DecreaseEnergyBy(DrainTickValue / 10.f);
	}
	else
	{
		StopDecreaseEnergy();

		if (OnOutOfEnergy.IsBound())
		{
			OnOutOfEnergy.Broadcast();
		}
	}
}

void UAttributeComponent::RegenerateTick()
{
	GetWorld()->GetTimerManager().SetTimer(EnergyRegenerateTimerHandle, this, &UAttributeComponent::RegenerateEnergy, .1f, true);
}

void UAttributeComponent::StopRegenerateTick()
{
	GetWorld()->GetTimerManager().ClearTimer(EnergyRegenerateTimerHandle);
}

void UAttributeComponent::RegenerateEnergy()
{
	if (!ItHasFullEnergy())
	{
		IncreaseEnergy(RegenerateTickValue / 10.f);
	}
	else StopRegenerateTick();
}

bool UAttributeComponent::RequiresEnergy(float EnergyRequired)
{
	return Energy >= EnergyRequired;
}

void UAttributeComponent::DecreaseEnergyBy(float EnergyToDecrease)
{
	Energy -= EnergyToDecrease;
}

void UAttributeComponent::ReceiveShieldDamage(float Damage)
{
	CurrentShieldHealth -= Damage;

	if (CurrentShieldHealth <= 0.f)
	{
		DettachShield();
	}
}