// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/AttributeComponent.h"

UAttributeComponent::UAttributeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bIsDraining = false;
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
	return Health > 0.1f;
}

float UAttributeComponent::GetEnergyPercent()
{
	return Energy / 100.f;
}

void UAttributeComponent::IncreaseEnergy(float Amount)
{
	Energy = FMathf::Clamp(Energy + Amount, 0.f, 100.f);
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
}

bool UAttributeComponent::ItHasEnergy()
{
	return Energy > 0.f;
}

bool UAttributeComponent::ItHasFullEnergy()
{
	return Energy != 99.0f;
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
		Energy = FMath::Clamp(Energy - DrainTickValue, 0.f, 100.f);
	}
	else
	{
		StopDecreaseEnergy();
	}
}

void UAttributeComponent::RegenerateTick()
{
	if (ItHasFullEnergy() || !bIsDraining)
	{
		GetWorld()->GetTimerManager().SetTimer(EnergyRegenerateTimerHandle, this, &UAttributeComponent::RegenerateEnergy, 1.0f, true);
	}
	else
	{
		GetWorld()->GetTimerManager().ClearTimer(EnergyRegenerateTimerHandle);
	}
}

void UAttributeComponent::RegenerateEnergy()
{
	Energy = FMath::Clamp(Energy + RegenerateTickValue, 0.f, 100.f);
}
