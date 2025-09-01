// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/Weapon/WeaponInterface.h"
#include "Items/Item.h"
#include "Weapon.generated.h"

/**
 * 
 */
UCLASS()
class TESISUE_API AWeapon : public AItem, public IWeaponInterface
{
	GENERATED_BODY()

public:
	virtual void UsePrimaryAttack() override {}
	
	virtual void UseSecondaryAttack() override {}
	
	virtual void Equip(USceneComponent* InParent, FName InSocketName, AActor* NewOwner, APawn* NewInstigator) override;

	virtual void Unequip() {}

	virtual void EnableVisuals(bool bEnable) override;

	virtual void SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled) override {}

	virtual void ClearIgnoreActors() override {}
};