// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapons/Strategies/HumanStrategy.h"

#include "Components/CombatComponent.h"
#include "Components/ExtraMovementComponent.h"
#include "Components/TargetingComponent.h"
#include "Interfaces/Weapon/WeaponInterface.h"


void UHumanStrategy::Strategy_UseFirstAttack(const bool bIsInAir, const TScriptInterface<IWeaponInterface> CurrentWeapon)
{
	Super::Strategy_UseFirstAttack(bIsInAir, CurrentWeapon);
	
	CurrentWeapon->UsePrimaryAttack(bIsInAir);
}

void UHumanStrategy::Strategy_UseSecondAttack(const bool bIsInAir, const TScriptInterface<IWeaponInterface> CurrentWeapon)
{
	Super::Strategy_UseSecondAttack(bIsInAir, CurrentWeapon);
	
	CurrentWeapon->UseSecondaryAttack(bIsInAir);
}

void UHumanStrategy::Strategy_UseAbility(AActor* User)
{
	Super::Strategy_UseAbility(User);

	if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Green, FString("Debug purposes: From " + GetName() + " executed."));
	
	FVector StartLocation;
	FRotator ViewRotation;
	GetWorld()->GetFirstPlayerController()->GetPlayerViewPoint(StartLocation, ViewRotation);

	const FVector TraceStart = StartLocation;
	const FVector TraceEnd = TraceStart + ViewRotation.Vector() * StrategyProperties.MaxAbilityDistance;

	const TScriptInterface<ICombatTargetInterface> Target = User->FindComponentByClass<UTargetingComponent>()->SearchCombatTarget(
		TraceStart,
		TraceEnd,
		StrategyProperties.MaxAbilityRadius
		);
	
	if (Target)
	{
		User->FindComponentByClass<UCombatComponent>()->PerformExecute(Target, StrategyMontages.FinisherMontage);
	}
	else if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Green, FString("Debug purposes: Not a valid target"));
}

void UHumanStrategy::Strategy_Dodge(AActor* Actor)
{
	Super::Strategy_Dodge(Actor);

	Actor->GetComponentByClass<UExtraMovementComponent>()->PerformDodge(StrategyProperties.MaxDodgeDistance,
	                                                                    StrategyMontages.DodgeMontage);
}
