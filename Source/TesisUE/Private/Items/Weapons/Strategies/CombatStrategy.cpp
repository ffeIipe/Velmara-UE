#include "Items/Weapons/Strategies/CombatStrategy.h"
#include "DataAssets/CombatStrategyDataAsset.h"
#include "Items/Weapons/Commands/Command.h"


void UCombatStrategy::InitializeStrategy()
{
	if (!CombatStrategyData || bWasInitialized) return;
	
	bWasInitialized = true;

	if (CombatStrategyData->StrategyProperties.CommandsClasses.IsEmpty()) return;
	
	CommandsInstances.Empty();
	
	for (const auto& Pair : CombatStrategyData->StrategyProperties.CommandsClasses)
	{
		if (Pair.Key == ECT_None || !Pair.Value) continue;

		CommandsInstances.Add(Pair.Key, *NewObject<UCommand>(this, Pair.Value));
	}
}

void UCombatStrategy::Strategy_UseCommand(AEntity* User, const EInputCommandType& CommandType)
{
	if (CommandsInstances.Contains(CommandType))
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Blue, "Valid command.");
		CommandsInstances[CommandType]->ExecuteCommand(User);
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "Not valid command.");
	}
}

void UCombatStrategy::SetCurrentValues(const TObjectPtr<AEntity>& Entity) const
{
	/*if (CombatStrategyData->StrategyProperties.StaticStruct())
	{
		Entity->GetCharacterMovement()->MaxWalkSpeed = CombatStrategyData->StrategyProperties.MaxWalkSpeed;
		Entity->GetCharacterMovement()->JumpZVelocity = CombatStrategyData->StrategyProperties.JumpForce;

		Entity->Execute_GetExtraMovementComponent(Entity)->DoubleJumpForce = CombatStrategyData->StrategyProperties.DoubleJumpForce;
	}
	else if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "MISSING! Combat strategy properties.");
	}*/
}

void UCombatStrategy::ResetCommands()
{
	for (const auto& Pair : CommandsInstances)
	{
		Pair.Value->ResetCommand();
	}
}