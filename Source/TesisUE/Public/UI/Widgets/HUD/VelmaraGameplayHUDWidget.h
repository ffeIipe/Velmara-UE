#pragma once

#include "CoreMinimal.h"
#include "CommonInputTypeEnum.h"
#include "GameplayEffectTypes.h"
#include "HUD/PlayerMainWidget.h"
#include "VelmaraGameplayHUDWidget.generated.h"

class UAbilitySystemComponent;
class UProgressionComponent;
class UInventoryComponent;
class IPickable;
class UProgressBar;
class UImage;
class UHorizontalBox;
class UCommonTextBlock;
class USizeBox;

UCLASS()
class TESISUE_API UVelmaraGameplayHUDWidget : public UPlayerMainWidget
{
	GENERATED_BODY()

public:
	UVelmaraGameplayHUDWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Velmara|HUD")
	void BindToPlayerPawn(APawn* PlayerPawn);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildHUDTree();
	void UnbindFromCurrentPawn();
	void RefreshAllVitals();
	void UpdateHealthBar(float Health, float MaxHealth);
	void UpdateShieldBar(float Shield, float MaxShield);
	void UpdateEnergyBar(float Energy, float MaxEnergy);
	void UpdateBloodCounterImmediate(int32 Amount);
	void UpdateWeaponSlotIcon(TScriptInterface<IPickable> WeaponPickable);
	void RefreshSkillKeyHints();
	void HandleInputMethodChanged(ECommonInputType NewInputType);

	void OnHealthChanged(const FOnAttributeChangeData& Data);
	void OnMaxHealthChanged(const FOnAttributeChangeData& Data);
	void OnShieldChanged(const FOnAttributeChangeData& Data);
	void OnMaxShieldChanged(const FOnAttributeChangeData& Data);
	void OnEnergyChanged(const FOnAttributeChangeData& Data);
	void OnMaxEnergyChanged(const FOnAttributeChangeData& Data);

	UFUNCTION()
	void HandleCurrencyChanged(int32 NewAmount);

	UFUNCTION()
	void HandleItemEquipped(TScriptInterface<IPickable> NewWeapon);

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> ShieldBarRowSize;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> ShieldProgressBar;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> DelayedHealthBar;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> HealthProgressBar;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> EnergyProgressBar;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> EnergyPipImages;

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> SunlightBadgeRow;

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> PossessionBadgeRow;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCommonTextBlock>> SkillHintTexts;

	UPROPERTY(Transient)
	TObjectPtr<UImage> ActiveWeaponIcon;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> BloodCounterText;

	TWeakObjectPtr<APawn> BoundPawn;
	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
	TWeakObjectPtr<UProgressionComponent> BoundProgression;
	TWeakObjectPtr<UInventoryComponent> BoundInventory;

	FDelegateHandle HealthChangedHandle;
	FDelegateHandle MaxHealthChangedHandle;
	FDelegateHandle ShieldChangedHandle;
	FDelegateHandle MaxShieldChangedHandle;
	FDelegateHandle EnergyChangedHandle;
	FDelegateHandle MaxEnergyChangedHandle;
	FDelegateHandle InputMethodHandle;

	float TargetHealthPercent = 1.0f;
	float DisplayedDelayedHealth = 1.0f;
	float DelayedHealthHoldTimer = 0.0f;
	float DisplayedBloodValue = 0.0f;
	int32 TargetBloodValue = 0;
	float SunlightPulseTime = 0.0f;
};
