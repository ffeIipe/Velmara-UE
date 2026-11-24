#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "CommonButtonBase.h"
#include "DataAssets/ShopItemData.h"
#include "VelmaraShopMenuWidget.generated.h"

class UBorder;
class UCommonTextBlock;
class UDataTable;
class UImage;
class UProgressionComponent;
class UTexture2D;
class UVelmaraBracketPanel;
class UVelmaraButtonBase;
class UVelmaraFooterBar;
class UVerticalBox;

UENUM(BlueprintType)
enum class EVelmaraShopItemState : uint8
{
	Available,
	Owned,
	Locked,
	InsufficientCurrency
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnVelmaraShopRowEvent, int32);

UCLASS()
class TESISUE_API UVelmaraShopRowButton : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UVelmaraShopRowButton(const FObjectInitializer& ObjectInitializer);

	void ConfigureRow(int32 InRowIndex);
	void SetRowData(const FShopItemData& InData, EVelmaraShopItemState InState, int32 InStatLevel);
	void SetRowSelected(bool bInSelected);

	FOnVelmaraShopRowEvent OnRowFocusedOrHovered;
	FOnVelmaraShopRowEvent OnRowActivated;

protected:
	virtual UCommonButtonInternalBase* ConstructInternalButton() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnHovered() override;
	virtual void NativeOnUnhovered() override;
	virtual void NativeOnClicked() override;
	virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent) override;

private:
	void BuildRowTree();
	void RefreshVisuals();

	int32 RowIndex = INDEX_NONE;
	EVelmaraShopItemState ItemState = EVelmaraShopItemState::Available;
	bool bRowSelected = false;
	bool bVisualHovered = false;
	bool bVisualFocused = false;

	UPROPERTY(Transient)
	TObjectPtr<UImage> FocusStarImage;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> FillBorder;

	UPROPERTY(Transient)
	TObjectPtr<UImage> HoverWashImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> FrameBorderImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> IconImage;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> NameText;

	UPROPERTY(Transient)
	TObjectPtr<UImage> StateDiamondImage;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> StateText;

	UPROPERTY(Transient)
	TObjectPtr<UImage> CostBloodIcon;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> CostText;

	UPROPERTY(Transient)
	TObjectPtr<UImage> ChevronImage;
};

UCLASS()
class TESISUE_API UVelmaraShopMenuWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UVelmaraShopMenuWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Velmara|Shop")
	void RefreshShop();

	UFUNCTION(BlueprintCallable, Category = "Velmara|Shop")
	void SetShopDataTable(UDataTable* InDataTable);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Velmara|Shop")
	TObjectPtr<UDataTable> ShopDataTable;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual bool NativeOnHandleBackAction() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

private:
	void BuildScreenTree();
	void ResolveProgressionComponent();
	void LoadShopEntries();
	void RebuildRows();
	void RefreshCurrencyHeader();
	void RefreshDetailsPanel();
	EVelmaraShopItemState EvaluateItemState(const FShopItemData& Item, int32& OutStatLevel) const;
	void SelectRow(int32 RowIndex);
	void PurchaseSelected();
	void ApplyPauseState(bool bPaused);

	UFUNCTION()
	void HandleCurrencyChanged(int32 NewAmount);

	UFUNCTION()
	void HandleComboUnlocked(FGameplayTag UnlockedTag);

	UFUNCTION()
	void HandleStatUpgraded(FGameplayTag StatTag, int32 NewLevel);

	void HandleRowFocused(int32 RowIndex);
	void HandleRowActivated(int32 RowIndex);
	void HandleBuyButtonClicked();

	TWeakObjectPtr<UProgressionComponent> ProgressionComp;
	TArray<FShopItemData> ShopItems;
	int32 SelectedIndex = 0;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> CurrencyValueText;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> RowsContainer;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> EmptyMessageText;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UVelmaraShopRowButton>> RowWidgets;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraBracketPanel> DetailsPanel;

	UPROPERTY(Transient)
	TObjectPtr<UImage> DetailIconImage;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> DetailStatusText;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> DetailTitleText;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> DetailDescText;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> DetailRequirementText;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> DetailCostText;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> DetailFeedbackText;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraButtonBase> BuyButton;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraFooterBar> FooterBar;
};
