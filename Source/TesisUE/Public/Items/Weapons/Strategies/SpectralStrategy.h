// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CombatStrategy.h"
#include "SpectralStrategy.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class TESISUE_API USpectralStrategy : public UCombatStrategy
{
	GENERATED_BODY()

public:
	virtual void Strategy_UseFirstAttack(bool bIsInAir, TScriptInterface<IWeaponInterface> CurrentWeapon) override;
	virtual void Strategy_UseSecondAttack(bool bIsInAir, TScriptInterface<IWeaponInterface> CurrentWeapon) override;
	virtual void Strategy_UseAbility(AActor* Actor) override;
	virtual void Strategy_Dodge(AActor* Actor) override;
};
