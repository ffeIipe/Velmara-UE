// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Components/MementoComponent.h"
#include "MementoEntity.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UMementoEntity : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class TESISUE_API IMementoEntity
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent)
	UMementoComponent* GetMementoComponent();
};
