// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "EnvironmentalDamage.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum class EEnvironmentalDamageTypes : uint8
{
	EEDT_None			UMETA(DisplayName = "None"),
	EEDT_SpectralTrap	UMETA(DisplayName = "SpectralTrap")
};

UCLASS()
class TESISUE_API UEnvironmentalDamage : public UDamageType
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	EEnvironmentalDamageTypes EnvironmentalDamageType;
};
