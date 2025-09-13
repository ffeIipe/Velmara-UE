// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/Weapons/Commands/Command.h"
#include "SingleCommand.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class TESISUE_API USingleCommand : public UCommand
{
	GENERATED_BODY()

	protected:
	virtual void ExecuteCommand(TScriptInterface<IAnimatorProvider> AnimatorProvider) override;
	virtual void ResetCommand() override {}
	
	UPROPERTY(EditAnywhere)
	UAnimMontage* MontageToPlay;
};
