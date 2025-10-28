// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AnimatorProvider.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
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
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	float PlayAnimMontage(UAnimMontage* Montage, float Rate = 1.f, FName Section = "Default");

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void StopAnimMontage(UAnimMontage* MontageToStop = nullptr);
	
	virtual USceneComponent* GetMeshComponent() = 0;
	virtual void PauseAnims(bool bEnable) = 0;
	virtual void ChangeWeaponAnimationState() = 0;
};
