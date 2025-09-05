// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/EnergyItem.h"
#include "UObject/Interface.h"
#include "AttributeProvider.generated.h"

class UAttributeComponent;
// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UAttributeProvider : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class TESISUE_API IAttributeProvider
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual bool RequiresEnergy(float X) = 0;
	virtual void SetEnergy(float EnergyFromPossessor) = 0;
	// virtual void StartDecreaseEnergy() = 0;
	virtual void IncreaseHealth(float X) = 0;
	virtual void IncreaseEnergy(float EnergyToIncrease) = 0;
};
