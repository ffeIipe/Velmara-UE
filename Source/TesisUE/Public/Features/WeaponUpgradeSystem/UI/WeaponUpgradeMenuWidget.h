#pragma once

#include "CoreMinimal.h"
#include "Features/UpgradeSystem/UI/UpgradeMenuWidgetBase.h"
#include "Features/WeaponUpgradeSystem/Data/WeaponUpgradeTypes.h"
#include "WeaponUpgradeMenuWidget.generated.h"

class UBorder;
class UHorizontalBox;
class USizeBox;
class UTextBlock;
class UVerticalBox;
class UWeaponComboRowWidget;
class UWeaponData;
class UWeaponUpgradeComponent;
class UWeaponUpgradeTabWidget;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnWeaponComboSelectedSignature, UWeaponData*, FGameplayTag);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnWeaponFeedbackSignature, const FText&, bool);

UCLASS()
class TESISUE_API UWeaponUpgradeMenuWidget : public UUpgradeMenuWidgetBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Weapon Upgrades")
	UWeaponUpgradeComponent* GetWeaponUpgrades() const;

	virtual void RefreshMenu() override;

	virtual UWidget* GetFirstFocusableWidget() const override;

	void SelectWeapon(int32 WeaponIndex);

	void CycleWeapon(int32 Delta);

	void ShowUnlockResult(EWeaponUpgradeResult Result, const UWeaponData* Weapon, const FGameplayTag& UnlockTag);

	void NotifyComboSelected(UWeaponData* InWeapon, const FGameplayTag& InUnlockTag);

	FOnWeaponComboSelectedSignature OnComboSelected;

	FOnWeaponFeedbackSignature OnFeedbackMessage;

protected:
	virtual void NativeOnInitialized() override;

	virtual void BindToUpgradeComponent() override;

	virtual void UnbindFromUpgradeComponent() override;

	virtual void OnUpgradeComponentSet() override;

	virtual void OnEmbeddedModeChanged() override;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Upgrades")
	TSubclassOf<UWeaponComboRowWidget> RowWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Upgrades")
	TSubclassOf<UWeaponUpgradeTabWidget> TabWidgetClass;

private:
	void BuildLayout();

	void UpdateStandaloneChromeVisibility();

	void RebuildTabs();

	void RebuildRows();

	UWeaponData* GetSelectedWeapon() const;

	UFUNCTION()
	void HandleCloseClicked();

	UFUNCTION()
	void HandleCombosChanged();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWeaponData>> Weapons;

	UPROPERTY(Transient)
	TObjectPtr<UWeaponData> SelectedWeapon;

	UPROPERTY(Transient)
	TObjectPtr<UWeaponData> RowsWeapon;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> RootDimmer;

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> PanelSizeBox;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> PanelBorder;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> StandaloneHeaderBox;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> StandaloneFooterBox;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CurrencyText;

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> TabsContainer;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> WeaponNameText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> WeaponProgressText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EmptyText;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> RowsContainer;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FeedbackText;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWeaponUpgradeTabWidget>> Tabs;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWeaponComboRowWidget>> Rows;
};
