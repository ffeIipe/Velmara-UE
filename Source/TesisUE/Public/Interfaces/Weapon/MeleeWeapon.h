// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/CombatComponent.h"
#include "UObject/Interface.h"
#include "MeleeWeapon.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UMeleeWeapon : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class TESISUE_API IMeleeWeapon
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	/*virtual void PerformAttack(TWeakObjectPtr<UCombatComponent> CombComp) = 0;*/
};
