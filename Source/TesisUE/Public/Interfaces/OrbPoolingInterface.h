// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "OrbPoolingInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UOrbPoolingInterface : public UInterface
{
	GENERATED_BODY()
};

class TESISUE_API IOrbPoolingInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintNativeEvent)
	void ActivateOrb(const FVector& Location, const FRotator& Rotation);
	
	UFUNCTION(BlueprintNativeEvent)
	void DeactivateOrb();
};
