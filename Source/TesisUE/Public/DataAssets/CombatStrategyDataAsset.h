// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CombatStrategyDataAsset.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FStrategyProperties
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float JumpForce = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DoubleJumpForce = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxWalkSpeed = 500.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName FirstCommandName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SecondCommandName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName AbilityCommandName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName DodgeCommandName;
};

UCLASS(Blueprintable)
class TESISUE_API UCombatStrategyData : public UDataAsset
{
	GENERATED_BODY()

	public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FStrategyProperties StrategyProperties;
};
