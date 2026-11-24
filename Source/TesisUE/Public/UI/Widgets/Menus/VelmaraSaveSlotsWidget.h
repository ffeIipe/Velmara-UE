#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "UI/Widgets/Menus/VelmaraSaveSlotCardWidget.h"
#include "VelmaraSaveSlotsWidget.generated.h"

class UVelmaraBracketPanel;
class UVelmaraFooterBar;
class UVelmaraButtonBase;
class UCommonTextBlock;
class UVerticalBox;

UENUM(BlueprintType)
enum class ESaveSlotsMode : uint8
{
	NewGame,
	LoadGame
};

UCLASS()
class TESISUE_API UVelmaraSaveSlotsWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UVelmaraSaveSlotsWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Velmara|UI")
	void SetSlotsMode(ESaveSlotsMode InMode);

	UFUNCTION(BlueprintCallable, Category = "Velmara|UI")
	void RefreshSlots();

	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnActivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual bool NativeOnHandleBackAction() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void BuildScreenTree();
	void UpdateHeaderText();
	void UpdateDetailPanel(int32 SlotIndex);
	void UpdateFooterActions();
	void HandleSlotActivated(int32 SlotIndex);
	void HandleSlotHighlighted(int32 SlotIndex);
	void HandleSlotDeleteRequested(int32 SlotIndex);
	void HandleDeleteButtonClicked();
	void HandleBackButtonClicked();

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> ModeSubtitleText;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> HeaderTitleText;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> SlotsContainer;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UVelmaraSaveSlotCardWidget>> SlotCards;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraBracketPanel> DetailsPanel;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> DetailTitleText;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> DetailBodyText;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> DetailQuoteText;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraButtonBase> DeleteSlotButton;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraButtonBase> BackButton;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraFooterBar> FooterBar;

	ESaveSlotsMode CurrentMode = ESaveSlotsMode::NewGame;
	TArray<FVelmaraSaveSlotViewData> CachedSlotsData;
	int32 HighlightedSlotIndex = 0;
};
