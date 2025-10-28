// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapons/Commands/ComboCommand.h"

#include "Interfaces/AnimatorProvider.h"
#include "Entities/Entity.h"


void UComboCommand::ExecuteCommand_Implementation(AEntity* User)
{
	Super::ExecuteCommand_Implementation(User);

	if (ComboMontages.IsEmpty())
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Orange, "Empty Combo Anim Montages: " + GetName());
		return;
	}
	
	if (const TScriptInterface<IAnimatorProvider> AnimatorProvider = User)
	{
		AnimatorProvider->Execute_PlayAnimMontage(User, ComboMontages[ComboIndex], 1.f, "Default");
		ComboIndex++;

		if (ComboIndex >= ComboMontages.Num())
		{
			OnComboFinished.ExecuteIfBound();
			ComboIndex = 0;
		}
	}

	//End event to call a function when the combo finishes.
}

void UComboCommand::ResetCommand_Implementation()
{
	Super::ResetCommand_Implementation();

	ComboIndex = 0;
}