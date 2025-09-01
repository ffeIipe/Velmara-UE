// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CharacterStateProvider.generated.h"

struct FCharacterStates;
class UCharacterStateComponent;
enum class ECharacterHumanStates : uint8;
enum class ECharacterSpectralStates : uint8;
enum class ECharacterActions : uint8;
enum class ECharacterMode : uint8;

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
	virtual ECharacterHumanStates SetHumanState(ECharacterHumanStates NewState) = 0;
	virtual ECharacterSpectralStates SetSpectralState(ECharacterSpectralStates NewSpectralState) = 0;
	virtual ECharacterActions SetAction(ECharacterActions NewAction) = 0;
	virtual ECharacterMode SetMode(ECharacterMode NewForm) = 0;
	virtual bool IsHumanStateEqualToAny(const TArray<ECharacterHumanStates>& StatesToCheck) const = 0;
	virtual bool IsSpectralStateEqualToAny(const TArray<ECharacterSpectralStates>& SpectralStatesToCheck) const = 0;
	virtual bool IsActionEqualToAny(const TArray<ECharacterActions>& ActionsToCheck) const = 0;
	virtual bool IsModeEqualToAny(const TArray<ECharacterMode>& FormsToCheck) const = 0;
};
