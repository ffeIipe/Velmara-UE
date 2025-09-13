// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Command.generated.h"

class IAnimatorProvider;
/**
 * 
 */
UCLASS(Abstract)
class TESISUE_API UCommand : public UObject
{
	GENERATED_BODY()

public:
	virtual void ExecuteCommand(TScriptInterface<IAnimatorProvider> AnimatorProvider) {}
	virtual void ResetCommand() {}
};
