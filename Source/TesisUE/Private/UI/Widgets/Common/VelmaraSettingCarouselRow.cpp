#include "UI/Widgets/Common/VelmaraSettingCarouselRow.h"
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
#include "Components/SizeBox.h"
#include "UI/Style/VelmaraUITheme.h"
#include "UI/Widgets/Common/VelmaraCommonStyles.h"

UVelmaraSettingCarouselRow::UVelmaraSettingCarouselRow(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Style = UVelmaraTransparentButtonStyle::StaticClass();
	SetIsFocusable(true);
}

UCommonButtonInternalBase* UVelmaraSettingCarouselRow::ConstructInternalButton()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildRowTree();
	}
	return Super::ConstructInternalButton();
}

TSharedRef<SWidget> UVelmaraSettingCarouselRow::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildRowTree();
	}
	FVelmaraUITheme::ApplyButtonSounds(*this, false);
	return Super::RebuildWidget();
}

void UVelmaraSettingCarouselRow::NativeConstruct()
{
	Super::NativeConstruct();
	UpdateValueDisplay();
	UpdateVisualState();
}

void UVelmaraSettingCarouselRow::BuildRowTree()
{
	USizeBox* RootSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CarouselRootSize"));
	RootSize->SetMinDesiredHeight(56.0f);
	WidgetTree->RootWidget = RootSize;

	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("CarouselOverlay"));
	RootSize->SetContent(RootOverlay);

	SolidBackdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CarouselBackdrop"));
	SolidBackdrop->SetPadding(FMargin(0.0f));
	if (UOverlaySlot* BgSlot = RootOverlay->AddChildToOverlay(SolidBackdrop))
	{
		BgSlot->SetHorizontalAlignment(HAlign_Fill);
		BgSlot->SetVerticalAlignment(VAlign_Fill);
	}

	FocusGradient = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("CarouselGradient"));
	FocusGradient->SetVisibility(ESlateVisibility::Hidden);
	if (UOverlaySlot* GradSlot = RootOverlay->AddChildToOverlay(FocusGradient))
	{
		GradSlot->SetHorizontalAlignment(HAlign_Fill);
		GradSlot->SetVerticalAlignment(VAlign_Fill);
	}

	FrameBorder = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("CarouselFrame"));
	FrameBorder->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (UOverlaySlot* FrameSlot = RootOverlay->AddChildToOverlay(FrameBorder))
	{
		FrameSlot->SetHorizontalAlignment(HAlign_Fill);
		FrameSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UGridPanel* Grid = WidgetTree->ConstructWidget<UGridPanel>(UGridPanel::StaticClass(), TEXT("CarouselGrid"));
	Grid->SetColumnFill(0, 0.52f);
	Grid->SetColumnFill(1, 0.48f);
	if (UOverlaySlot* GridSlot = RootOverlay->AddChildToOverlay(Grid))
	{
		GridSlot->SetHorizontalAlignment(HAlign_Fill);
		GridSlot->SetVerticalAlignment(VAlign_Fill);
		GridSlot->SetPadding(FMargin(28.0f, 6.0f, 20.0f, 6.0f));
	}

	UHorizontalBox* LeftBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CarouselLeftBox"));
	if (UGridSlot* LeftCell = Grid->AddChildToGrid(LeftBox, 0, 0))
	{
		LeftCell->SetHorizontalAlignment(HAlign_Fill);
		LeftCell->SetVerticalAlignment(VAlign_Center);
	}

	LabelText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("CarouselLabel"));
	LabelText->SetText(RowLabel.ToUpper());
	LabelText->SetFont(FVelmaraUITheme::GetDisplayFont(19, 170));
	if (UHorizontalBoxSlot* LabelSlot = LeftBox->AddChildToHorizontalBox(LabelText))
	{
		LabelSlot->SetVerticalAlignment(VAlign_Center);
		LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UHorizontalBox* RightBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CarouselRightBox"));
	if (UGridSlot* RightCell = Grid->AddChildToGrid(RightBox, 0, 1))
	{
		RightCell->SetHorizontalAlignment(HAlign_Fill);
		RightCell->SetVerticalAlignment(VAlign_Center);
	}

	PrevHitBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PrevHitBox"));
	PrevHitBox->SetWidthOverride(36.0f);
	PrevHitBox->SetHeightOverride(40.0f);
	UBorder* PrevWrap = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PrevWrap"));
	PrevWrap->SetBrush(FVelmaraUITheme::MakeSolidBrush(FLinearColor::Transparent));
	PrevWrap->SetHorizontalAlignment(HAlign_Center);
	PrevWrap->SetVerticalAlignment(VAlign_Center);
	USizeBox* PrevArrowBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PrevArrowBox"));
	PrevArrowBox->SetWidthOverride(13.0f);
	PrevArrowBox->SetHeightOverride(18.0f);
	PrevArrow = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PrevArrowImg"));
	PrevArrowBox->SetContent(PrevArrow);
	PrevWrap->SetContent(PrevArrowBox);
	PrevHitBox->SetContent(PrevWrap);
	if (UHorizontalBoxSlot* PrevSlot = RightBox->AddChildToHorizontalBox(PrevHitBox))
	{
		PrevSlot->SetVerticalAlignment(VAlign_Center);
	}

	ValueText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("CarouselValueText"));
	ValueText->SetFont(FVelmaraUITheme::GetDisplayFont(19, 160));
	ValueText->SetJustification(ETextJustify::Center);
	if (UHorizontalBoxSlot* ValSlot = RightBox->AddChildToHorizontalBox(ValueText))
	{
		ValSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ValSlot->SetHorizontalAlignment(HAlign_Center);
		ValSlot->SetVerticalAlignment(VAlign_Center);
		ValSlot->SetPadding(FMargin(8.0f, 0.0f));
	}

	NextHitBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("NextHitBox"));
	NextHitBox->SetWidthOverride(36.0f);
	NextHitBox->SetHeightOverride(40.0f);
	UBorder* NextWrap = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("NextWrap"));
	NextWrap->SetBrush(FVelmaraUITheme::MakeSolidBrush(FLinearColor::Transparent));
	NextWrap->SetHorizontalAlignment(HAlign_Center);
	NextWrap->SetVerticalAlignment(VAlign_Center);
	USizeBox* NextArrowBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("NextArrowBox"));
	NextArrowBox->SetWidthOverride(13.0f);
	NextArrowBox->SetHeightOverride(18.0f);
	NextArrow = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("NextArrowImg"));
	NextArrowBox->SetContent(NextArrow);
	NextWrap->SetContent(NextArrowBox);
	NextHitBox->SetContent(NextWrap);
	if (UHorizontalBoxSlot* NextSlot = RightBox->AddChildToHorizontalBox(NextHitBox))
	{
		NextSlot->SetVerticalAlignment(VAlign_Center);
	}

	USizeBox* StarBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BottomStarBox"));
	StarBox->SetWidthOverride(12.0f);
	StarBox->SetHeightOverride(12.0f);
	BottomStar = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BottomStarImg"));
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

void UVelmaraSettingCarouselRow::SetupRow(const FText& InLabel, const TArray<FText>& InOptions, int32 InitialIndex, bool bInWrapAround, const FVelmaraSettingMetadata& InMeta)
{
	RowLabel = InLabel;
	Options = InOptions;
	bWrapAround = bInWrapAround;
	Metadata = InMeta;
	SelectedIndex = Options.Num() > 0 ? FMath::Clamp(InitialIndex, 0, Options.Num() - 1) : 0;

	if (LabelText)
	{
		LabelText->SetText(RowLabel.ToUpper());
	}
	UpdateValueDisplay();
	UpdateVisualState();
}

void UVelmaraSettingCarouselRow::SetOptions(const TArray<FText>& InOptions, int32 NewIndex, bool bBroadcast)
{
	Options = InOptions;
	SetSelectedIndex(NewIndex, bBroadcast);
}

void UVelmaraSettingCarouselRow::SetSelectedIndex(int32 NewIndex, bool bBroadcast)
{
	if (Options.Num() == 0)
	{
		SelectedIndex = 0;
		UpdateValueDisplay();
		return;
	}

	const int32 Clamped = FMath::Clamp(NewIndex, 0, Options.Num() - 1);
	const bool bChanged = (Clamped != SelectedIndex);
	SelectedIndex = Clamped;
	UpdateValueDisplay();
	UpdateVisualState();

	if (bBroadcast && bChanged)
	{
		OnSelectionChanged.Broadcast(SelectedIndex);
	}
}

void UVelmaraSettingCarouselRow::StepOption(int32 Delta)
{
	if (Options.Num() <= 1 || Delta == 0)
	{
		return;
	}

	int32 TargetIndex = SelectedIndex + Delta;
	if (bWrapAround)
	{
		TargetIndex = (TargetIndex % Options.Num() + Options.Num()) % Options.Num();
	}
	else
	{
		TargetIndex = FMath::Clamp(TargetIndex, 0, Options.Num() - 1);
	}

	if (TargetIndex != SelectedIndex)
	{
		SelectedIndex = TargetIndex;
		FVelmaraUITheme::PlayClickSound();
		UpdateValueDisplay();
		UpdateVisualState();
		OnSelectionChanged.Broadcast(SelectedIndex);
	}
}

void UVelmaraSettingCarouselRow::UpdateValueDisplay()
{
	if (!ValueText)
	{
		return;
	}

	if (Options.IsValidIndex(SelectedIndex))
	{
		ValueText->SetText(Options[SelectedIndex].ToUpper());
	}
	else
	{
		ValueText->SetText(FText::FromString(TEXT("—")));
	}
}

bool UVelmaraSettingCarouselRow::IsVisuallyFocused() const
{
	return IsHovered() || HasAnyUserFocus() || HasKeyboardFocus() || bIsInFocusPath;
}

void UVelmaraSettingCarouselRow::UpdateVisualState()
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

	UTexture2D* ChevL = FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnChevronL);
	UTexture2D* ChevR = FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnChevronR);
	UTexture2D* StarTex = FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnRowStar);

	const bool bCanPrev = Options.Num() > 1 && (bWrapAround || SelectedIndex > 0);
	const bool bCanNext = Options.Num() > 1 && (bWrapAround || SelectedIndex < Options.Num() - 1);

	if (PrevArrow)
	{
		const FLinearColor PrevColor = !bCanPrev ? FVelmaraUITheme::Ash(0.35f) : (bFocused ? FVelmaraUITheme::BloodBright() : FVelmaraUITheme::BoneMuted());
		PrevArrow->SetBrush(FVelmaraUITheme::MakeImageBrush(ChevL, FVector2D(13.0f, 18.0f), PrevColor));
	}
	if (NextArrow)
	{
		const FLinearColor NextColor = !bCanNext ? FVelmaraUITheme::Ash(0.35f) : (bFocused ? FVelmaraUITheme::BloodBright() : FVelmaraUITheme::BoneMuted());
		NextArrow->SetBrush(FVelmaraUITheme::MakeImageBrush(ChevR, FVector2D(13.0f, 18.0f), NextColor));
	}
	if (BottomStar)
	{
		BottomStar->SetBrush(FVelmaraUITheme::MakeImageBrush(StarTex, FVector2D(12.0f, 12.0f), bFocused ? FVelmaraUITheme::Bone() : FVelmaraUITheme::BoneMuted(0.65f)));
	}
}

FReply UVelmaraSettingCarouselRow::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		const FVector2D ScreenPos = InMouseEvent.GetScreenSpacePosition();
		if (PrevHitBox && PrevHitBox->GetCachedGeometry().IsUnderLocation(ScreenPos))
		{
			SetKeyboardFocus();
			StepOption(-1);
			return FReply::Handled();
		}
		if (NextHitBox && NextHitBox->GetCachedGeometry().IsUnderLocation(ScreenPos))
		{
			SetKeyboardFocus();
			StepOption(1);
			return FReply::Handled();
		}
	}
	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

void UVelmaraSettingCarouselRow::NativeOnClicked()
{
	Super::NativeOnClicked();
	StepOption(1);
}

void UVelmaraSettingCarouselRow::NativeOnHovered()
{
	Super::NativeOnHovered();
	SetKeyboardFocus();
	OnRowHighlighted.Broadcast(Metadata);
	UpdateVisualState();
}

void UVelmaraSettingCarouselRow::NativeOnUnhovered()
{
	Super::NativeOnUnhovered();
	UpdateVisualState();
}

void UVelmaraSettingCarouselRow::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnAddedToFocusPath(InFocusEvent);
	bIsInFocusPath = true;
	OnRowHighlighted.Broadcast(Metadata);
	UpdateVisualState();
}

void UVelmaraSettingCarouselRow::NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnRemovedFromFocusPath(InFocusEvent);
	bIsInFocusPath = false;
	UpdateVisualState();
}

FReply UVelmaraSettingCarouselRow::NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent)
{
	const FReply Reply = Super::NativeOnFocusReceived(InGeometry, InFocusEvent);
	bIsInFocusPath = true;
	OnRowHighlighted.Broadcast(Metadata);
	UpdateVisualState();
	return Reply;
}

void UVelmaraSettingCarouselRow::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnFocusLost(InFocusEvent);
	bIsInFocusPath = false;
	UpdateVisualState();
}

FReply UVelmaraSettingCarouselRow::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Left || Key == EKeys::A || Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_LeftStick_Left)
	{
		StepOption(-1);
		return FReply::Handled();
	}
	if (Key == EKeys::Right || Key == EKeys::D || Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_LeftStick_Right)
	{
		StepOption(1);
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
