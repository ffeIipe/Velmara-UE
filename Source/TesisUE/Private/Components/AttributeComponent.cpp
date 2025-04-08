// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/AttributeComponent.h"

UAttributeComponent::UAttributeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

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

void UAttributeComponent::IncreaseEnergy(float Amount)
{
	Energy = FMathf::Clamp(Energy + Amount, 0.f, 100.f);
}

void UAttributeComponent::StartDecreaseEnergy()
{
	if (ItHasEnergy() && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 1.f, FColor::Blue, FString::SanitizeFloat(Energy));
		Energy = FMath::Clamp(Energy - 1.f, 0.f, 100.f);
	}
	else
	{
		if (OnDepletedCallback)
		{
			OnDepletedCallback(); //this exec the callback!!!
		}
		StopDecreaseEnergy();
	}
}

void UAttributeComponent::StopDecreaseEnergy()
{
	OnDepletedCallback;
	GetWorld()->GetTimerManager().ClearTimer(EnergyTimerHandle);
}

bool UAttributeComponent::ItHasEnergy()
{
	return Energy > 0.f;
}

void UAttributeComponent::BeginPlay()
{
	Super::BeginPlay();

	GetWorld()->GetTimerManager().SetTimer(EnergyTimerHandle, this, &UAttributeComponent::StartDecreaseEnergy, 1.0f, true);
}

void UAttributeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}