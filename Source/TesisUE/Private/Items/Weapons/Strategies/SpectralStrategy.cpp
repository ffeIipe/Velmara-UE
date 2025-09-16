// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapons/Strategies/SpectralStrategy.h"

#include "Components/PossessionComponent.h"

void USpectralStrategy::Strategy_UseFirstAttack(const bool bIsInAir, const TScriptInterface<IWeaponInterface> CurrentWeapon)
{
	Super::Strategy_UseFirstAttack(bIsInAir, CurrentWeapon);
	
}

void USpectralStrategy::Strategy_UseSecondAttack(const bool bIsInAir, const TScriptInterface<IWeaponInterface> CurrentWeapon)
{
	Super::Strategy_UseSecondAttack(bIsInAir, CurrentWeapon);
	
}

void USpectralStrategy::Strategy_UseAbility(AActor* Actor)
{
	Super::Strategy_UseAbility(Actor);

	if (UPossessionComponent* PossessionComponent = Actor->FindComponentByClass<UPossessionComponent>())
	{
		PossessionComponent->AttemptPossession(
		PossessionComponent->FindPossessionVictim(StrategyProperties.MaxAbilityDistance,
												  StrategyProperties.MaxAbilityRadius));
	}
}
