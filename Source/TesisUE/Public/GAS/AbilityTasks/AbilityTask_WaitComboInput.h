#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "GameplayTagContainer.h"
#include "AbilityTask_WaitComboInput.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FComboInputDelegate, bool, bIsHold);

struct FComboInputState
{
	enum class EResult : uint8 { Nothing, ChainPending, Chain };

	bool bPressed = false;
	bool bResolved = false;
	bool bIsHold = false;
	bool bWindowReached = false;
	bool bPendingSent = false;

	bool Press();
	void Release();
	void HoldReached();
	void OpenWindow() { bWindowReached = true; }

	EResult Evaluate();
};

UCLASS()
class TESISUE_API UAbilityTask_WaitComboInput : public UAbilityTask
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FComboInputDelegate OnChainPending;

	UPROPERTY(BlueprintAssignable)
	FComboInputDelegate OnComboInput;

	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UAbilityTask_WaitComboInput* WaitComboInput(UGameplayAbility* OwningAbility, FGameplayTag WindowTag, float HoldThreshold = 0.32f);

protected:
	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	void OnInputPressed();
	void OnInputReleased();
	void OnHoldThresholdReached();
	void OnWindowTagCountChanged(const FGameplayTag Tag, int32 NewCount);
	void HandleState();

	FGameplayTag WindowTag;
	float HoldThreshold = 0.32f;

	FComboInputState State;
	int32 WindowTagCount = 0;

	FTimerHandle HoldTimerHandle;
	FDelegateHandle PressedHandle;
	FDelegateHandle ReleasedHandle;
	FDelegateHandle WindowTagHandle;
};
