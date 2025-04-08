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

		GetWorld()->GetTimerManager().SetTimer(EnergyTimerHandle, this, &UAttributeComponent::DrainTick, 1.0f, true);
	}
}

void UAttributeComponent::StopDecreaseEnergy()
{
	if (OnDepletedCallback)
	{
		OnDepletedCallback();
	}

	GetWorld()->GetTimerManager().ClearTimer(EnergyTimerHandle);
	bIsDraining = false;
}

bool UAttributeComponent::ItHasEnergy()
{
	return Energy > 0.f;
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
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 1.f, FColor::Red, FString::SanitizeFloat(Energy));
		}
		Energy = FMath::Clamp(Energy - 1.f, 0.f, 100.f);
	}
	else
	{
		StopDecreaseEnergy();
	}
}