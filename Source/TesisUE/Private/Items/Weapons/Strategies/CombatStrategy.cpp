// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapons/Strategies/CombatStrategy.h"

#include "Components/ExtraMovementComponent.h"
#include "Components/TargetingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

void UCombatStrategy::SetCurrentValues(UExtraMovementComponent* ExtraMoveComp, UCharacterMovementComponent* MoveComp, UTargetingComponent* TargetingComp) const
{
	if (StrategyMontages.StaticStruct() && StrategyProperties.StaticStruct())
	{
		ExtraMoveComp->SetCurrentStrategyValues(
		StrategyProperties.MaxDodgeDistance,
		StrategyProperties.DoubleJumpForce,
		StrategyMontages.DodgeMontage
		);

		MoveComp->MaxWalkSpeed = StrategyProperties.MaxWalkSpeed;
		MoveComp->JumpZVelocity = StrategyProperties.JumpForce;

		TargetingComp->SetCurrentStrategyValues(StrategyProperties.MaxAbilityDistance, StrategyProperties.MaxAbilityRadius);
	}
	else if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "MISSING! Combat strategy properties.");
	}
}
