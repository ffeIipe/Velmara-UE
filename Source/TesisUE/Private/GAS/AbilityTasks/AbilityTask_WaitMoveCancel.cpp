#include "GAS/AbilityTasks/AbilityTask_WaitMoveCancel.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"

namespace
{
	TMap<TWeakObjectPtr<UGameplayAbility>, TWeakObjectPtr<UAbilityTask_WaitMoveCancel>> ActiveTasks;
}

UAbilityTask_WaitMoveCancel* UAbilityTask_WaitMoveCancel::WaitMoveCancel(UGameplayAbility* OwningAbility, const FGameplayTag WindowTag, const float InputThreshold)
{
	TWeakObjectPtr<UAbilityTask_WaitMoveCancel>& Slot = ActiveTasks.FindOrAdd(OwningAbility);
	if (Slot.IsValid())
	{
		Slot->EndTask();
	}

	UAbilityTask_WaitMoveCancel* MyTask = NewAbilityTask<UAbilityTask_WaitMoveCancel>(OwningAbility);
	MyTask->WindowTag = WindowTag;
	MyTask->InputThreshold = InputThreshold;
	Slot = MyTask;
	return MyTask;
}

void UAbilityTask_WaitMoveCancel::Activate()
{
	Super::Activate();

	bTickingTask = true;

	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (!ASC || !WindowTag.IsValid()) return;

	WindowTagHandle = ASC->RegisterGameplayTagEvent(WindowTag, EGameplayTagEventType::AnyCountChange)
		.AddUObject(this, &UAbilityTask_WaitMoveCancel::OnWindowTagCountChanged);
}

void UAbilityTask_WaitMoveCancel::OnWindowTagCountChanged(const FGameplayTag Tag, const int32 NewCount)
{
	bWindowOpened = NewCount > 0;
}

bool UAbilityTask_WaitMoveCancel::IsAttackInputHeld() const
{
	const UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	const FGameplayAbilitySpec* Spec = ASC ? ASC->FindAbilitySpecFromHandle(GetAbilitySpecHandle()) : nullptr;
	return Spec && Spec->InputPressed;
}

void UAbilityTask_WaitMoveCancel::TickTask(const float DeltaTime)
{
	Super::TickTask(DeltaTime);

	if (!bWindowOpened || IsAttackInputHeld()) return;

	const APawn* Pawn = Cast<APawn>(GetAvatarActor());
	if (!Pawn || Pawn->GetLastMovementInputVector().SizeSquared() < FMath::Square(InputThreshold)) return;

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnMoveInput.Broadcast();
	}
	EndTask();
}

void UAbilityTask_WaitMoveCancel::OnDestroy(const bool bInOwnerFinished)
{
	if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get(); ASC && WindowTagHandle.IsValid())
	{
		ASC->RegisterGameplayTagEvent(WindowTag, EGameplayTagEventType::AnyCountChange).Remove(WindowTagHandle);
	}

	Super::OnDestroy(bInOwnerFinished);
}
