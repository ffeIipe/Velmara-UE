// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Command.generated.h"

struct FInputActionValue;
DECLARE_DYNAMIC_DELEGATE(FOnComboFinished);

class AEntity;
class IAnimatorProvider;
/**
 * 
 */
UCLASS(Blueprintable)
class TESISUE_API UCommand : public UObject
{
	GENERATED_BODY()

public:
	//Start the Command
	UFUNCTION(BlueprintNativeEvent)
	void ExecuteCommand(AEntity* User);

	//If not an instance, the command can reset stats
	UFUNCTION(BlueprintNativeEvent)
	void ResetCommand();

	//Shut down the command, and if it is an instance, it will be destroyed.
	UFUNCTION(BlueprintNativeEvent)
	void TerminateCommand();
	
	UPROPERTY(BlueprintReadWrite)
	FOnComboFinished OnComboFinished;
	
	bool bIsInstance = false;
};
