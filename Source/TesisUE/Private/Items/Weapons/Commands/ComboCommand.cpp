// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapons/Commands/ComboCommand.h"

#include "Interfaces/AnimatorProvider.h"

void UComboCommand::ExecuteCommand(const TScriptInterface<IAnimatorProvider> AnimatorProvider)
{
	if (ComboMontages.IsEmpty())
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Orange, "Empty Combo Anim Montages: " + GetName());
		return;
	}
	
	if (AnimatorProvider)
	{
		AnimatorProvider->PlayAnimMontage(ComboMontages[ComboIndex]);
		ComboIndex++;

		if (ComboIndex >= ComboMontages.Num())
		{
			ComboIndex = 0;
		}
	}
}

void UComboCommand::ResetCommand()
{
	ComboIndex = 0;
}
