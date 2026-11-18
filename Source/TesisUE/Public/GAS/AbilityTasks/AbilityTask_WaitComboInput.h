#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "GameplayTagContainer.h"
#include "AbilityTask_WaitComboInput.generated.h"

// Los nodos de tarea solo muestran pines de salida si todos los delegates comparten firma.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FComboInputDelegate, bool, bIsHold);

// Lógica del input de un golpe, sin GAS para poder testearla (AbilityTask_WaitComboInputTest.cpp).
struct FComboInputState
{
	enum class EResult : uint8 { Nothing, ChainPending, Chain };

	bool bPressed = false;
	bool bResolved = false;
	bool bIsHold = false;
	bool bWindowReached = false;
	bool bPendingSent = false;

	// false si ya había un input en curso: solo el primero del golpe cuenta.
	bool Press();
	void Release();
	void HoldReached();
	void OpenWindow() { bWindowReached = true; }

	EResult Evaluate();
};

/*
 * Espera el input que encadena el siguiente golpe del combo.
 * Tap: se suelta antes de HoldThreshold. Hold: se mantiene HoldThreshold (se decide ahí, sin esperar a soltar).
 * Sale cuando el input está resuelto y la ventana (WindowTag) ya se abrió en este golpe, aunque ya se haya cerrado:
 * un input apretado a tiempo no se pierde porque la decisión Tap/Hold llegue tarde.
 */
UCLASS()
class TESISUE_API UAbilityTask_WaitComboInput : public UAbilityTask
{
	GENERATED_BODY()

public:
	// Input apretado con la ventana abierta: OnComboInput llega en menos de HoldThreshold, no terminar la habilidad. bIsHold siempre false.
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
