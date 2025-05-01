// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Player/CharacterStates.h"
#include "CharacterStateComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TESISUE_API UCharacterStateComponent : public UActorComponent
{
	GENERATED_BODY()	

private:
	struct FCharacterStates
	{
		ECharacterStates State;
		ECharacterActions Action;
		ECharacterForm Form;
	};

	FCharacterStates CharacterStates;

public:
	UCharacterStateComponent();

	FCharacterStates GetCurrentCharacterState();

	ECharacterStates SetCharacterState(ECharacterStates NewState);

	ECharacterActions SetCharacterAction(ECharacterActions NewAction);

	ECharacterForm SetCharacterForm(ECharacterForm NewForm);

	bool IsActionEqualToAny(const TArray<ECharacterActions>& FormsToCheck);

	bool IsStateEqualToAny(const TArray<ECharacterStates>& StatesToCheck);

	bool IsFormEqualToAny(const TArray<ECharacterForm>& StatesToCheck);

protected:
	virtual void BeginPlay() override;
};//