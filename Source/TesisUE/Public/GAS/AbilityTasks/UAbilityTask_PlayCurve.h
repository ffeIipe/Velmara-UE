#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "UObject/ObjectMacros.h"
#include "Curves/CurveFloat.h"
#include "UAbilityTask_PlayCurve.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPlayCurveDelegate, float, CurrentValue, float, CurrentTime);

UCLASS()
class UAbilityTask_PlayCurve : public UAbilityTask
{
	GENERATED_BODY()

public:
	UAbilityTask_PlayCurve();

	UFUNCTION(BlueprintCallable, Category="Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UAbilityTask_PlayCurve* PlayCurve(UGameplayAbility* OwningAbility, UCurveFloat* CurveAsset, float DurationMultiplier = 1.0f, float UpdateInterval = 1.0f, bool bReverse = false);

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

	UPROPERTY(BlueprintAssignable)
	FPlayCurveDelegate OnUpdate;

	UPROPERTY(BlueprintAssignable)
	FPlayCurveDelegate OnFinished;

protected:
	UPROPERTY()
	UCurveFloat* CurveFloat;

	float DurationMultiplier;
	float UpdateInterval;
	bool bReverse;

	float MaxTime;
	float CurrentTime;
	float TimeSinceLastUpdate;
};