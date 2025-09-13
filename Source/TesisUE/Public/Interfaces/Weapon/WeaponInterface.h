// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/CombatComponent.h"
#include "UObject/Interface.h"
#include "WeaponInterface.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponUsed);
// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UWeaponInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class TESISUE_API IWeaponInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	FOnWeaponUsed OnWeaponUsed;
	
	virtual void UsePrimaryAttack(bool bIsInAir) = 0;
	virtual void UseSecondaryAttack(bool bIsInAir) = 0;

	virtual void EnableVisuals(bool Enable) = 0;
	
	virtual void SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled) = 0;
	virtual void ClearIgnoreActors() = 0;
	virtual FName& GetUniqueSaveID() = 0;
};
