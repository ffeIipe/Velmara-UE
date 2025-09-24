// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapons/Strategies/CombatStrategy.h"

#include "Components/ExtraMovementComponent.h"
#include "DataAssets/CombatStrategyDataAsset.h"
#include "DataTables/CommandDataTable.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interfaces/OwnerUtilsInterface.h"
#include "Interfaces/Weapon/WeaponInterface.h"
#include "Interfaces/Weapon/WeaponProvider.h"
#include "Items/Weapons/Commands/Command.h"

void UCombatStrategy::InitializeStrategy()
{
	if (bWasInitialized) return;

	if (!CombatStrategyData || !CommandsDataTable) return;
	
	bWasInitialized = true;
	
	if (const FCommandDataTable* FirstCommand = CommandsDataTable->FindRow<FCommandDataTable>(CombatStrategyData->StrategyProperties.FirstCommandName, TEXT("Command Name")))
	{
		FirstCommandClass = FirstCommand->CommandClass;
	}

	if (const FCommandDataTable* SecondCommand = CommandsDataTable->FindRow<FCommandDataTable>(CombatStrategyData->StrategyProperties.SecondCommandName, TEXT("Command Name")))
	{
		SecondCommandClass = SecondCommand->CommandClass;
	}

	if (const FCommandDataTable* AbilityCommand = CommandsDataTable->FindRow<FCommandDataTable>(CombatStrategyData->StrategyProperties.AbilityCommandName, TEXT("Command Name")))
	{
		AbilityCommandClass = AbilityCommand->CommandClass;
	}

	if (const FCommandDataTable* DodgeCommand = CommandsDataTable->FindRow<FCommandDataTable>(CombatStrategyData->StrategyProperties.DodgeCommandName, TEXT("Command Name")))
	{
		DodgeCommandClass = DodgeCommand->CommandClass;
	}
}

void UCombatStrategy::Strategy_UseFirstCommand(AActor* User)
{
	if (const TScriptInterface<IWeaponProvider> WeaponProvider = User; WeaponProvider->Execute_GetCurrentWeapon(User))
	{
		const TScriptInterface<IWeaponInterface> WeaponEquipped = WeaponProvider->Execute_GetCurrentWeapon(User);
		
		if (const TScriptInterface<IOwnerUtilsInterface> OwnerUtils = User; OwnerUtils->IsEquipped())
		{
			WeaponProvider->Execute_GetCurrentWeapon(User)->Execute_UsePrimaryAttack(WeaponEquipped.GetObject());
			return;
		}
	}

	if (!FirstCommandInstance)
	{
		if (FirstCommandClass)
		{
			FirstCommandInstance = NewObject<UCommand>(User, FirstCommandClass);
			FirstCommandInstance->ExecuteCommand(User);
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Red, "Not a valid First Command class in: " + User->GetName());
		}
	}
	else FirstCommandInstance->ExecuteCommand(User);
}

void UCombatStrategy::Strategy_UseSecondCommand(AActor* User)
{
	if (const TScriptInterface<IWeaponProvider> WeaponProvider = User; WeaponProvider->Execute_GetCurrentWeapon(User))
	{
		const TScriptInterface<IWeaponInterface> WeaponEquipped = WeaponProvider->Execute_GetCurrentWeapon(User);
		
		if (const TScriptInterface<IOwnerUtilsInterface> OwnerUtils = User; OwnerUtils->IsEquipped())
		{
			WeaponProvider->Execute_GetCurrentWeapon(User)->Execute_UseSecondaryAttack(WeaponEquipped.GetObject());
			return;
		}
	}
	
	if (!SecondCommandInstance)
	{
		if (SecondCommandClass)
		{
			SecondCommandInstance = NewObject<UCommand>(User, SecondCommandClass);
			SecondCommandInstance->ExecuteCommand(User);
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Red, "Not a valid Second Command class in: " + User->GetName());
		}
	}
	else SecondCommandInstance->ExecuteCommand(User);
}

void UCombatStrategy::Strategy_UseAbility(AActor* User)
{
	if (const TScriptInterface<IWeaponProvider> WeaponProvider = User; WeaponProvider->Execute_GetCurrentWeapon(User))
	{
		const TScriptInterface<IWeaponInterface> WeaponEquipped = WeaponProvider->Execute_GetCurrentWeapon(User);
		
		if (const TScriptInterface<IOwnerUtilsInterface> OwnerUtils = User; OwnerUtils->IsEquipped())
		{
			WeaponProvider->Execute_GetCurrentWeapon(User)->Execute_UseAbilityAttack(WeaponEquipped.GetObject());
			return;
		}
	}

	if (!AbilityCommandInstance)
	{
		if (AbilityCommandClass)
		{
			AbilityCommandInstance = NewObject<UCommand>(User, AbilityCommandClass);
			AbilityCommandInstance->ExecuteCommand(User);
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "Not a valid Ability Command class in: " + User->GetName());
		}
	}
}

void UCombatStrategy::Strategy_Dodge(AActor* User)
{
	if (!DodgeCommandInstance)
	{
		if (DodgeCommandClass)
		{
			DodgeCommandInstance = NewObject<UCommand>(User, DodgeCommandClass);
			DodgeCommandInstance->ExecuteCommand(User);
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Red, "MISSING! Dodge Command from: " + User->GetName());
		}
	}
	else DodgeCommandInstance->ExecuteCommand(User);
}

void UCombatStrategy::SetCurrentValues(const TObjectPtr<AEntity>& Entity) const
{
	if (CombatStrategyData->StrategyProperties.StaticStruct())
	{
		Entity->GetCharacterMovement()->MaxWalkSpeed = CombatStrategyData->StrategyProperties.MaxWalkSpeed;
		Entity->GetCharacterMovement()->JumpZVelocity = CombatStrategyData->StrategyProperties.JumpForce;

		Entity->Execute_GetExtraMovementComponent(Entity)->DoubleJumpForce = CombatStrategyData->StrategyProperties.DoubleJumpForce;
	}
	else if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "MISSING! Combat strategy properties.");
	}
}

void UCombatStrategy::ResetCommands()
{
	if (FirstCommandInstance)
	{
		FirstCommandInstance->ResetCommand();
	}
	else
	{
		SecondCommandInstance->ResetCommand();
	}
}