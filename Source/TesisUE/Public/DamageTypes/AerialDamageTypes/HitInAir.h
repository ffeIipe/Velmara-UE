// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DamageTypes/DamageTypeMain.h"
#include "HitInAir.generated.h"

/**
 * 
 */
UCLASS()
class TESISUE_API UHitInAir : public UDamageTypeMain
{
	GENERATED_BODY()
	
public:
	EMainDamageTypes DamageType = EMainDamageTypes::EMDT_InAir;
};
