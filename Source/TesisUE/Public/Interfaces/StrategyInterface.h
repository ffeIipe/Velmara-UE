// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "StrategyInterface.generated.h"

class UCombatStrategy;

// This class does not need to be modified.

UINTERFACE(MinimalAPI)
class UStrategyProvider : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class TESISUE_API IStrategyProvider
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual UCombatStrategy* GetCurrentStrategy() = 0;
};
