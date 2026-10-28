#include "GAS/Abilities/BloodSuckAbility.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameplayEffect.h"

UBloodSuckAbility::UBloodSuckAbility()
{
	SetAssetTags(FGameplayTagContainer(FGameplayTag::RequestGameplayTag(FName("Ability.BloodSuck"), false)));
}

void UBloodSuckAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!SuckMontage)
	{
		HandleMontageCompleted();
		return;
	}

	// Si la interrumpen (hit react, muerte) no cura: el cooldown se gasta igual.
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, SuckMontage);
	MontageTask->OnCompleted.AddDynamic(this, &UBloodSuckAbility::HandleMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &UBloodSuckAbility::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UBloodSuckAbility::HandleMontageInterrupted);
	MontageTask->ReadyForActivation();
}

void UBloodSuckAbility::HandleMontageCompleted()
{
	if (HealEffect)
	{
		BP_ApplyGameplayEffectToOwner(HealEffect);
	}
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UBloodSuckAbility::HandleMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
