// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SpectralInteractable.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class USpectralInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class TESISUE_API ISpectralInteractable
{
	GENERATED_BODY()

public:
	virtual void SpectralInteract() = 0;
};
