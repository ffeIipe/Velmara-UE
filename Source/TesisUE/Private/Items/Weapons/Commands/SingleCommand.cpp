// Fill out your copyright notice in the Description page of Project Settings.


#include "SingleCommand.h"

#include "Interfaces/AnimatorProvider.h"

void USingleCommand::ExecuteCommand(TScriptInterface<IAnimatorProvider> AnimatorProvider)
{
	Super::ExecuteCommand(AnimatorProvider);

	if (!MontageToPlay)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "Invalid Combo Data Asset: " + GetName());
		return;
	}
	
	if (AnimatorProvider)
	{
		AnimatorProvider->PlayAnimMontage(MontageToPlay);
	}
}
