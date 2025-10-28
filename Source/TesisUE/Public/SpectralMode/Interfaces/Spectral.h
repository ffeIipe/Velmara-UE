// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Spectral.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class USpectral : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class TESISUE_API ISpectral
{
	GENERATED_BODY()

public:
	virtual void AddToSpectralObjects() = 0;
	virtual void ActivateVisiblity() = 0;
	virtual void DeactivateVisibility() = 0;
};
