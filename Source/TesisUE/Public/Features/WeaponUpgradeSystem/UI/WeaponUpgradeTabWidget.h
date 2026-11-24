#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WeaponUpgradeTabWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UWeaponUpgradeMenuWidget;

UCLASS()
class TESISUE_API UWeaponUpgradeTabWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(UWeaponUpgradeMenuWidget* InOwnerMenu, int32 InWeaponIndex, const FText& Label);

	void SetProgressText(const FText& Progress);

	void SetSelected(bool bInSelected);

protected:
	virtual void NativeOnInitialized() override;

private:
	void BuildLayout();

	void RefreshVisualState();

	UFUNCTION()
	void HandleClicked();

	UFUNCTION()
	void HandleHovered();

	UFUNCTION()
	void HandleUnhovered();

	TWeakObjectPtr<UWeaponUpgradeMenuWidget> OwnerMenu;

	int32 WeaponIndex = INDEX_NONE;

	bool bSelected = false;

	bool bHovered = false;

	UPROPERTY(Transient)
	TObjectPtr<UButton> TabButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LabelText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ProgressText;

	UPROPERTY(Transient)
	TObjectPtr<UImage> ActiveLineImage;
};
