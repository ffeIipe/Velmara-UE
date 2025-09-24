// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapons/Commands/Command.h"

void UCommand::ExecuteCommand_Implementation(AActor* User)
{
}

void UCommand::ResetCommand_Implementation()
{
}

void UCommand::TerminateCommand_Implementation()
{
	if (bIsInstance)
	{
		FinishDestroy();
	}
}
