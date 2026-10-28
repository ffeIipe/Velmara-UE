#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "WeaponComboRowWidget.generated.h"

class UBorder;
class UButton;
class UImage;
class USizeBox;
class UTextBlock;
class UWeaponData;
class UWeaponUpgradeComponent;
class UWeaponUpgradeMenuWidget;

/*
 * Fila del Altar de Sangre: un combo desbloqueable del arma (nombre, inputs, golpes, requisitos) y su botón de compra.
 * Construye su layout por código.
 */
UCLASS()
class TESISUE_API UWeaponComboRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(UWeaponUpgradeComponent* InWeaponUpgrades, UWeaponData* InWeapon, const FGameplayTag& InUnlockTag, UWeaponUpgradeMenuWidget* InOwnerMenu);

	void Refresh();

protected:
	virtual void NativeOnInitialized() override;

private:
	void BuildLayout();

	UFUNCTION()
	void HandleUnlockClicked();

	TWeakObjectPtr<UWeaponUpgradeComponent> WeaponUpgrades;

	TWeakObjectPtr<UWeaponData> Weapon;

	TWeakObjectPtr<UWeaponUpgradeMenuWidget> OwnerMenu;

	FGameplayTag UnlockTag;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> Background;

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> IconBox;

	UPROPERTY(Transient)
	TObjectPtr<UImage> IconImage;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StateText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> InputText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DescriptionText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RequirementsText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> UnlockButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ButtonLabelText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ButtonCostText;
};
