#pragma once

#include "CoreMinimal.h"
#include "Features/AttributeTreeSystem/Data/AttributeTreeTypes.h"
#include "Features/UpgradeSystem/UI/UpgradeMenuWidgetBase.h"
#include "AttributeTreeMenuWidget.generated.h"

class UAttributeTreeComponent;
class UAttributeTreeRowWidget;
class UBorder;
class USizeBox;
class UTextBlock;
class UVerticalBox;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnAttributeRowSelectedSignature, FGameplayTag);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnAttributeFeedbackSignature, const FText&, bool);

UCLASS()
class TESISUE_API UAttributeTreeMenuWidget : public UUpgradeMenuWidgetBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Attribute Tree")
	UAttributeTreeComponent* GetAttributeTree() const;

	virtual void RefreshMenu() override;

	virtual UWidget* GetFirstFocusableWidget() const override;

	void ShowUpgradeResult(EAttributeUpgradeResult Result, const FGameplayTag& UpgradeTag);

	void NotifyRowSelected(const FGameplayTag& UpgradeTag);

	FOnAttributeRowSelectedSignature OnRowSelected;

	FOnAttributeFeedbackSignature OnFeedbackMessage;

protected:
	virtual void NativeOnInitialized() override;

	virtual void BindToUpgradeComponent() override;

	virtual void UnbindFromUpgradeComponent() override;

	virtual void OnUpgradeComponentSet() override;

	virtual void OnEmbeddedModeChanged() override;

	UPROPERTY(EditDefaultsOnly, Category = "Attribute Tree")
	TSubclassOf<UAttributeTreeRowWidget> RowWidgetClass;

private:
	void BuildLayout();

	void RebuildRows();

	void UpdateStandaloneChromeVisibility();

	UFUNCTION()
	void HandleCloseClicked();

	UFUNCTION()
	void HandleAttributeUpgraded(FGameplayTag UpgradeTag, int32 NewLevel);

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
	TObjectPtr<UTextBlock> FeedbackText;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> RowsContainer;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UAttributeTreeRowWidget>> Rows;
};
