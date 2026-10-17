#pragma once

#include "CoreMinimal.h"
#include "Features/UpgradeSystem/UI/UpgradeMenuWidgetBase.h"
#include "Features/WeaponUpgradeSystem/Data/WeaponUpgradeTypes.h"
#include "WeaponUpgradeMenuWidget.generated.h"

class UHorizontalBox;
class UTextBlock;
class UVerticalBox;
class UWeaponComboRowWidget;
class UWeaponData;
class UWeaponUpgradeComponent;
class UWeaponUpgradeTabWidget;

/*
 * Menú del Altar de Sangre: una pestaña por arma y, para la seleccionada, sus combos desbloqueables.
 * Construye su layout por código. Lo abre/cierra UWeaponUpgradeComponent::ToggleMenu() (ej. desde ABloodAltar).
 */
UCLASS()
class TESISUE_API UWeaponUpgradeMenuWidget : public UUpgradeMenuWidgetBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Weapon Upgrades")
	UWeaponUpgradeComponent* GetWeaponUpgrades() const;

	virtual void RefreshMenu() override;

	void SelectWeapon(int32 WeaponIndex);

	// Lo llaman las filas tras intentar un desbloqueo.
	void ShowUnlockResult(EWeaponUpgradeResult Result, const UWeaponData* Weapon, const FGameplayTag& UnlockTag);

protected:
	virtual void NativeOnInitialized() override;

	virtual void BindToUpgradeComponent() override;

	virtual void UnbindFromUpgradeComponent() override;

	virtual void OnUpgradeComponentSet() override;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Upgrades")
	TSubclassOf<UWeaponComboRowWidget> RowWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Upgrades")
	TSubclassOf<UWeaponUpgradeTabWidget> TabWidgetClass;

private:
	void BuildLayout();

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

	// Arma y cantidad de combos con las que se construyeron las filas actuales.
	UPROPERTY(Transient)
	TObjectPtr<UWeaponData> RowsWeapon;

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
