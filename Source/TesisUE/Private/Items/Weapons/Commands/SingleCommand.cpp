// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapons/Commands/SingleCommand.h"

#include "Interfaces/AnimatorProvider.h"
#include "Entities/Entity.h"

void USingleCommand::ExecuteCommand_Implementation(AEntity* User)
{
	Super::ExecuteCommand_Implementation(User);

	if (!MontageToPlay)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "Invalid Combo Data Asset: " + GetName());
		return;
	}

	if (const TScriptInterface<IAnimatorProvider> AnimatorProvider = User)
	{
		AnimatorProvider->Execute_PlayAnimMontage(User, MontageToPlay, 1.f, "Default");
	}
}

void USingleCommand::ResetCommand_Implementation()
{
	Super::ResetCommand_Implementation();

	
}
