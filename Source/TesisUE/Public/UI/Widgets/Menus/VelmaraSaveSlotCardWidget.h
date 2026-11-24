#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "VelmaraSaveSlotCardWidget.generated.h"

class UBorder;
class UImage;
class UCommonTextBlock;
class UHorizontalBox;
class UVerticalBox;

USTRUCT(BlueprintType)
struct TESISUE_API FVelmaraSaveSlotViewData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 SlotIndex = 0;

	UPROPERTY(BlueprintReadOnly)
	bool bOccupied = false;

	UPROPERTY(BlueprintReadOnly)
	FString RawLevelName;

	UPROPERTY(BlueprintReadOnly)
	FDateTime Timestamp;

	UPROPERTY(BlueprintReadOnly)
	int32 TotalAttributeLevels = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 UnlockedCombosCount = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 SavedActorsCount = 0;
};

UCLASS()
class TESISUE_API UVelmaraSaveSlotCardWidget : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UVelmaraSaveSlotCardWidget(const FObjectInitializer& ObjectInitializer);

	void ConfigureCard(const FVelmaraSaveSlotViewData& InData, bool bInLoadMode);

	int32 GetSlotIndex() const { return SlotData.SlotIndex; }
	bool IsOccupied() const { return SlotData.bOccupied; }
	const FVelmaraSaveSlotViewData& GetSlotData() const { return SlotData; }

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnSaveSlotEvent, int32);
	FOnSaveSlotEvent OnSlotActivated;
	FOnSaveSlotEvent OnSlotHighlighted;
	FOnSaveSlotEvent OnSlotDeleteRequested;

protected:
	virtual UCommonButtonInternalBase* ConstructInternalButton() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeOnHovered() override;
	virtual void NativeOnUnhovered() override;
	virtual void NativeOnClicked() override;
	virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual FReply NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnFocusLost(const FFocusEvent& InFocusEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void BuildCardTree();
	void RefreshContent();
	void UpdateVisualState();
	bool IsVisuallyFocused() const;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> SolidBackdrop;

	UPROPERTY(Transient)
	TObjectPtr<UImage> FocusGradient;

	UPROPERTY(Transient)
	TObjectPtr<UImage> FrameBorder;

	UPROPERTY(Transient)
	TObjectPtr<UImage> BottomStar;

	UPROPERTY(Transient)
	TObjectPtr<UImage> EmblemFrame;

	UPROPERTY(Transient)
	TObjectPtr<UImage> EmblemGlyph;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> OccupiedInfoBox;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> SubtitleText;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> EmptySlotText;

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> RightMetaBox;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> ProgressionCountText;

	UPROPERTY(Transient)
	TObjectPtr<UImage> RightChevron;

	FVelmaraSaveSlotViewData SlotData;
	bool bLoadMode = false;
	bool bIsInFocusPath = false;
};
