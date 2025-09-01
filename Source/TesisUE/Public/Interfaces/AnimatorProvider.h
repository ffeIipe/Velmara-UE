// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AnimatorProvider.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UAnimatorProvider : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class TESISUE_API IAnimatorProvider
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual void PlayAnimMontage(UAnimMontage* Montage, float Rate = 1.f, FName Section = "Default") = 0;
	virtual void StopAnimMontage() = 0;
	virtual USceneComponent* GetMesh() = 0;
	virtual void PauseAnims(bool bEnable) = 0;
};
