#include "UI/Widgets/Common/VelmaraButtonBase.h"
#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "UI/Style/VelmaraUITheme.h"
#include "UI/Widgets/Common/VelmaraCommonStyles.h"

UVelmaraButtonBase::UVelmaraButtonBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Style = UVelmaraTransparentButtonStyle::StaticClass();
	SetIsFocusable(true);
	bSelectable = false;
	bInteractableWhenSelected = true;
}

UCommonButtonInternalBase* UVelmaraButtonBase::ConstructInternalButton()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildButtonTree();
	}
	return Super::ConstructInternalButton();
}

TSharedRef<SWidget> UVelmaraButtonBase::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildButtonTree();
	}
	return Super::RebuildWidget();
}

void UVelmaraButtonBase::NativeConstruct()
{
	Super::NativeConstruct();
	UpdateVisualState();
}

void UVelmaraButtonBase::BuildButtonTree()
{
	RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ButtonSizeBox"));
	RootSizeBox->SetMinDesiredHeight(ButtonMinHeight);
	if (ButtonMinWidth > 0.0f)
	{
		RootSizeBox->SetMinDesiredWidth(ButtonMinWidth);
	}
	WidgetTree->RootWidget = RootSizeBox;

	RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("ButtonOverlay"));
	RootSizeBox->SetContent(RootOverlay);

	SolidBackdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SolidBackdrop"));
	SolidBackdrop->SetPadding(FMargin(0.0f));
	if (UOverlaySlot* BackdropSlot = RootOverlay->AddChildToOverlay(SolidBackdrop))
	{
		BackdropSlot->SetHorizontalAlignment(HAlign_Fill);
		BackdropSlot->SetVerticalAlignment(VAlign_Fill);
	}

	BgGradient = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BgGradient"));
	BgGradient->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (UOverlaySlot* GradSlot = RootOverlay->AddChildToOverlay(BgGradient))
	{
		GradSlot->SetHorizontalAlignment(HAlign_Fill);
		GradSlot->SetVerticalAlignment(VAlign_Fill);
	}

	TopHairline = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TopHairline"));
	TopHairline->SetPadding(FMargin(0.0f));
	TopHairline->SetBrush(FVelmaraUITheme::MakeSolidBrush(FVelmaraUITheme::BloodBright(0.75f)));
	TopHairline->SetDesiredSizeScale(FVector2D(1.0f, 1.0f));
	USizeBox* TopLineBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("TopLineBox"));
	TopLineBox->SetHeightOverride(1.0f);
	TopLineBox->SetContent(TopHairline);
	if (UOverlaySlot* TopSlot = RootOverlay->AddChildToOverlay(TopLineBox))
	{
		TopSlot->SetHorizontalAlignment(HAlign_Fill);
		TopSlot->SetVerticalAlignment(VAlign_Top);
	}

	BottomHairline = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BottomHairline"));
	BottomHairline->SetPadding(FMargin(0.0f));
	BottomHairline->SetBrush(FVelmaraUITheme::MakeSolidBrush(FVelmaraUITheme::BloodBright(0.75f)));
	USizeBox* BottomLineBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BottomLineBox"));
	BottomLineBox->SetHeightOverride(1.0f);
	BottomLineBox->SetContent(BottomHairline);
	if (UOverlaySlot* BottomSlot = RootOverlay->AddChildToOverlay(BottomLineBox))
	{
		BottomSlot->SetHorizontalAlignment(HAlign_Fill);
		BottomSlot->SetVerticalAlignment(VAlign_Bottom);
	}

	FrameBorder = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("FrameBorder"));
	FrameBorder->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (UOverlaySlot* FrameSlot = RootOverlay->AddChildToOverlay(FrameBorder))
	{
		FrameSlot->SetHorizontalAlignment(HAlign_Fill);
		FrameSlot->SetVerticalAlignment(VAlign_Fill);
	}

	ContentRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ContentRow"));
	if (UOverlaySlot* ContentSlot = RootOverlay->AddChildToOverlay(ContentRow))
	{
		ContentSlot->SetHorizontalAlignment(bCenterAligned ? HAlign_Center : HAlign_Left);
		ContentSlot->SetVerticalAlignment(VAlign_Center);
		ContentSlot->SetPadding(FMargin(20.0f, 8.0f, 20.0f, 8.0f));
	}

	USizeBox* MarkerBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MarkerBox"));
	MarkerBox->SetWidthOverride(14.0f);
	MarkerBox->SetHeightOverride(14.0f);
	LeftMarker = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("LeftMarker"));
	MarkerBox->SetContent(LeftMarker);
	if (UHorizontalBoxSlot* MarkerSlot = ContentRow->AddChildToHorizontalBox(MarkerBox))
	{
		MarkerSlot->SetVerticalAlignment(VAlign_Center);
		MarkerSlot->SetPadding(FMargin(0.0f, 0.0f, 14.0f, 0.0f));
	}

	LabelText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("LabelText"));
	LabelText->SetText(ButtonLabel.ToUpper());
	LabelText->SetFont(FVelmaraUITheme::GetDisplayFont(LabelFontSize, 200));
	LabelText->SetJustification(bCenterAligned ? ETextJustify::Center : ETextJustify::Left);
	if (UHorizontalBoxSlot* LabelSlot = ContentRow->AddChildToHorizontalBox(LabelText))
	{
		LabelSlot->SetVerticalAlignment(VAlign_Center);
	}

	UpdateVisualState();
}

void UVelmaraButtonBase::SetButtonText(const FText& InText)
{
	ButtonLabel = InText;
	if (LabelText)
	{
		LabelText->SetText(ButtonLabel.ToUpper());
	}
}

void UVelmaraButtonBase::SetVariant(EVelmaraButtonVariant InVariant)
{
	Variant = InVariant;
	if (Variant == EVelmaraButtonVariant::HexAction)
	{
		bCenterAligned = true;
		ButtonMinHeight = FMath::Max(ButtonMinHeight, 52.0f);
	}
	else if (Variant == EVelmaraButtonVariant::FramedRow)
	{
		ButtonMinHeight = FMath::Max(ButtonMinHeight, 54.0f);
	}
	else if (Variant == EVelmaraButtonVariant::SidebarTab)
	{
		ButtonMinHeight = 48.0f;
	}

	if (RootSizeBox)
	{
		RootSizeBox->SetMinDesiredHeight(ButtonMinHeight);
	}
	if (ContentRow)
	{
		if (UOverlaySlot* ContentSlot = Cast<UOverlaySlot>(ContentRow->Slot))
		{
			ContentSlot->SetHorizontalAlignment(bCenterAligned ? HAlign_Center : HAlign_Left);
		}
	}
	UpdateVisualState();
}

void UVelmaraButtonBase::SetFontSize(int32 InFontSize)
{
	LabelFontSize = InFontSize;
	if (LabelText)
	{
		LabelText->SetFont(FVelmaraUITheme::GetDisplayFont(LabelFontSize, 200));
	}
}

void UVelmaraButtonBase::SetButtonMinSize(float InMinWidth, float InMinHeight)
{
	ButtonMinWidth = InMinWidth;
	ButtonMinHeight = InMinHeight;
	if (RootSizeBox)
	{
		if (ButtonMinWidth > 0.0f)
		{
			RootSizeBox->SetMinDesiredWidth(ButtonMinWidth);
		}
		if (ButtonMinHeight > 0.0f)
		{
			RootSizeBox->SetMinDesiredHeight(ButtonMinHeight);
		}
	}
}

void UVelmaraButtonBase::SetCenterAligned(bool bInCenterAligned)
{
	bCenterAligned = bInCenterAligned;
	if (ContentRow)
	{
		if (UOverlaySlot* ContentSlot = Cast<UOverlaySlot>(ContentRow->Slot))
		{
			ContentSlot->SetHorizontalAlignment(bCenterAligned ? HAlign_Center : HAlign_Left);
		}
	}
	if (LabelText)
	{
		LabelText->SetJustification(bCenterAligned ? ETextJustify::Center : ETextJustify::Left);
	}
}

void UVelmaraButtonBase::SetHighlightedState(bool bInHighlighted)
{
	bExternalHighlight = bInHighlighted;
	UpdateVisualState();
}

bool UVelmaraButtonBase::IsVisuallyFocused() const
{
	return IsHovered() || HasAnyUserFocus() || HasKeyboardFocus() || bIsInFocusPath;
}

void UVelmaraButtonBase::UpdateVisualState()
{
	if (!LabelText || !BgGradient || !FrameBorder || !LeftMarker || !SolidBackdrop)
	{
		return;
	}

	const bool bEnabled = GetIsEnabled() && IsInteractionEnabled();
	const bool bFocused = bEnabled && IsVisuallyFocused();
	const bool bIsSelected = GetSelected() || bExternalHighlight;
	const bool bActive = bFocused || bIsSelected;
	const bool bPressed = bEnabled && bIsButtonPressed;

	const TCHAR* GradPath = (Variant == EVelmaraButtonVariant::HexAction)
		? VelmaraUIPaths::GradHFadeCenter
		: VelmaraUIPaths::GradHFadeLeftToRight;
	UTexture2D* GradTex = FVelmaraUITheme::LoadTexture(GradPath);
	float GradAlpha = 0.0f;
	if (bPressed)
	{
		GradAlpha = 0.95f;
	}
	else if (bFocused)
	{
		GradAlpha = 0.78f;
	}
	else if (bIsSelected)
	{
		GradAlpha = 0.62f;
	}

	if (GradTex && GradAlpha > 0.0f)
	{
		BgGradient->SetBrush(FVelmaraUITheme::MakeImageBrush(GradTex, FVector2D(512.0f, 8.0f), FVelmaraUITheme::Blood(GradAlpha)));
		BgGradient->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else
	{
		BgGradient->SetVisibility(ESlateVisibility::Hidden);
	}

	if (Variant == EVelmaraButtonVariant::FramedRow)
	{
		SolidBackdrop->SetBrush(FVelmaraUITheme::MakeSolidBrush(bActive ? FVelmaraUITheme::BloodDried(0.42f) : FVelmaraUITheme::Charcoal(0.68f)));
		FrameBorder->SetBrush(FVelmaraUITheme::MakeOptionRowBrush(bActive ? FVelmaraUITheme::BloodBright() : FVelmaraUITheme::Ash()));
		FrameBorder->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		TopHairline->SetVisibility(ESlateVisibility::Collapsed);
		BottomHairline->SetVisibility(ESlateVisibility::Collapsed);
	}
	else if (Variant == EVelmaraButtonVariant::HexAction)
	{
		SolidBackdrop->SetBrush(FVelmaraUITheme::MakeSolidBrush(bActive ? FVelmaraUITheme::BloodDried(0.50f) : FVelmaraUITheme::Void(0.75f)));
		FrameBorder->SetBrush(FVelmaraUITheme::MakeHexButtonBrush(bActive ? FVelmaraUITheme::BloodBright() : FVelmaraUITheme::Ash()));
		FrameBorder->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		TopHairline->SetVisibility(ESlateVisibility::Collapsed);
		BottomHairline->SetVisibility(ESlateVisibility::Collapsed);
	}
	else if (Variant == EVelmaraButtonVariant::SidebarTab)
	{
		SolidBackdrop->SetBrush(FVelmaraUITheme::MakeSolidBrush(FLinearColor::Transparent));
		if (bActive)
		{
			UTexture2D* SideTex = FVelmaraUITheme::LoadTexture(VelmaraUIPaths::FrameSidebarActive);
			FrameBorder->SetBrush(FVelmaraUITheme::MakeImageBrush(SideTex, FVector2D(256.0f, 48.0f), FVelmaraUITheme::BloodBright()));
			FrameBorder->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			FrameBorder->SetVisibility(ESlateVisibility::Hidden);
		}
		TopHairline->SetVisibility(ESlateVisibility::Collapsed);
		BottomHairline->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		SolidBackdrop->SetBrush(FVelmaraUITheme::MakeSolidBrush(FLinearColor::Transparent));
		FrameBorder->SetVisibility(ESlateVisibility::Collapsed);
		const ESlateVisibility LineVis = bActive ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden;
		TopHairline->SetVisibility(LineVis);
		BottomHairline->SetVisibility(LineVis);
	}

	if (Variant == EVelmaraButtonVariant::SidebarTab || Variant == EVelmaraButtonVariant::HexAction)
	{
		UTexture2D* MarkerTex = FVelmaraUITheme::LoadTexture(bActive ? VelmaraUIPaths::MarkerDiamondSolid : VelmaraUIPaths::MarkerDiamondHollow);
		const FLinearColor MarkerColor = !bEnabled ? FVelmaraUITheme::Ash(0.4f) : (bActive ? FVelmaraUITheme::BloodBright() : FVelmaraUITheme::BoneMuted(0.75f));
		LeftMarker->SetBrush(FVelmaraUITheme::MakeImageBrush(MarkerTex, FVector2D(14.0f, 14.0f), MarkerColor));
		LeftMarker->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else
	{
		UTexture2D* MarkerTex = FVelmaraUITheme::LoadTexture(VelmaraUIPaths::MarkerDiamondSolid);
		LeftMarker->SetBrush(FVelmaraUITheme::MakeImageBrush(MarkerTex, FVector2D(14.0f, 14.0f), FVelmaraUITheme::BloodBright()));
		LeftMarker->SetVisibility(bActive ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	}

	FLinearColor TextColor = FVelmaraUITheme::BoneMuted();
	if (!bEnabled)
	{
		TextColor = FVelmaraUITheme::Ash(0.45f);
	}
	else if (bActive)
	{
		TextColor = FVelmaraUITheme::Bone();
	}
	LabelText->SetColorAndOpacity(FSlateColor(TextColor));
}

void UVelmaraButtonBase::NativeOnHovered()
{
	Super::NativeOnHovered();
	if (GetIsEnabled() && IsInteractionEnabled())
	{
		SetKeyboardFocus();
		OnButtonHighlighted.Broadcast(this);
	}
	UpdateVisualState();
}

void UVelmaraButtonBase::NativeOnUnhovered()
{
	Super::NativeOnUnhovered();
	bIsButtonPressed = false;
	UpdateVisualState();
}

void UVelmaraButtonBase::NativeOnPressed()
{
	Super::NativeOnPressed();
	bIsButtonPressed = true;
	UpdateVisualState();
}

void UVelmaraButtonBase::NativeOnReleased()
{
	Super::NativeOnReleased();
	bIsButtonPressed = false;
	UpdateVisualState();
}

void UVelmaraButtonBase::NativeOnSelected(bool bBroadcast)
{
	Super::NativeOnSelected(bBroadcast);
	UpdateVisualState();
}

void UVelmaraButtonBase::NativeOnDeselected(bool bBroadcast)
{
	Super::NativeOnDeselected(bBroadcast);
	UpdateVisualState();
}

void UVelmaraButtonBase::NativeOnEnabled()
{
	Super::NativeOnEnabled();
	UpdateVisualState();
}

void UVelmaraButtonBase::NativeOnDisabled()
{
	Super::NativeOnDisabled();
	UpdateVisualState();
}

void UVelmaraButtonBase::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnAddedToFocusPath(InFocusEvent);
	bIsInFocusPath = true;
	OnButtonHighlighted.Broadcast(this);
	UpdateVisualState();
}

void UVelmaraButtonBase::NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnRemovedFromFocusPath(InFocusEvent);
	bIsInFocusPath = false;
	UpdateVisualState();
}

FReply UVelmaraButtonBase::NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent)
{
	const FReply Reply = Super::NativeOnFocusReceived(InGeometry, InFocusEvent);
	bIsInFocusPath = true;
	OnButtonHighlighted.Broadcast(this);
	UpdateVisualState();
	return Reply;
}

void UVelmaraButtonBase::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnFocusLost(InFocusEvent);
	bIsInFocusPath = false;
	UpdateVisualState();
}
