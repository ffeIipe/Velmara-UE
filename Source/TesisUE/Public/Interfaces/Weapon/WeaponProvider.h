// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WeaponProvider.generated.h"

class IWeaponInterface;
// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UWeaponProvider : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class TESISUE_API IWeaponProvider
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	TScriptInterface<IWeaponInterface> GetCurrentWeapon();
};
