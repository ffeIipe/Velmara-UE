#include "Components/AttributeComponent.h"

#include "DataAssets/EntityData.h"
#include "Entities/Entity.h"

UAttributeComponent::UAttributeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bIsDraining = false;

	ShieldMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShieldMeshComponent"));
	ShieldMeshComponent->SetCollisionProfileName(FName("Custom"));
	ShieldMeshComponent->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Block);
}

void UAttributeComponent::InitializeValues(const FAttributeData& AttributeData)
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE,3.f,FColor::White,TEXT("Attributes assigned by Data Asset"));

	Health = AttributeData.MaxHealth;

	if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE,3.f,FColor::White, FString::Printf(TEXT("Health: %f"), Health));
	MaxHealth = AttributeData.MaxHealth;
	CurrentShieldHealth = AttributeData.MaxShieldHealth;
	MaxShieldHealth = AttributeData.MaxShieldHealth;
	Energy = AttributeData.Energy;
	DrainTickValue = AttributeData.DrainTickValue;
	RegenerateTickValue = AttributeData.RegenerateTickValue;
}

void UAttributeComponent::ReceiveDamage(float Damage)
{
	if (!IsAlive()) return;

	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);

	if (!IsAlive() && OnEntityDead.IsBound())
	{
		OnEntityDead.Broadcast();
		//OnEntityDead.Clear();
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
		return false;
	}
	return true;
}

float UAttributeComponent::GetEnergyPercent()
{
	return Energy / 100.f;
}

void UAttributeComponent::IncreaseEnergy(float Amount)
{
	Energy += Amount;
}

bool UAttributeComponent::ItHasEnergy()
{
	return Energy > 1.f;
}

bool UAttributeComponent::ItHasFullEnergy()
{
	return Energy >= 99.9f;
}

void UAttributeComponent::AttachShield(USceneComponent* InParent, const FName SocketName)
{
	ShieldMeshComponent->SetupAttachment(InParent, SocketName);
}

void UAttributeComponent::DetachShield()
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
	DetachShield();
	return false;
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
		DetachShield();
	}
}