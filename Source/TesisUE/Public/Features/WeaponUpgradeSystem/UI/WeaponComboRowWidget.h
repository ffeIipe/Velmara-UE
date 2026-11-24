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
class UTexture2D;
class UWeaponData;
class UWeaponUpgradeComponent;
class UWeaponUpgradeMenuWidget;

UCLASS()
class TESISUE_API UWeaponComboRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(UWeaponUpgradeComponent* InWeaponUpgrades, UWeaponData* InWeapon, const FGameplayTag& InUnlockTag, UWeaponUpgradeMenuWidget* InOwnerMenu);

	void Refresh();

	FGameplayTag GetUnlockTag() const { return UnlockTag; }

	UWeaponData* GetWeapon() const { return Weapon.Get(); }

	UWidget* GetFocusTarget() const;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent) override;

private:
	void BuildLayout();

	void RefreshVisualState();

	UTexture2D* ResolveFallbackIcon() const;

	UFUNCTION()
	void HandleUnlockClicked();

	UFUNCTION()
	void HandleRowHovered();

	UFUNCTION()
	void HandleRowUnhovered();

	TWeakObjectPtr<UWeaponUpgradeComponent> WeaponUpgrades;

	TWeakObjectPtr<UWeaponData> Weapon;

	TWeakObjectPtr<UWeaponUpgradeMenuWidget> OwnerMenu;

	FGameplayTag UnlockTag;

	bool bIsRowHovered = false;

	bool bIsRowFocused = false;

	bool bIsUnlocked = false;

	bool bIsAvailable = false;

	UPROPERTY(Transient)
	TObjectPtr<UButton> RowButton;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> FillBorder;

	UPROPERTY(Transient)
	TObjectPtr<UImage> HoverWashImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> FrameBorderImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> FocusStarImage;

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> IconBox;

	UPROPERTY(Transient)
	TObjectPtr<UImage> IconImage;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> InputText;

	UPROPERTY(Transient)
	TObjectPtr<UImage> StateDiamondImage;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StateText;

	UPROPERTY(Transient)
	TObjectPtr<UImage> CostBloodIcon;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CostText;

	UPROPERTY(Transient)
	TObjectPtr<UImage> ChevronImage;
};
