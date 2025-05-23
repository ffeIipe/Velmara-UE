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
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
}

float UAttributeComponent::GetHealthPercent()
{
	return Health / MaxHealth;
}

bool UAttributeComponent::IsAlive()
{
	return Health > 0.01f;
}

float UAttributeComponent::GetEnergyPercent()
{
	return Energy / 100.f;
}

void UAttributeComponent::IncreaseEnergy(float Amount)
{
	Energy = FMath::Clamp(Energy + Amount, 0.f, 100.f);
}

void UAttributeComponent::StartDecreaseEnergy()
{
	if (!bIsDraining)
	{
		bIsDraining = true;

		GetWorld()->GetTimerManager().SetTimer(EnergyDecreaseTimerHandle, this, &UAttributeComponent::DrainTick, 1.0f, true);
	}
}

void UAttributeComponent::StopDecreaseEnergy()
{
	if (OnDepletedCallback)
	{
		OnDepletedCallback();
	}

	GetWorld()->GetTimerManager().ClearTimer(EnergyDecreaseTimerHandle);
	bIsDraining = false;
	RegenerateTick();
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
	if (ShieldMeshComponent)
	{
		ShieldMeshComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		ShieldMeshComponent->SetSimulatePhysics(true);
		ShieldMeshComponent->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
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

void UAttributeComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UAttributeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UAttributeComponent::DrainTick()
{
	if (ItHasEnergy())
	{
		Energy = FMath::Clamp(Energy - DrainTickValue, 1.f, 100.f);
	}
	else
	{
		StopDecreaseEnergy();
	}
}

void UAttributeComponent::RegenerateTick()
{
	if (!ItHasFullEnergy())
	{
		GetWorld()->GetTimerManager().SetTimer(EnergyRegenerateTimerHandle, this, &UAttributeComponent::RegenerateEnergy, 1.0f, true); //linea 94
	}
	else StopRegenerateTick();
}

void UAttributeComponent::StopRegenerateTick()
{
	GetWorld()->GetTimerManager().ClearTimer(EnergyRegenerateTimerHandle);
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
	CurrentShieldHealth = FMath::Clamp(CurrentShieldHealth - Damage, 0.f, MaxShieldHealth);

	if (CurrentShieldHealth <= 0.f)
	{
		DettachShield();
	}
}

void UAttributeComponent::RegenerateEnergy()
{
	if (!ItHasFullEnergy())
	{
		Energy = FMath::Clamp(Energy + RegenerateTickValue, 1.f, 100.f);
	}
}