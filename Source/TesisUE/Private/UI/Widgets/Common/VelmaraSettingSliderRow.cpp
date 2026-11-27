#include "UI/Widgets/Common/VelmaraSettingSliderRow.h"
#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "UI/Style/VelmaraUITheme.h"
#include "UI/Widgets/Common/VelmaraCommonStyles.h"

UVelmaraSettingSliderRow::UVelmaraSettingSliderRow(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Style = UVelmaraTransparentButtonStyle::StaticClass();
	SetIsFocusable(true);
}

UCommonButtonInternalBase* UVelmaraSettingSliderRow::ConstructInternalButton()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildRowTree();
	}
	return Super::ConstructInternalButton();
}

TSharedRef<SWidget> UVelmaraSettingSliderRow::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildRowTree();
	}
	FVelmaraUITheme::ApplyButtonSounds(*this, false);
	return Super::RebuildWidget();
}

void UVelmaraSettingSliderRow::NativeConstruct()
{
	Super::NativeConstruct();
	UpdateValueDisplay();
	UpdateVisualState();
}

void UVelmaraSettingSliderRow::BuildRowTree()
{
	USizeBox* RootSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SliderRootSize"));
	RootSize->SetMinDesiredHeight(56.0f);
	WidgetTree->RootWidget = RootSize;

	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("SliderOverlay"));
	RootSize->SetContent(RootOverlay);

	SolidBackdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SliderBackdrop"));
	SolidBackdrop->SetPadding(FMargin(0.0f));
	if (UOverlaySlot* BgSlot = RootOverlay->AddChildToOverlay(SolidBackdrop))
	{
		BgSlot->SetHorizontalAlignment(HAlign_Fill);
		BgSlot->SetVerticalAlignment(VAlign_Fill);
	}

	FocusGradient = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SliderGradient"));
	FocusGradient->SetVisibility(ESlateVisibility::Hidden);
	if (UOverlaySlot* GradSlot = RootOverlay->AddChildToOverlay(FocusGradient))
	{
		GradSlot->SetHorizontalAlignment(HAlign_Fill);
		GradSlot->SetVerticalAlignment(VAlign_Fill);
	}

	FrameBorder = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SliderFrame"));
	FrameBorder->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (UOverlaySlot* FrameSlot = RootOverlay->AddChildToOverlay(FrameBorder))
	{
		FrameSlot->SetHorizontalAlignment(HAlign_Fill);
		FrameSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UGridPanel* Grid = WidgetTree->ConstructWidget<UGridPanel>(UGridPanel::StaticClass(), TEXT("SliderGrid"));
	Grid->SetColumnFill(0, 0.52f);
	Grid->SetColumnFill(1, 0.48f);
	if (UOverlaySlot* GridSlot = RootOverlay->AddChildToOverlay(Grid))
	{
		GridSlot->SetHorizontalAlignment(HAlign_Fill);
		GridSlot->SetVerticalAlignment(VAlign_Fill);
		GridSlot->SetPadding(FMargin(28.0f, 6.0f, 20.0f, 6.0f));
	}

	UHorizontalBox* LeftBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SliderLeftBox"));
	if (UGridSlot* LeftCell = Grid->AddChildToGrid(LeftBox, 0, 0))
	{
		LeftCell->SetHorizontalAlignment(HAlign_Fill);
		LeftCell->SetVerticalAlignment(VAlign_Center);
	}

	LabelText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("SliderLabel"));
	LabelText->SetText(RowLabel.ToUpper());
	LabelText->SetFont(FVelmaraUITheme::GetDisplayFont(19, 170));
	if (UHorizontalBoxSlot* LabelSlot = LeftBox->AddChildToHorizontalBox(LabelText))
	{
		LabelSlot->SetVerticalAlignment(VAlign_Center);
		LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UHorizontalBox* RightBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SliderRightBox"));
	if (UGridSlot* RightCell = Grid->AddChildToGrid(RightBox, 0, 1))
	{
		RightCell->SetHorizontalAlignment(HAlign_Fill);
		RightCell->SetVerticalAlignment(VAlign_Center);
	}

	UOverlay* SliderOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("SliderTrackOverlay"));
	if (UHorizontalBoxSlot* TrackSlot = RightBox->AddChildToHorizontalBox(SliderOverlay))
	{
		TrackSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TrackSlot->SetVerticalAlignment(VAlign_Center);
		TrackSlot->SetPadding(FMargin(6.0f, 0.0f, 14.0f, 0.0f));
	}

	USizeBox* FillSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SliderFillSize"));
	FillSize->SetHeightOverride(3.0f);
	FillBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("SliderFillBar"));
	FillBar->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	FProgressBarStyle BarStyle;
	FSlateBrush EmptyBg;
	EmptyBg.DrawAs = ESlateBrushDrawType::NoDrawType;
	BarStyle.SetBackgroundImage(EmptyBg);
	BarStyle.SetFillImage(FVelmaraUITheme::MakeSolidBrush(FVelmaraUITheme::BloodBright()));
	FillBar->SetWidgetStyle(BarStyle);
	FillBar->SetFillColorAndOpacity(FVelmaraUITheme::BloodBright());
	FillSize->SetContent(FillBar);
	if (UOverlaySlot* FillSlot = SliderOverlay->AddChildToOverlay(FillSize))
	{
		FillSlot->SetHorizontalAlignment(HAlign_Fill);
		FillSlot->SetVerticalAlignment(VAlign_Center);
		FillSlot->SetPadding(FMargin(8.0f, 0.0f));
	}

	ValueSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("SettingValueSlider"));
	ValueSlider->SetMinValue(MinValue);
	ValueSlider->SetMaxValue(MaxValue);
	ValueSlider->SetStepSize(StepSize);
	ValueSlider->IsFocusable = false;
	ValueSlider->OnValueChanged.AddDynamic(this, &UVelmaraSettingSliderRow::HandleSliderValueChanged);

	FSliderStyle SliderStyle;
	const FSlateBrush TrackNormal = FVelmaraUITheme::MakeSliderTrackBrush(FVelmaraUITheme::Ash());
	const FSlateBrush TrackHovered = FVelmaraUITheme::MakeSliderTrackBrush(FVelmaraUITheme::BoneMuted());
	UTexture2D* ThumbTex = FVelmaraUITheme::LoadTexture(VelmaraUIPaths::SliderThumb);
	const FSlateBrush ThumbNormal = FVelmaraUITheme::MakeImageBrush(ThumbTex, FVector2D(18.0f, 18.0f), FVelmaraUITheme::Bone());
	const FSlateBrush ThumbHovered = FVelmaraUITheme::MakeImageBrush(ThumbTex, FVector2D(20.0f, 20.0f), FVelmaraUITheme::BloodBright());

	SliderStyle.SetNormalBarImage(TrackNormal);
	SliderStyle.SetHoveredBarImage(TrackHovered);
	SliderStyle.SetDisabledBarImage(TrackNormal);
	SliderStyle.SetNormalThumbImage(ThumbNormal);
	SliderStyle.SetHoveredThumbImage(ThumbHovered);
	SliderStyle.SetDisabledThumbImage(ThumbNormal);
	SliderStyle.SetBarThickness(4.0f);
	ValueSlider->SetWidgetStyle(SliderStyle);

	if (UOverlaySlot* SliderSlot = SliderOverlay->AddChildToOverlay(ValueSlider))
	{
		SliderSlot->SetHorizontalAlignment(HAlign_Fill);
		SliderSlot->SetVerticalAlignment(VAlign_Center);
	}

	USizeBox* ValueTextSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SliderValueTextSize"));
	ValueTextSize->SetWidthOverride(64.0f);
	ValueText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("SliderValueText"));
	ValueText->SetFont(FVelmaraUITheme::GetDisplayFont(18, 120));
	ValueText->SetJustification(ETextJustify::Right);
	ValueTextSize->SetContent(ValueText);
	if (UHorizontalBoxSlot* ValTextSlot = RightBox->AddChildToHorizontalBox(ValueTextSize))
	{
		ValTextSlot->SetVerticalAlignment(VAlign_Center);
	}

	USizeBox* StarBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SliderStarBox"));
	StarBox->SetWidthOverride(12.0f);
	StarBox->SetHeightOverride(12.0f);
	BottomStar = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SliderStarImg"));
	BottomStar->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	StarBox->SetContent(BottomStar);
	if (UOverlaySlot* StarSlot = RootOverlay->AddChildToOverlay(StarBox))
	{
		StarSlot->SetHorizontalAlignment(HAlign_Center);
		StarSlot->SetVerticalAlignment(VAlign_Bottom);
		StarSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, -6.0f));
	}

	UpdateValueDisplay();
	UpdateVisualState();
}

void UVelmaraSettingSliderRow::SetupRow(
	const FText& InLabel,
	float InMinValue,
	float InMaxValue,
	float InStepSize,
	float InInitialValue,
	EVelmaraSliderDisplayFormat InFormat,
	const FVelmaraSettingMetadata& InMeta)
{
	RowLabel = InLabel;
	MinValue = InMinValue;
	MaxValue = FMath::Max(InMinValue + 0.01f, InMaxValue);
	StepSize = FMath::Max(0.005f, InStepSize);
	DisplayFormat = InFormat;
	Metadata = InMeta;
	CurrentValue = FMath::Clamp(InInitialValue, MinValue, MaxValue);

	if (LabelText)
	{
		LabelText->SetText(RowLabel.ToUpper());
	}
	if (ValueSlider)
	{
		bSuppressSliderEvent = true;
		ValueSlider->SetMinValue(MinValue);
		ValueSlider->SetMaxValue(MaxValue);
		ValueSlider->SetStepSize(StepSize);
		ValueSlider->SetValue(CurrentValue);
		bSuppressSliderEvent = false;
	}

	UpdateValueDisplay();
	UpdateVisualState();
}

void UVelmaraSettingSliderRow::SetSliderValue(float NewValue, bool bBroadcast)
{
	const float Clamped = FMath::Clamp(NewValue, MinValue, MaxValue);
	const bool bChanged = !FMath::IsNearlyEqual(Clamped, CurrentValue, 0.0005f);
	CurrentValue = Clamped;

	if (ValueSlider)
	{
		bSuppressSliderEvent = true;
		ValueSlider->SetValue(CurrentValue);
		bSuppressSliderEvent = false;
	}

	UpdateValueDisplay();

	if (bBroadcast && bChanged)
	{
		OnValueChanged.Broadcast(CurrentValue);
	}
}

void UVelmaraSettingSliderRow::HandleSliderValueChanged(float InValue)
{
	if (bSuppressSliderEvent)
	{
		return;
	}

	const float Snapped = FMath::RoundToFloat((InValue - MinValue) / StepSize) * StepSize + MinValue;
	CurrentValue = FMath::Clamp(Snapped, MinValue, MaxValue);
	UpdateValueDisplay();
	OnValueChanged.Broadcast(CurrentValue);
}

void UVelmaraSettingSliderRow::UpdateValueDisplay()
{
	const float Alpha = (MaxValue > MinValue) ? FMath::Clamp((CurrentValue - MinValue) / (MaxValue - MinValue), 0.0f, 1.0f) : 0.0f;
	if (FillBar)
	{
		FillBar->SetPercent(Alpha);
	}

	if (!ValueText)
	{
		return;
	}

	FString Formatted;
	switch (DisplayFormat)
	{
	case EVelmaraSliderDisplayFormat::Percent:
		Formatted = FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Alpha * 100.0f));
		break;
	case EVelmaraSliderDisplayFormat::Decimal1:
		Formatted = FString::Printf(TEXT("%.1f"), CurrentValue);
		break;
	default:
		Formatted = FString::Printf(TEXT("%.2f"), CurrentValue);
		break;
	}
	ValueText->SetText(FText::FromString(Formatted));
}

bool UVelmaraSettingSliderRow::IsVisuallyFocused() const
{
	return IsHovered() || HasAnyUserFocus() || HasKeyboardFocus() || bIsInFocusPath;
}

void UVelmaraSettingSliderRow::UpdateVisualState()
{
	if (!SolidBackdrop || !FrameBorder || !LabelText || !ValueText)
	{
		return;
	}

	const bool bFocused = IsVisuallyFocused();

	SolidBackdrop->SetBrush(FVelmaraUITheme::MakeSolidBrush(bFocused ? FVelmaraUITheme::BloodDried(0.38f) : FVelmaraUITheme::Void(0.72f)));
	FrameBorder->SetBrush(FVelmaraUITheme::MakeOptionRowBrush(bFocused ? FVelmaraUITheme::BloodBright() : FVelmaraUITheme::Ash()));

	if (FocusGradient)
	{
		if (bFocused)
		{
			UTexture2D* GradTex = FVelmaraUITheme::LoadTexture(VelmaraUIPaths::GradHFadeLeftToRight);
			FocusGradient->SetBrush(FVelmaraUITheme::MakeImageBrush(GradTex, FVector2D(16.0f, 8.0f), FVelmaraUITheme::Blood(0.32f)));
			FocusGradient->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			FocusGradient->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	LabelText->SetColorAndOpacity(FSlateColor(bFocused ? FVelmaraUITheme::Bone() : FVelmaraUITheme::BoneMuted()));
	ValueText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));

	UTexture2D* StarTex = FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnRowStar);

	if (BottomStar)
	{
		BottomStar->SetBrush(FVelmaraUITheme::MakeImageBrush(StarTex, FVector2D(12.0f, 12.0f), bFocused ? FVelmaraUITheme::Bone() : FVelmaraUITheme::BoneMuted(0.65f)));
	}
}

void UVelmaraSettingSliderRow::NativeOnHovered()
{
	Super::NativeOnHovered();
	SetKeyboardFocus();
	OnRowHighlighted.Broadcast(Metadata);
	UpdateVisualState();
}

void UVelmaraSettingSliderRow::NativeOnUnhovered()
{
	Super::NativeOnUnhovered();
	UpdateVisualState();
}

void UVelmaraSettingSliderRow::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnAddedToFocusPath(InFocusEvent);
	bIsInFocusPath = true;
	OnRowHighlighted.Broadcast(Metadata);
	UpdateVisualState();
}

void UVelmaraSettingSliderRow::NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnRemovedFromFocusPath(InFocusEvent);
	bIsInFocusPath = false;
	UpdateVisualState();
}

FReply UVelmaraSettingSliderRow::NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent)
{
	const FReply Reply = Super::NativeOnFocusReceived(InGeometry, InFocusEvent);
	bIsInFocusPath = true;
	OnRowHighlighted.Broadcast(Metadata);
	UpdateVisualState();
	return Reply;
}

void UVelmaraSettingSliderRow::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnFocusLost(InFocusEvent);
	bIsInFocusPath = false;
	UpdateVisualState();
}

FReply UVelmaraSettingSliderRow::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Left || Key == EKeys::A || Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_LeftStick_Left)
	{
		SetSliderValue(CurrentValue - StepSize, true);
		return FReply::Handled();
	}
	if (Key == EKeys::Right || Key == EKeys::D || Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_LeftStick_Right)
	{
		SetSliderValue(CurrentValue + StepSize, true);
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
