#include "Components/CharacterStateComponent.h"

UCharacterStateComponent::UCharacterStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	CurrentStates.HumanState = ECharacterHumanStates::ECHS_Unequipped;
	CurrentStates.SpectralState = ECharacterSpectralStates::ECSS_Unequipped;
	CurrentStates.Action = ECharacterActions::ECA_Nothing;
	CurrentStates.Mode = ECharacterMode::ECM_Human;
}

ECharacterHumanStates UCharacterStateComponent::SetHumanState(ECharacterHumanStates NewState)
{
	CurrentStates.HumanState = NewState;
	return CurrentStates.HumanState;
}

ECharacterSpectralStates UCharacterStateComponent::SetSpectralState(ECharacterSpectralStates NewSpectralState)
{
	CurrentStates.SpectralState = NewSpectralState;

	return CurrentStates.SpectralState;
}

ECharacterActions UCharacterStateComponent::SetAction(ECharacterActions NewAction)
{
	CurrentStates.Action = NewAction;

	return CurrentStates.Action;
}

ECharacterMode UCharacterStateComponent::SetMode(ECharacterMode NewForm)
{
	CurrentStates.Mode = NewForm;

	return CurrentStates.Mode;
}

bool UCharacterStateComponent::IsHumanStateEqualToAny(const TArray<ECharacterHumanStates>& StatesToCheck) const
{
	return StatesToCheck.Contains(CurrentStates.HumanState);
}

bool UCharacterStateComponent::IsSpectralStateEqualToAny(const TArray<ECharacterSpectralStates>& SpectralStatesToCheck) const
{
	return SpectralStatesToCheck.Contains(CurrentStates.SpectralState);
}

bool UCharacterStateComponent::IsActionEqualToAny(const TArray<ECharacterActions>& ActionsToCheck) const
{
	return ActionsToCheck.Contains(CurrentStates.Action);
}

bool UCharacterStateComponent::IsModeEqualToAny(const TArray<ECharacterMode>& FormsToCheck) const
{
	return FormsToCheck.Contains(CurrentStates.Mode);
}