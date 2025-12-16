#include "Components/CharacterStateComponent.h"

UCharacterStateComponent::UCharacterStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	CharacterStates.State = ECharacterStates::ECS_Unequipped;
	CharacterStates.Action = ECharacterActions::ECA_Nothing;
	CharacterStates.Form = ECharacterForm::ECF_Human;
}

FCharacterStates UCharacterStateComponent::GetCurrentCharacterState()
{
	return CharacterStates;
}

ECharacterStates UCharacterStateComponent::SetCharacterState(ECharacterStates NewState)
{
	CharacterStates.State = NewState;

	return CharacterStates.State;
}

ECharacterSpectralStates UCharacterStateComponent::SetCharacterSpectralState(ECharacterSpectralStates NewSpectralState)
{
	CharacterStates.SpectralState = NewSpectralState;

	return CharacterStates.SpectralState;
}

ECharacterActions UCharacterStateComponent::SetCharacterAction(ECharacterActions NewAction)
{
	CharacterStates.Action = NewAction;

	return CharacterStates.Action;
}

ECharacterForm UCharacterStateComponent::SetCharacterForm(ECharacterForm NewForm)
{
	CharacterStates.Form = NewForm;

	return CharacterStates.Form;
}

bool UCharacterStateComponent::IsStateEqualToAny(const TArray<ECharacterStates>& StatesToCheck)
{
	return StatesToCheck.Contains(CharacterStates.State);
}

bool UCharacterStateComponent::IsSpectralStateEqualToAny(const TArray<ECharacterSpectralStates>& SpectralStatesToCheck)
{
	return SpectralStatesToCheck.Contains(CharacterStates.SpectralState);
}

bool UCharacterStateComponent::IsActionEqualToAny(const TArray<ECharacterActions>& ActionsToCheck)
{
	return ActionsToCheck.Contains(CharacterStates.Action);
}

bool UCharacterStateComponent::IsFormEqualToAny(const TArray<ECharacterForm>& FormsToCheck)
{
	return FormsToCheck.Contains(CharacterStates.Form);
}

void UCharacterStateComponent::BeginPlay()
{
	Super::BeginPlay();
}