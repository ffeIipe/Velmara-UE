// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/Item.h"
#include "SpectralWeaponItem.generated.h"

/**
 * 
 */
UCLASS()
class TESISUE_API ASpectralWeaponItem : public AItem
{
	GENERATED_BODY()
	
public:
	void Use(ACharacter* Character) override;

private:
	UPROPERTY(EditDefaultsOnly)
	UAnimMontage* EquipMontage;
};
