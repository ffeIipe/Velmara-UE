// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/Weapons/Commands/Command.h"
#include "SingleCommand.generated.h"

/**
 * This class works with only one animation, so if you want to make a class with combo attacks,
 * you will have to use the "UComboCommand" class, instead of USingleCommand. 
 */

UCLASS(Blueprintable)
class TESISUE_API USingleCommand : public UCommand
{
	GENERATED_BODY()

	protected:
	virtual void ExecuteCommand_Implementation(AEntity* User) override;
	virtual void ResetCommand_Implementation() override;

	UPROPERTY(BlueprintReadWrite, Category = Montage)
	UAnimMontage* MontageToPlay;
};
