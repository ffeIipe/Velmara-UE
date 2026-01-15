#include "GAS/AbilityTasks/UAbilityTask_PlayCurve.h"

UAbilityTask_PlayCurve::UAbilityTask_PlayCurve()
{
	bTickingTask = true;
}

UAbilityTask_PlayCurve* UAbilityTask_PlayCurve::PlayCurve(UGameplayAbility* OwningAbility, UCurveFloat* CurveAsset, float DurationMultiplier)
{
	UAbilityTask_PlayCurve* MyTask = NewAbilityTask<UAbilityTask_PlayCurve>(OwningAbility);
	MyTask->CurveFloat = CurveAsset;
	MyTask->DurationMultiplier = DurationMultiplier;
	return MyTask;
}

void UAbilityTask_PlayCurve::Activate()
{
	Super::Activate();

	CurrentTime = 0.0f;
    
	if (CurveFloat)
	{
		float MinTime;
		CurveFloat->GetTimeRange(MinTime, MaxTime);
	}
	else
	{
		MaxTime = 1.0f;
	}
}

void UAbilityTask_PlayCurve::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		CurrentTime += DeltaTime * DurationMultiplier;

		float CurrentValue = 0.0f;
		if (CurveFloat)
		{
			CurrentValue = CurveFloat->GetFloatValue(CurrentTime);
		}
		else
		{
			CurrentValue = FMath::Clamp(CurrentTime / MaxTime, 0.0f, 1.0f);
		}

		OnUpdate.Broadcast(CurrentValue, CurrentTime);

		if (CurrentTime >= MaxTime)
		{
			OnFinished.Broadcast(1.0f, MaxTime);
			EndTask();
		}
	}
}

void UAbilityTask_PlayCurve::OnDestroy(bool bInOwnerFinished)
{
	Super::OnDestroy(bInOwnerFinished);
}