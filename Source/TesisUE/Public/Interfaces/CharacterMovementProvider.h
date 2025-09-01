// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CharacterMovementProvider.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UCharacterMovementProvider : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class TESISUE_API ICharacterMovementProvider
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual FVector GetLastMovementInputVector() = 0;
	virtual FRotator GetControlRotation() = 0;
	virtual bool IsUsingControllerRotationYaw() = 0;
	virtual float GetMaxWalkSpeed() = 0;
	virtual void AddMovementInput(const FVector& Vector, double X) = 0;
	virtual void AddControllerPitchInput(double X) = 0;
	virtual void AddControllerYawInput(double X) = 0;
	virtual void LaunchCharacter(const FVector& Vector, bool bCond, bool bCond1) = 0;
	virtual FVector GetVelocity() = 0;
	virtual FVector GetCurrentAcceleration() = 0;
};
