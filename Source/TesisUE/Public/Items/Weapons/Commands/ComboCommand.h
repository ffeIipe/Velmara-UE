// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Command.h"
#include "UObject/Object.h"
#include "ComboCommand.generated.h"

class IAnimatorProvider;
/**
 * 
 */
UCLASS(Blueprintable)
class TESISUE_API UComboCommand : public UCommand
{
	GENERATED_BODY()

	
protected:
	virtual void ExecuteCommand_Implementation(AEntity* User) override;
	virtual void ResetCommand_Implementation() override;
	
	int32 ComboIndex;

	UPROPERTY(EditAnywhere)
	TArray<UAnimMontage*> ComboMontages;
};
