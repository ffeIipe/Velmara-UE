#include "GAS/AbilityTasks/AbilityTask_WaitComboInput.h"

#include "AbilitySystemComponent.h"
#include "TimerManager.h"

bool FComboInputState::Press()
{
	if (bPressed) return false;

	bPressed = true;
	return true;
}

void FComboInputState::Release()
{
	if (!bPressed || bResolved) return;

	bResolved = true;
	bIsHold = false;
}

void FComboInputState::HoldReached()
{
	if (!bPressed || bResolved) return;

	bResolved = true;
	bIsHold = true;
}

FComboInputState::EResult FComboInputState::Evaluate()
{
	if (!bPressed || !bWindowReached) return EResult::Nothing;
	if (bResolved) return EResult::Chain;
	if (bPendingSent) return EResult::Nothing;

	bPendingSent = true;
	return EResult::ChainPending;
}

UAbilityTask_WaitComboInput* UAbilityTask_WaitComboInput::WaitComboInput(UGameplayAbility* OwningAbility, const FGameplayTag WindowTag, const float HoldThreshold)
{
	UAbilityTask_WaitComboInput* MyTask = NewAbilityTask<UAbilityTask_WaitComboInput>(OwningAbility);
	MyTask->WindowTag = WindowTag;
	MyTask->HoldThreshold = FMath::Max(HoldThreshold, KINDA_SMALL_NUMBER);
	return MyTask;
}

void UAbilityTask_WaitComboInput::Activate()
{
	Super::Activate();

	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (!ASC || !Ability) return;

	PressedHandle = ASC->AbilityReplicatedEventDelegate(EAbilityGenericReplicatedEvent::InputPressed, GetAbilitySpecHandle(), GetActivationPredictionKey())
		.AddUObject(this, &UAbilityTask_WaitComboInput::OnInputPressed);
	ReleasedHandle = ASC->AbilityReplicatedEventDelegate(EAbilityGenericReplicatedEvent::InputReleased, GetAbilitySpecHandle(), GetActivationPredictionKey())
		.AddUObject(this, &UAbilityTask_WaitComboInput::OnInputReleased);

	if (!WindowTag.IsValid())
	{
		State.OpenWindow();
		return;
	}

	WindowTagCount = ASC->GetTagCount(WindowTag);
	WindowTagHandle = ASC->RegisterGameplayTagEvent(WindowTag, EGameplayTagEventType::AnyCountChange)
		.AddUObject(this, &UAbilityTask_WaitComboInput::OnWindowTagCountChanged);
}

void UAbilityTask_WaitComboInput::OnInputPressed()
{
	if (!State.Press()) return;

	if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
	{
		ASC->ConsumeGenericReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, GetAbilitySpecHandle(), GetActivationPredictionKey());
	}

	GetWorld()->GetTimerManager().SetTimer(HoldTimerHandle, this, &UAbilityTask_WaitComboInput::OnHoldThresholdReached, HoldThreshold, false);

	HandleState();
}

void UAbilityTask_WaitComboInput::OnInputReleased()
{
	if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
	{
		ASC->ConsumeGenericReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, GetAbilitySpecHandle(), GetActivationPredictionKey());
	}

	GetWorld()->GetTimerManager().ClearTimer(HoldTimerHandle);
	State.Release();

	HandleState();
}

void UAbilityTask_WaitComboInput::OnHoldThresholdReached()
{
	State.HoldReached();

	HandleState();
}

void UAbilityTask_WaitComboInput::OnWindowTagCountChanged(const FGameplayTag Tag, const int32 NewCount)
{
	if (NewCount > WindowTagCount)
	{
		State.OpenWindow();
	}
	WindowTagCount = NewCount;

	HandleState();
}

void UAbilityTask_WaitComboInput::HandleState()
{
	if (!ShouldBroadcastAbilityTaskDelegates()) return;

	switch (State.Evaluate())
	{
	case FComboInputState::EResult::ChainPending:
		OnChainPending.Broadcast(false);
		break;
	case FComboInputState::EResult::Chain:
		OnComboInput.Broadcast(State.bIsHold);
		EndTask();
		break;
	default:
		break;
	}
}

void UAbilityTask_WaitComboInput::OnDestroy(const bool bInOwnerFinished)
{
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HoldTimerHandle);
	}

	if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
	{
		ASC->AbilityReplicatedEventDelegate(EAbilityGenericReplicatedEvent::InputPressed, GetAbilitySpecHandle(), GetActivationPredictionKey()).Remove(PressedHandle);
		ASC->AbilityReplicatedEventDelegate(EAbilityGenericReplicatedEvent::InputReleased, GetAbilitySpecHandle(), GetActivationPredictionKey()).Remove(ReleasedHandle);

		if (WindowTagHandle.IsValid())
		{
			ASC->RegisterGameplayTagEvent(WindowTag, EGameplayTagEventType::AnyCountChange).Remove(WindowTagHandle);
		}
	}

	Super::OnDestroy(bInOwnerFinished);
}
