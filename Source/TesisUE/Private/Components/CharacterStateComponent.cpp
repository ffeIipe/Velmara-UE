#include "Components/CharacterStateComponent.h"

UCharacterStateComponent::UCharacterStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	CurrentStates.WeaponState = ECharacterWeaponStates::ECWS_Unequipped;
	CurrentStates.Action = ECharacterActionsStates::ECAS_Nothing;
	CurrentStates.Mode = ECharacterModeStates::ECMS_Human;
}

ECharacterWeaponStates UCharacterStateComponent::SetHumanState(ECharacterWeaponStates NewState)
{
	CurrentStates.WeaponState = NewState;
	return CurrentStates.WeaponState;
}

ECharacterActionsStates UCharacterStateComponent::SetAction(ECharacterActionsStates NewAction)
{
	CurrentStates.Action = NewAction;

	return CurrentStates.Action;
}

ECharacterModeStates UCharacterStateComponent::SetMode(ECharacterModeStates NewForm)
{
	CurrentStates.Mode = NewForm;

	return CurrentStates.Mode;
}

bool UCharacterStateComponent::IsWeaponStateEqualToAny(const TArray<ECharacterWeaponStates>& StatesToCheck) const
{
	return StatesToCheck.Contains(CurrentStates.WeaponState);
}

bool UCharacterStateComponent::IsActionEqualToAny(const TArray<ECharacterActionsStates>& ActionsToCheck) const
{
	return ActionsToCheck.Contains(CurrentStates.Action);
}

bool UCharacterStateComponent::IsModeEqualToAny(const TArray<ECharacterModeStates>& FormsToCheck) const
{
	return FormsToCheck.Contains(CurrentStates.Mode);
}