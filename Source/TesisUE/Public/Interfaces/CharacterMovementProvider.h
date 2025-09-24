// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CharacterMovementProvider.generated.h"

class UCharacterMovementComponent;

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
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

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	UCharacterMovementComponent* GetCharacterMovementComponent();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	UExtraMovementComponent* GetExtraMovementComponent();
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	ACharacter* GetCharacter();
};
