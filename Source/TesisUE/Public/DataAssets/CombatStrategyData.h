// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CombatStrategyData.generated.h"

/**
 * 
 */

enum EInputCommandType : uint8;
class UCommand;
class UCombatStrategy;

USTRUCT(BlueprintType)
struct FStrategyProperties
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float JumpForce;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DoubleJumpForce;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxWalkSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<TEnumAsByte<EInputCommandType>, TSubclassOf<UCommand>> CommandsClasses;
};

USTRUCT(BlueprintType)
struct FCharacterModeConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	int32 TeamID;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UCombatStrategy> StrategyClass;

	UPROPERTY(EditDefaultsOnly)
	bool bShowWeapon;
    
	UPROPERTY(EditDefaultsOnly)
	bool bUseControllerRotationYaw;
};

UCLASS(Blueprintable)
class TESISUE_API UCombatStrategyData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FStrategyProperties StrategyProperties;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FCharacterModeConfig CharacterModeConfig;
};
