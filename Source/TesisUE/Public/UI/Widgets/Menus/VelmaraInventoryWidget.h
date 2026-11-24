#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "HUD/Inventory.h"
#include "VelmaraInventoryWidget.generated.h"

class UBorder;
class UCommonTextBlock;
class UImage;
class UInventoryComponent;
class UProgressionComponent;
class UTexture2D;
class UUniformGridPanel;
class UVelmaraBracketPanel;
class UVelmaraButtonBase;
class UVelmaraFooterBar;
class UVerticalBox;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnVelmaraItemSlotSelected, int32);

UCLASS()
class TESISUE_API UVelmaraItemSlotButton : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UVelmaraItemSlotButton(const FObjectInitializer& ObjectInitializer);

	void ConfigureSlot(int32 InSlotIndex, float InSlotDimension);
	void SetSlotData(UTexture2D* InIcon, bool bInOccupied, bool bInEquipped);
	void SetSlotHighlighted(bool bInSelected);
	int32 GetSlotIndex() const { return SlotIndex; }

	FOnVelmaraItemSlotSelected OnSlotFocusedOrHovered;
	FOnVelmaraItemSlotSelected OnSlotActivated;

protected:
	virtual UCommonButtonInternalBase* ConstructInternalButton() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnHovered() override;
	virtual void NativeOnUnhovered() override;
	virtual void NativeOnClicked() override;
	virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent) override;

private:
	void BuildSlotTree();
	void RefreshVisuals();

	int32 SlotIndex = INDEX_NONE;
	float SlotDimension = 86.0f;
	bool bOccupied = false;
	bool bEquipped = false;
	bool bHighlighted = false;
	bool bIsVisualHovered = false;
	bool bIsVisualFocused = false;

	UPROPERTY(Transient)
	TObjectPtr<UImage> FillImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> FrameImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> SelectedOverlayImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> ItemIconImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> EquippedMarkerImage;
};

UCLASS()
class TESISUE_API UVelmaraInventoryWidget : public UInventory
{
	GENERATED_BODY()

public:
	UVelmaraInventoryWidget(const FObjectInitializer& ObjectInitializer);

	virtual void BindInventoryComponent(UInventoryComponent* InInventoryComponent) override;
	virtual void RefreshInventoryUI_Implementation(const TArray<TScriptInterface<IPickable>>& Items) override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual bool NativeOnHandleBackAction() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

private:
	void BuildScreenTree();
	void RebuildGridDisplay();
	void RefreshEquippedColumn();
	void RefreshDetailsPanel();
	void RefreshCurrencyHeader();
	UTexture2D* ResolveItemIcon(UObject* ItemObject) const;
	FText ResolveItemName(UObject* ItemObject) const;
	FText ResolveItemDescription(UObject* ItemObject) const;
	FText ResolveItemCategory(UObject* ItemObject, bool bIsEquipped) const;
	void PopulateWeaponStats(UObject* ItemObject);
	void SelectSlot(int32 SlotIndex);
	void EquipSelectedSlot();
	void DropSelectedSlot();

	UFUNCTION()
	void HandleCurrencyChanged(int32 NewAmount);

	void HandleGridSlotFocused(int32 SlotIndex);
	void HandleGridSlotActivated(int32 SlotIndex);
	void HandleEquipButtonClicked();
	void HandleDropButtonClicked();

	static constexpr int32 GridColumns = 5;
	static constexpr int32 GridRows = 4;
	static constexpr int32 TotalGridSlots = GridColumns * GridRows;

	TWeakObjectPtr<UInventoryComponent> InventoryComp;
	TWeakObjectPtr<UProgressionComponent> ProgressionComp;

	TArray<TScriptInterface<IPickable>> CachedItems;
	int32 SelectedSlotIndex = 0;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> CurrencyValueText;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraItemSlotButton> PrimaryEquippedSlot;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> PrimaryEquippedNameText;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraItemSlotButton> SecondaryEquippedSlot;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> SecondaryEquippedNameText;

	UPROPERTY(Transient)
	TObjectPtr<UUniformGridPanel> ItemGridPanel;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UVelmaraItemSlotButton>> GridSlots;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraBracketPanel> DetailsPanel;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> DetailCategoryText;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> DetailTitleText;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> DetailDescriptionText;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> DetailStatsBox;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraButtonBase> EquipActionButton;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraButtonBase> DropActionButton;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraFooterBar> FooterBar;
};
