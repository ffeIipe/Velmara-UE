// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "InputData.generated.h"

/**
 * 
 */

class UInputAction;

USTRUCT(BlueprintType)
struct FInputs
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Input | Movement")
	UInputAction* InputAction_Move;

	UPROPERTY(EditDefaultsOnly, Category = "Input | Movement")
	UInputAction* InputAction_Look;

	UPROPERTY(EditDefaultsOnly, Category = "Input | Movement")
	UInputAction* InputAction_Crouch;

	UPROPERTY(EditDefaultsOnly, Category = "Input | Movement")
	UInputAction* InputAction_Jump;

	UPROPERTY(EditDefaultsOnly, Category = "Input | Movement")
	UInputAction* InputAction_Dodge;

	UPROPERTY(EditDefaultsOnly, Category = "Input | Actions")
	UInputAction* InputAction_Interact;

	UPROPERTY(EditDefaultsOnly, Category = "Input | Actions")
	UInputAction* InputAction_SwitchForm;

	UPROPERTY(EditDefaultsOnly, Category = "Input | Actions")
	UInputAction* InputAction_Possess;

	UPROPERTY(EditDefaultsOnly, Category = "Input | Actions")
	UInputAction* InputAction_Inventory;

	UPROPERTY(EditDefaultsOnly, Category = "Input | Actions")
	UInputAction* InputAction_Attack;

	UPROPERTY(EditDefaultsOnly, Category = "Input | Actions")
	UInputAction* InputAction_HeavyAttack;

	UPROPERTY(EditDefaultsOnly, Category = "Input | Actions")
	UInputAction* InputAction_Launch;

	UPROPERTY(EditDefaultsOnly, Category = "Input | Actions")
	UInputAction* InputAction_Block;

	UPROPERTY(EditDefaultsOnly, Category = "Input | Actions")
	UInputAction* InputAction_ChangeHardLockTarget;

	UPROPERTY(EditDefaultsOnly, Category = "Input | Actions")
	UInputAction* InputAction_ToggleWeapon;
};

UCLASS()
class TESISUE_API UInputData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inputs")
	FInputs Inputs;
};
