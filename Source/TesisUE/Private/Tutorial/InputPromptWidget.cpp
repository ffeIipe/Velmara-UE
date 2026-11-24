#include "Tutorial/InputPromptWidget.h"
#include "Blueprint/WidgetTree.h"
#include "CommonInputSubsystem.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "UI/Style/VelmaraUITheme.h"

TSharedRef<SWidget> UInputPromptWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultPromptTree();
	}
	return Super::RebuildWidget();
}

void UInputPromptWidget::BuildDefaultPromptTree()
{
	UVerticalBox* RootVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PromptRootVBox"));
	WidgetTree->RootWidget = RootVBox;

	USizeBox* EmblemSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PromptEmblemSize"));
	EmblemSize->SetWidthOverride(76.0f);
	EmblemSize->SetHeightOverride(76.0f);
	UOverlay* EmblemOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("PromptEmblemOverlay"));
	EmblemSize->SetContent(EmblemOverlay);

	UImage* FillBackdrop = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PromptDiamondBackdrop"));
	FillBackdrop->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::SlotDiamondFillMask),
		FVector2D(76.0f, 76.0f),
		FVelmaraUITheme::Void(0.76f)));
	if (UOverlaySlot* BgSlot = EmblemOverlay->AddChildToOverlay(FillBackdrop))
	{
		BgSlot->SetHorizontalAlignment(HAlign_Fill);
		BgSlot->SetVerticalAlignment(VAlign_Fill);
	}

	RadialProgressImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PromptRadialRing"));
	RadialProgressImage->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::RingProgress),
		FVector2D(76.0f, 76.0f),
		FVelmaraUITheme::BloodBright()));
	RadialProgressImage->SetVisibility(ESlateVisibility::Collapsed);
	if (UOverlaySlot* RingSlot = EmblemOverlay->AddChildToOverlay(RadialProgressImage))
	{
		RingSlot->SetHorizontalAlignment(HAlign_Fill);
		RingSlot->SetVerticalAlignment(VAlign_Fill);
	}

	DiamondFrameImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PromptDiamondFrame"));
	DiamondFrameImage->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::SlotDiamondNormal),
		FVector2D(76.0f, 76.0f),
		FVelmaraUITheme::Bone(0.92f)));
	if (UOverlaySlot* FrameSlot = EmblemOverlay->AddChildToOverlay(DiamondFrameImage))
	{
		FrameSlot->SetHorizontalAlignment(HAlign_Fill);
		FrameSlot->SetVerticalAlignment(VAlign_Fill);
	}

	USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PromptIconSize"));
	IconSize->SetWidthOverride(34.0f);
	IconSize->SetHeightOverride(34.0f);
	Image_Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_Icon"));
	IconSize->SetContent(Image_Icon);
	if (UOverlaySlot* IconSlot = EmblemOverlay->AddChildToOverlay(IconSize))
	{
		IconSlot->SetHorizontalAlignment(HAlign_Center);
		IconSlot->SetVerticalAlignment(VAlign_Center);
	}

	if (UVerticalBoxSlot* EmbSlot = RootVBox->AddChildToVerticalBox(EmblemSize))
	{
		EmbSlot->SetHorizontalAlignment(HAlign_Center);
		EmbSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	}

	Text_DisplayName = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_DisplayName"));
	Text_DisplayName->SetFont(FVelmaraUITheme::GetDisplayFont(15, 200));
	Text_DisplayName->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
	Text_DisplayName->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* TextSlot = RootVBox->AddChildToVerticalBox(Text_DisplayName))
	{
		TextSlot->SetHorizontalAlignment(HAlign_Center);
	}
}

void UInputPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ULocalPlayer* LP = GetOwningLocalPlayer())
	{
		if (UCommonInputSubsystem* InputSys = UCommonInputSubsystem::Get(LP))
		{
			InputMethodHandle = InputSys->OnInputMethodChangedNative.AddUObject(this, &UInputPromptWidget::HandleInputMethodChanged);
			if (bHasPromptData)
			{
				RefreshIconForInputType(InputSys->GetCurrentInputType() == ECommonInputType::Gamepad);
			}
		}
	}
}

void UInputPromptWidget::NativeDestruct()
{
	if (InputMethodHandle.IsValid())
	{
		if (ULocalPlayer* LP = GetOwningLocalPlayer())
		{
			if (UCommonInputSubsystem* InputSys = UCommonInputSubsystem::Get(LP))
			{
				InputSys->OnInputMethodChangedNative.Remove(InputMethodHandle);
			}
		}
		InputMethodHandle.Reset();
	}

	Super::NativeDestruct();
}

void UInputPromptWidget::InitializePrompt(const FInputPromptData& PromptData, bool bUsingGamepad)
{
	CachedPromptData = PromptData;
	bHasPromptData = true;

	bool bLiveGamepad = bUsingGamepad;
	if (ULocalPlayer* LP = GetOwningLocalPlayer())
	{
		if (const UCommonInputSubsystem* InputSys = UCommonInputSubsystem::Get(LP))
		{
			bLiveGamepad = (InputSys->GetCurrentInputType() == ECommonInputType::Gamepad);
		}
	}

	if (Text_DisplayName)
	{
		Text_DisplayName->SetText(PromptData.DisplayText.ToUpper());
		Text_DisplayName->SetFont(FVelmaraUITheme::GetDisplayFont(15, 200));
		Text_DisplayName->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
	}

	RefreshIconForInputType(bLiveGamepad);
}

void UInputPromptWidget::RefreshIconForInputType(bool bUsingGamepad)
{
	if (!Image_Icon || !bHasPromptData)
	{
		return;
	}

	UTexture2D* Icon = bUsingGamepad ? CachedPromptData.GamepadIcon : CachedPromptData.KeyboardIcon;
	if (!Icon)
	{
		Icon = CachedPromptData.KeyboardIcon ? CachedPromptData.KeyboardIcon : CachedPromptData.GamepadIcon;
	}

	if (Icon)
	{
		Image_Icon->SetBrush(FVelmaraUITheme::MakeImageBrush(Icon, FVector2D(34.0f, 34.0f), FVelmaraUITheme::Bone()));
	}
}

void UInputPromptWidget::SetHoldProgress(float NormalizedProgress)
{
	if (!RadialProgressImage)
	{
		return;
	}

	const float Clamped = FMath::Clamp(NormalizedProgress, 0.0f, 1.0f);
	if (Clamped <= 0.001f)
	{
		RadialProgressImage->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		RadialProgressImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		RadialProgressImage->SetRenderOpacity(0.35f + 0.65f * Clamped);
		const float Scale = 0.85f + 0.15f * Clamped;
		RadialProgressImage->SetRenderScale(FVector2D(Scale, Scale));
	}
}

void UInputPromptWidget::HandleInputMethodChanged(ECommonInputType NewInputType)
{
	if (bHasPromptData)
	{
		RefreshIconForInputType(NewInputType == ECommonInputType::Gamepad);
	}
}
