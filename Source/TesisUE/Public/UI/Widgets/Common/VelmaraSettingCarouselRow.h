#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "UI/Widgets/Common/VelmaraSettingRowBase.h"
#include "VelmaraSettingCarouselRow.generated.h"

class USizeBox;
class UOverlay;
class UBorder;
class UImage;
class UGridPanel;
class UCommonTextBlock;

UCLASS()
class TESISUE_API UVelmaraSettingCarouselRow : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UVelmaraSettingCarouselRow(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Velmara|Settings")
	void SetupRow(const FText& InLabel, const TArray<FText>& InOptions, int32 InitialIndex, bool bInWrapAround, const FVelmaraSettingMetadata& InMeta);

	UFUNCTION(BlueprintCallable, Category = "Velmara|Settings")
	void SetOptions(const TArray<FText>& InOptions, int32 NewIndex = 0, bool bBroadcast = false);

	UFUNCTION(BlueprintCallable, Category = "Velmara|Settings")
	void SetSelectedIndex(int32 NewIndex, bool bBroadcast = false);

	UFUNCTION(BlueprintPure, Category = "Velmara|Settings")
	int32 GetSelectedIndex() const { return SelectedIndex; }

	UFUNCTION(BlueprintCallable, Category = "Velmara|Settings")
	void StepOption(int32 Delta);

	const FVelmaraSettingMetadata& GetMetadata() const { return Metadata; }

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnCarouselSelectionChanged, int32);
	FOnCarouselSelectionChanged OnSelectionChanged;

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnSettingRowHighlighted, const FVelmaraSettingMetadata&);
	FOnSettingRowHighlighted OnRowHighlighted;

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
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void BuildRowTree();
	void UpdateVisualState();
	void UpdateValueDisplay();
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
	TObjectPtr<UImage> LeftDecorativeChevron;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> LabelText;

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> PrevHitBox;

	UPROPERTY(Transient)
	TObjectPtr<UImage> PrevArrow;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> ValueText;

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> NextHitBox;

	UPROPERTY(Transient)
	TObjectPtr<UImage> NextArrow;

	FText RowLabel;
	TArray<FText> Options;
	int32 SelectedIndex = 0;
	bool bWrapAround = true;
	bool bIsInFocusPath = false;
	FVelmaraSettingMetadata Metadata;
};
