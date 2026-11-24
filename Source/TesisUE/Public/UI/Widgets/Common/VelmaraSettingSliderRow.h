#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "UI/Widgets/Common/VelmaraSettingRowBase.h"
#include "VelmaraSettingSliderRow.generated.h"

class UBorder;
class UImage;
class USlider;
class UProgressBar;
class UCommonTextBlock;

UENUM(BlueprintType)
enum class EVelmaraSliderDisplayFormat : uint8
{
	Percent,
	Decimal1,
	Decimal2
};

UCLASS()
class TESISUE_API UVelmaraSettingSliderRow : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UVelmaraSettingSliderRow(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Velmara|Settings")
	void SetupRow(
		const FText& InLabel,
		float InMinValue,
		float InMaxValue,
		float InStepSize,
		float InInitialValue,
		EVelmaraSliderDisplayFormat InFormat,
		const FVelmaraSettingMetadata& InMeta);

	UFUNCTION(BlueprintCallable, Category = "Velmara|Settings")
	void SetSliderValue(float NewValue, bool bBroadcast = false);

	UFUNCTION(BlueprintPure, Category = "Velmara|Settings")
	float GetSliderValue() const { return CurrentValue; }

	const FVelmaraSettingMetadata& GetMetadata() const { return Metadata; }

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnSliderSettingChanged, float);
	FOnSliderSettingChanged OnValueChanged;

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnSliderRowHighlighted, const FVelmaraSettingMetadata&);
	FOnSliderRowHighlighted OnRowHighlighted;

protected:
	virtual UCommonButtonInternalBase* ConstructInternalButton() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeOnHovered() override;
	virtual void NativeOnUnhovered() override;
	virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual FReply NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnFocusLost(const FFocusEvent& InFocusEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void BuildRowTree();
	void UpdateVisualState();
	void UpdateValueDisplay();
	bool IsVisuallyFocused() const;

	UFUNCTION()
	void HandleSliderValueChanged(float InValue);

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
	TObjectPtr<UProgressBar> FillBar;

	UPROPERTY(Transient)
	TObjectPtr<USlider> ValueSlider;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> ValueText;

	FText RowLabel;
	float MinValue = 0.0f;
	float MaxValue = 1.0f;
	float StepSize = 0.05f;
	float CurrentValue = 1.0f;
	EVelmaraSliderDisplayFormat DisplayFormat = EVelmaraSliderDisplayFormat::Percent;
	bool bIsInFocusPath = false;
	bool bSuppressSliderEvent = false;
	FVelmaraSettingMetadata Metadata;
};
