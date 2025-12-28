#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

struct FVelmaraGameplayTags
{
	static const FVelmaraGameplayTags& Get() { return GameplayTags; };
	static void InitializeGameplayTags();

	FGameplayTag State_Dead;
	FGameplayTag State_Shielded;
	FGameplayTag State_Stunned;
	FGameplayTag State_Attacking;
	FGameplayTag State_Blocking;
	FGameplayTag State_Executing;
	FGameplayTag State_Dodging;
	FGameplayTag State_ComboWindow_First;
	FGameplayTag State_ComboWindow_Second;
	FGameplayTag Mode_Primary;
	FGameplayTag Mode_Secondary;
	FGameplayTag Damage;


protected:
	static FVelmaraGameplayTags GameplayTags;
};
