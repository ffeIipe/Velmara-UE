// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/AbilityTasks/AbilityTask_WaitInputHold.h"
#include "AbilitySystemComponent.h"
#include "TimerManager.h"

UAbilityTask_WaitInputHold* UAbilityTask_WaitInputHold::WaitInputHold(UGameplayAbility* OwningAbility, float HoldTime)
{
	UAbilityTask_WaitInputHold* Task = NewAbilityTask<UAbilityTask_WaitInputHold>(OwningAbility);
	Task->HoldDuration = HoldTime;
	return Task;
}

void UAbilityTask_WaitInputHold::Activate()
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (ASC && Ability)
	{
		FGameplayAbilitySpec* Spec = Ability->GetCurrentAbilitySpec();
		if (Spec && !Spec->InputPressed)
		{
			if (ShouldBroadcastAbilityTaskDelegates())
			{
				OnHoldCancelled.Broadcast();
			}
			EndTask();
			return;
		}

		DelegateHandle = ASC->AbilityReplicatedEventDelegate(
			EAbilityGenericReplicatedEvent::InputReleased, 
			GetAbilitySpecHandle(), 
			GetActivationPredictionKey()
		).AddUObject(this, &UAbilityTask_WaitInputHold::OnInputRelease);

		GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UAbilityTask_WaitInputHold::OnHoldFinished, HoldDuration, false);
	}
	else
	{
		EndTask();
	}
}

void UAbilityTask_WaitInputHold::OnInputRelease()
{
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle);

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnHoldCancelled.Broadcast();
	}
	EndTask();
}

void UAbilityTask_WaitInputHold::OnHoldFinished()
{
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnHoldCompleted.Broadcast();
	}
	EndTask();
}

void UAbilityTask_WaitInputHold::OnDestroy(bool bInOwnerFinished)
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (ASC && DelegateHandle.IsValid())
	{
		ASC->AbilityReplicatedEventDelegate(
			EAbilityGenericReplicatedEvent::InputReleased, 
			GetAbilitySpecHandle(), 
			GetActivationPredictionKey()
		).Remove(DelegateHandle);
	}

	GetWorld()->GetTimerManager().ClearTimer(TimerHandle);

	Super::OnDestroy(bInOwnerFinished);
}