// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WeaponInterface.generated.h"

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
	virtual void UsePrimaryAttack() = 0;
	virtual void UseSecondaryAttack() = 0;
	/*virtual void Pick(AActor* NewOwner) = 0;*/
	virtual void EnableVisuals(bool Enable) = 0;

	UFUNCTION(BlueprintNativeEvent)
	void SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled);
	virtual void ClearIgnoreActors() = 0;
	virtual FName& GetUniqueSaveID() = 0;
};
