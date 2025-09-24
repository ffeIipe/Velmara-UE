// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Command.generated.h"

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
	void ExecuteCommand(AActor* User);

	//If not an instance, the command can reset stats
	UFUNCTION(BlueprintNativeEvent)
	void ResetCommand();

	//Shut down the command, and if it is an instance, it will be destroyed.
	UFUNCTION(BlueprintNativeEvent)
	void TerminateCommand();

	bool bIsInstance = false;
};
