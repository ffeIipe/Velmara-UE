// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/CombatComponent.h"
#include "UObject/Interface.h"
#include "WeaponInterface.generated.h"

class UMeleeDamage;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponUsed);
// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
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

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void UsePrimaryAttack();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void UseLaunchAttack();
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void UseSecondaryAttack();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void UseAbilityAttack();
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void ResetWeapon();
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void SetDamageType(TSubclassOf<UMeleeDamage> DamageType);
	
	virtual void EnableVisuals() = 0;
	virtual void DisableVisuals() = 0;
	virtual void SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled) = 0;
	virtual void ClearIgnoreActors() = 0;
	virtual FName& GetUniqueSaveID() = 0;
	
	
};
