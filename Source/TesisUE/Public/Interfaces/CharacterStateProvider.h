// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CharacterStateProvider.generated.h"

struct FCharacterStates;
enum class ECharacterWeaponStates : uint8;
enum class ECharacterActionsStates : uint8;
enum class ECharacterModeStates : uint8;

UINTERFACE(MinimalAPI)
class UCharacterStateProvider : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class TESISUE_API ICharacterStateProvider
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual const FCharacterStates& GetCurrentCharacterState() = 0;
	virtual ECharacterWeaponStates SetWeaponState(ECharacterWeaponStates NewState) = 0;
	virtual ECharacterActionsStates SetAction(ECharacterActionsStates NewAction) = 0;
	virtual ECharacterModeStates SetMode(ECharacterModeStates NewForm) = 0;
	virtual bool IsWeaponStateEqualToAny(const TArray<ECharacterWeaponStates>& StatesToCheck) const = 0;
	virtual bool IsActionStateEqualToAny(const TArray<ECharacterActionsStates>& ActionsToCheck) const = 0;
	virtual bool IsModeStateEqualToAny(const TArray<ECharacterModeStates>& FormsToCheck) const = 0;
};
