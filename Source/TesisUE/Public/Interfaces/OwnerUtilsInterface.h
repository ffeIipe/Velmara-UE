// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "OwnerUtilsInterface.generated.h"

class ICombatTargetInterface;
// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UOwnerUtilsInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class TESISUE_API IOwnerUtilsInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual bool IsAlive() = 0;
	virtual bool IsFalling() = 0;
	virtual bool IsFlying() = 0;
	virtual bool IsMovingBackwards() = 0;
	virtual bool IsEquipped() = 0;
	virtual bool IsLocking() = 0;

	virtual TScriptInterface<ICombatTargetInterface> GetLastDamageCauser() = 0;
	virtual void SetMovementMode(EMovementMode MovementMode) = 0;
};
