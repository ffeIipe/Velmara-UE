// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_WaitInputHold.generated.h"

/**
 * 
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWaitInputHoldDelegate);

UCLASS()
class TESISUE_API UAbilityTask_WaitInputHold : public UAbilityTask
{
	GENERATED_BODY()
	
	public:
    	UPROPERTY(BlueprintAssignable)
    	FWaitInputHoldDelegate OnHoldCompleted;
    
    	UPROPERTY(BlueprintAssignable)
    	FWaitInputHoldDelegate OnHoldCancelled;
    
    	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
    	static UAbilityTask_WaitInputHold* WaitInputHold(UGameplayAbility* OwningAbility, float HoldTime);
    
    	virtual void Activate() override;
    	virtual void OnDestroy(bool bInOwnerFinished) override;
    
    protected:
    	void OnInputRelease();
    	void OnHoldFinished();
    
    	float HoldDuration;
    	FTimerHandle TimerHandle;
    	FDelegateHandle DelegateHandle;
};
