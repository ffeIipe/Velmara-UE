#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UpgradeMenuWidgetBase.generated.h"

class UUpgradeComponentBase;

UCLASS(Abstract)
class TESISUE_API UUpgradeMenuWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Upgrades")
	void SetUpgradeComponent(UUpgradeComponentBase* InUpgradeComponent);

	UFUNCTION(BlueprintPure, Category = "Upgrades")
	UUpgradeComponentBase* GetUpgradeComponent() const { return UpgradeComponent.Get(); }

	UFUNCTION(BlueprintCallable, Category = "Upgrades")
	void SetEmbeddedMode(bool bInEmbedded);

	UFUNCTION(BlueprintPure, Category = "Upgrades")
	bool IsEmbeddedMode() const { return bEmbeddedMode; }

	UFUNCTION(BlueprintCallable, Category = "Upgrades")
	virtual void RefreshMenu() {}

	virtual UWidget* GetFirstFocusableWidget() const { return nullptr; }

protected:
	virtual void NativeOnInitialized() override;

	virtual void NativeDestruct() override;

	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	virtual void BindToUpgradeComponent();

	virtual void UnbindFromUpgradeComponent();

	virtual void OnUpgradeComponentSet() { RefreshMenu(); }

	virtual void OnEmbeddedModeChanged() {}

	void RequestClose();

	bool bEmbeddedMode = false;

private:
	bool IsCloseKey(const FKey& Key) const;

	UFUNCTION()
	void HandleCurrencyChanged(int32 NewCurrencyAmount);

	TWeakObjectPtr<UUpgradeComponentBase> UpgradeComponent;
};
