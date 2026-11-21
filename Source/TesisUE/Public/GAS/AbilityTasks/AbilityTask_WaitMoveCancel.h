#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "GameplayTagContainer.h"
#include "AbilityTask_WaitMoveCancel.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMoveCancelDelegate);

UCLASS()
class TESISUE_API UAbilityTask_WaitMoveCancel : public UAbilityTask
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FMoveCancelDelegate OnMoveInput;

	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UAbilityTask_WaitMoveCancel* WaitMoveCancel(UGameplayAbility* OwningAbility, FGameplayTag WindowTag, float InputThreshold = 0.2f);

protected:
	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	void OnWindowTagCountChanged(const FGameplayTag Tag, int32 NewCount);
	bool IsAttackInputHeld() const;

	FGameplayTag WindowTag;
	float InputThreshold = 0.2f;
	bool bWindowOpened = false;
	FDelegateHandle WindowTagHandle;
};
