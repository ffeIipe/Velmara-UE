#include "UI/Widgets/Menus/VelmaraShopMenuWidget.h"
#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/BackgroundBlur.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressionComponent.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/DataTable.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Player/CustomPlayerController.h"
#include "SceneEvents/VelmaraGameInstance.h"
#include "UI/Layout/VelmaraPrimaryGameLayout.h"
#include "UI/Style/VelmaraUITheme.h"
#include "UI/Widgets/Common/VelmaraBracketPanel.h"
#include "UI/Widgets/Common/VelmaraButtonBase.h"
#include "UI/Widgets/Common/VelmaraCommonStyles.h"
#include "UI/Widgets/Common/VelmaraFooterBar.h"

UVelmaraShopRowButton::UVelmaraShopRowButton(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetStyle(UVelmaraTransparentButtonStyle::StaticClass());
	SetIsFocusable(true);
}

void UVelmaraShopRowButton::ConfigureRow(int32 InRowIndex)
{
	RowIndex = InRowIndex;
}

UCommonButtonInternalBase* UVelmaraShopRowButton::ConstructInternalButton()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildRowTree();
	}
	return Super::ConstructInternalButton();
}

TSharedRef<SWidget> UVelmaraShopRowButton::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildRowTree();
	}
	return Super::RebuildWidget();
}

void UVelmaraShopRowButton::BuildRowTree()
{
	UHorizontalBox* OuterRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ShopRowOuter"));
	WidgetTree->RootWidget = OuterRow;

	USizeBox* StarBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ShopRowStarBox"));
	StarBox->SetWidthOverride(22.0f);
	StarBox->SetHeightOverride(22.0f);
	FocusStarImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ShopRowStarImg"));
	FocusStarImage->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnRowStar),
		FVector2D(20.0f, 20.0f),
		FVelmaraUITheme::Bone()));
	FocusStarImage->SetVisibility(ESlateVisibility::Hidden);
	StarBox->SetContent(FocusStarImage);
	if (UHorizontalBoxSlot* StarSlot = OuterRow->AddChildToHorizontalBox(StarBox))
	{
		StarSlot->SetVerticalAlignment(VAlign_Center);
		StarSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
	}

	USizeBox* CardSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ShopRowCardSize"));
	CardSize->SetHeightOverride(76.0f);
	if (UHorizontalBoxSlot* CardWrapSlot = OuterRow->AddChildToHorizontalBox(CardSize))
	{
		CardWrapSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		CardWrapSlot->SetVerticalAlignment(VAlign_Center);
	}

	UOverlay* CardOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("ShopRowOverlay"));
	CardSize->SetContent(CardOverlay);

	FillBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ShopRowFill"));
	FillBorder->SetBrush(FVelmaraUITheme::MakeSolidBrush(FVelmaraUITheme::Charcoal(0.74f)));
	if (UOverlaySlot* FillSlot = CardOverlay->AddChildToOverlay(FillBorder))
	{
		FillSlot->SetHorizontalAlignment(HAlign_Fill);
		FillSlot->SetVerticalAlignment(VAlign_Fill);
	}

	HoverWashImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ShopRowWash"));
	HoverWashImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	HoverWashImage->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::GradHFadeLeftToRight),
		FVector2D(64.0f, 64.0f),
		FVelmaraUITheme::Blood(0.55f)));
	HoverWashImage->SetRenderOpacity(0.0f);
	if (UOverlaySlot* WashSlot = CardOverlay->AddChildToOverlay(HoverWashImage))
	{
		WashSlot->SetHorizontalAlignment(HAlign_Fill);
		WashSlot->SetVerticalAlignment(VAlign_Fill);
	}

	FrameBorderImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ShopRowFrame"));
	FrameBorderImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	FrameBorderImage->SetBrush(FVelmaraUITheme::MakeOptionRowBrush(FVelmaraUITheme::Ash()));
	if (UOverlaySlot* FrameSlot = CardOverlay->AddChildToOverlay(FrameBorderImage))
	{
		FrameSlot->SetHorizontalAlignment(HAlign_Fill);
		FrameSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UHorizontalBox* ContentRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ShopRowContent"));
	if (UOverlaySlot* ContentSlot = CardOverlay->AddChildToOverlay(ContentRow))
	{
		ContentSlot->SetHorizontalAlignment(HAlign_Fill);
		ContentSlot->SetVerticalAlignment(VAlign_Center);
		ContentSlot->SetPadding(FMargin(20.0f, 0.0f, 18.0f, 0.0f));
	}

	USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ShopRowIconSize"));
	IconSize->SetWidthOverride(36.0f);
	IconSize->SetHeightOverride(36.0f);
	IconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ShopRowIconImg"));
	IconSize->SetContent(IconImage);
	if (UHorizontalBoxSlot* ISlot = ContentRow->AddChildToHorizontalBox(IconSize))
	{
		ISlot->SetVerticalAlignment(VAlign_Center);
		ISlot->SetPadding(FMargin(0.0f, 0.0f, 16.0f, 0.0f));
	}

	USizeBox* LDivSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	LDivSize->SetWidthOverride(1.0f);
	LDivSize->SetHeightOverride(34.0f);
	UBorder* LDiv = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	LDiv->SetBrush(FVelmaraUITheme::MakeSolidBrush(FVelmaraUITheme::Ash(0.75f)));
	LDivSize->SetContent(LDiv);
	if (UHorizontalBoxSlot* LDivSlot = ContentRow->AddChildToHorizontalBox(LDivSize))
	{
		LDivSlot->SetVerticalAlignment(VAlign_Center);
		LDivSlot->SetPadding(FMargin(0.0f, 0.0f, 18.0f, 0.0f));
	}

	NameText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("ShopRowName"));
	NameText->SetFont(FVelmaraUITheme::GetDisplayFont(18, 140));
	NameText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
	if (UHorizontalBoxSlot* NameSlot = ContentRow->AddChildToHorizontalBox(NameText))
	{
		NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		NameSlot->SetVerticalAlignment(VAlign_Center);
		NameSlot->SetPadding(FMargin(0.0f, 0.0f, 16.0f, 0.0f));
	}

	UHorizontalBox* StateBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	USizeBox* DiaSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	DiaSize->SetWidthOverride(14.0f);
	DiaSize->SetHeightOverride(14.0f);
	StateDiamondImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	DiaSize->SetContent(StateDiamondImage);
	if (UHorizontalBoxSlot* DSlot = StateBox->AddChildToHorizontalBox(DiaSize))
	{
		DSlot->SetVerticalAlignment(VAlign_Center);
		DSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
	}

	StateText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass());
	StateText->SetFont(FVelmaraUITheme::GetDisplayFont(12, 140));
	StateBox->AddChildToHorizontalBox(StateText)->SetVerticalAlignment(VAlign_Center);

	USizeBox* StateWrap = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	StateWrap->SetMinDesiredWidth(134.0f);
	StateWrap->SetContent(StateBox);
	if (UHorizontalBoxSlot* SWSlot = ContentRow->AddChildToHorizontalBox(StateWrap))
	{
		SWSlot->SetVerticalAlignment(VAlign_Center);
		SWSlot->SetPadding(FMargin(0.0f, 0.0f, 14.0f, 0.0f));
	}

	USizeBox* RDivSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	RDivSize->SetWidthOverride(1.0f);
	RDivSize->SetHeightOverride(34.0f);
	UBorder* RDiv = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	RDiv->SetBrush(FVelmaraUITheme::MakeSolidBrush(FVelmaraUITheme::Ash(0.75f)));
	RDivSize->SetContent(RDiv);
	if (UHorizontalBoxSlot* RDivSlot = ContentRow->AddChildToHorizontalBox(RDivSize))
	{
		RDivSlot->SetVerticalAlignment(VAlign_Center);
		RDivSlot->SetPadding(FMargin(0.0f, 0.0f, 16.0f, 0.0f));
	}

	USizeBox* CostWrap = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	CostWrap->SetMinDesiredWidth(104.0f);
	UHorizontalBox* CostBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	CostWrap->SetContent(CostBox);

	USizeBox* BloodSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	BloodSize->SetWidthOverride(16.0f);
	BloodSize->SetHeightOverride(16.0f);
	CostBloodIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	CostBloodIcon->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::IconStatBlood),
		FVector2D(16.0f, 16.0f),
		FVelmaraUITheme::Bone()));
	BloodSize->SetContent(CostBloodIcon);
	if (UHorizontalBoxSlot* BSlot = CostBox->AddChildToHorizontalBox(BloodSize))
	{
		BSlot->SetVerticalAlignment(VAlign_Center);
		BSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
	}

	CostText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass());
	CostText->SetFont(FVelmaraUITheme::GetDisplayFont(16, 80));
	if (UHorizontalBoxSlot* CTSlot = CostBox->AddChildToHorizontalBox(CostText))
	{
		CTSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		CTSlot->SetVerticalAlignment(VAlign_Center);
	}

	USizeBox* ChevSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	ChevSize->SetWidthOverride(12.0f);
	ChevSize->SetHeightOverride(16.0f);
	ChevronImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	ChevronImage->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnChevronR),
		FVector2D(12.0f, 16.0f),
		FVelmaraUITheme::BoneMuted()));
	ChevSize->SetContent(ChevronImage);
	if (UHorizontalBoxSlot* ChSlot = CostBox->AddChildToHorizontalBox(ChevSize))
	{
		ChSlot->SetVerticalAlignment(VAlign_Center);
		ChSlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
	}

	ContentRow->AddChildToHorizontalBox(CostWrap)->SetVerticalAlignment(VAlign_Center);
}

void UVelmaraShopRowButton::SetRowData(const FShopItemData& InData, EVelmaraShopItemState InState, int32 InStatLevel)
{
	ItemState = InState;

	if (IconImage)
	{
		UTexture2D* Tex = InData.Icon ? InData.Icon : FVelmaraUITheme::LoadTexture(VelmaraUIPaths::IconItemVial);
		IconImage->SetBrush(FVelmaraUITheme::MakeImageBrush(Tex, FVector2D(34.0f, 34.0f), FVelmaraUITheme::Bone()));
	}

	if (NameText)
	{
		NameText->SetText(InData.DisplayName.ToUpper());
	}

	UTexture2D* FilledDia = FVelmaraUITheme::LoadTexture(VelmaraUIPaths::PipDiamondFilled);
	UTexture2D* HollowDia = FVelmaraUITheme::LoadTexture(VelmaraUIPaths::PipDiamondHollow);

	if (StateDiamondImage && StateText)
	{
		switch (ItemState)
		{
		case EVelmaraShopItemState::Owned:
			StateDiamondImage->SetBrush(FVelmaraUITheme::MakeImageBrush(FilledDia, FVector2D(14.0f, 14.0f), FVelmaraUITheme::Bone()));
			StateText->SetText(NSLOCTEXT("VelmaraUI", "ShopStateOwned", "ADQUIRIDO"));
			StateText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
			break;
		case EVelmaraShopItemState::Locked:
			StateDiamondImage->SetBrush(FVelmaraUITheme::MakeImageBrush(HollowDia, FVector2D(14.0f, 14.0f), FVelmaraUITheme::Ash()));
			StateText->SetText(NSLOCTEXT("VelmaraUI", "ShopStateLocked", "BLOQUEADO"));
			StateText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted(0.55f)));
			break;
		case EVelmaraShopItemState::InsufficientCurrency:
			StateDiamondImage->SetBrush(FVelmaraUITheme::MakeImageBrush(HollowDia, FVector2D(14.0f, 14.0f), FVelmaraUITheme::BloodBright()));
			StateText->SetText(InStatLevel > 0
				? FText::Format(NSLOCTEXT("VelmaraUI", "ShopStateLvl", "NIVEL {0}"), FText::FromString(FVelmaraUITheme::FormatSlotRomanNumeral(InStatLevel - 1)))
				: NSLOCTEXT("VelmaraUI", "ShopStateNoBlood", "SANGRE INSUF."));
			StateText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BloodBright()));
			break;
		default:
			StateDiamondImage->SetBrush(FVelmaraUITheme::MakeImageBrush(FilledDia, FVector2D(14.0f, 14.0f), FVelmaraUITheme::BloodBright()));
			StateText->SetText(InStatLevel > 0
				? FText::Format(NSLOCTEXT("VelmaraUI", "ShopStateLvlAvail", "NIVEL {0}"), InStatLevel)
				: NSLOCTEXT("VelmaraUI", "ShopStateAvail", "DISPONIBLE"));
			StateText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BloodBright()));
			break;
		}
	}

	if (CostBloodIcon && CostText && ChevronImage)
	{
		if (ItemState == EVelmaraShopItemState::Owned)
		{
			CostBloodIcon->SetVisibility(ESlateVisibility::Collapsed);
			CostText->SetText(NSLOCTEXT("VelmaraUI", "ShopOwnedCost", "ACTIVO"));
			CostText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted(0.75f)));
			ChevronImage->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			CostBloodIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			CostText->SetText(FVelmaraUITheme::FormatGroupedNumber(InData.BaseCost));
			CostText->SetColorAndOpacity(FSlateColor(
				ItemState == EVelmaraShopItemState::Available ? FVelmaraUITheme::Bone() : FVelmaraUITheme::BloodBright()));
			ChevronImage->SetVisibility(ItemState == EVelmaraShopItemState::Available ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
	}

	RefreshVisuals();
}

void UVelmaraShopRowButton::SetRowSelected(bool bInSelected)
{
	bRowSelected = bInSelected;
	RefreshVisuals();
}

void UVelmaraShopRowButton::RefreshVisuals()
{
	const bool bActive = bVisualHovered || bVisualFocused || bRowSelected;

	if (FocusStarImage)
	{
		FocusStarImage->SetVisibility(bActive ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	}

	if (HoverWashImage)
	{
		HoverWashImage->SetRenderOpacity(bActive ? 1.0f : 0.0f);
	}

	if (FrameBorderImage)
	{
		FrameBorderImage->SetBrush(FVelmaraUITheme::MakeOptionRowBrush(
			bActive ? FVelmaraUITheme::BloodBright() : FVelmaraUITheme::Ash()));
	}

	if (FillBorder)
	{
		FillBorder->SetBrush(FVelmaraUITheme::MakeSolidBrush(
			bActive ? FVelmaraUITheme::BloodDried(0.45f) : FVelmaraUITheme::Charcoal(0.74f)));
	}

	if (ChevronImage)
	{
		ChevronImage->SetColorAndOpacity(bActive ? FVelmaraUITheme::BloodBright() : FVelmaraUITheme::BoneMuted());
	}
}

void UVelmaraShopRowButton::NativeOnHovered()
{
	Super::NativeOnHovered();
	bVisualHovered = true;
	SetKeyboardFocus();
	RefreshVisuals();
	OnRowFocusedOrHovered.Broadcast(RowIndex);
}

void UVelmaraShopRowButton::NativeOnUnhovered()
{
	Super::NativeOnUnhovered();
	bVisualHovered = false;
	RefreshVisuals();
}

void UVelmaraShopRowButton::NativeOnClicked()
{
	Super::NativeOnClicked();
	OnRowActivated.Broadcast(RowIndex);
}

void UVelmaraShopRowButton::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnAddedToFocusPath(InFocusEvent);
	bVisualFocused = true;
	RefreshVisuals();
	OnRowFocusedOrHovered.Broadcast(RowIndex);
}

void UVelmaraShopRowButton::NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnRemovedFromFocusPath(InFocusEvent);
	bVisualFocused = false;
	RefreshVisuals();
}

UVelmaraShopMenuWidget::UVelmaraShopMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = true;
	bSupportsActivationFocus = true;
	bAutoActivate = true;
}

TOptional<FUIInputConfig> UVelmaraShopMenuWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture, false);
}

TSharedRef<SWidget> UVelmaraShopMenuWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildScreenTree();
	}
	return Super::RebuildWidget();
}

void UVelmaraShopMenuWidget::BuildScreenTree()
{
	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("ShopRootOverlay"));
	WidgetTree->RootWidget = RootOverlay;

	UBackgroundBlur* Blur = WidgetTree->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass(), TEXT("ShopBlur"));
	Blur->SetBlurStrength(12.0f);
	if (UOverlaySlot* BlurSlot = RootOverlay->AddChildToOverlay(Blur))
	{
		BlurSlot->SetHorizontalAlignment(HAlign_Fill);
		BlurSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UBorder* DimBg = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ShopDimBg"));
	DimBg->SetBrush(FVelmaraUITheme::MakeSolidBrush(FVelmaraUITheme::Void(0.80f)));
	if (UOverlaySlot* DimSlot = RootOverlay->AddChildToOverlay(DimBg))
	{
		DimSlot->SetHorizontalAlignment(HAlign_Fill);
		DimSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UVerticalBox* MainStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ShopMainStack"));
	if (UOverlaySlot* MainSlot = RootOverlay->AddChildToOverlay(MainStack))
	{
		MainSlot->SetHorizontalAlignment(HAlign_Fill);
		MainSlot->SetVerticalAlignment(VAlign_Fill);
		MainSlot->SetPadding(FMargin(64.0f, 38.0f, 64.0f, 34.0f));
	}

	UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ShopHeaderRow"));
	if (UVerticalBoxSlot* HeadSlot = MainStack->AddChildToVerticalBox(HeaderRow))
	{
		HeadSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 24.0f));
	}

	UVerticalBox* TitleCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UCommonTextBlock* TitleText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("ShopTitle"));
	TitleText->SetText(NSLOCTEXT("VelmaraUI", "ShopHeader", "T I E N D A   D E   S A N G R E"));
	TitleText->SetFont(FVelmaraUITheme::GetDisplayFont(34, 220));
	TitleText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
	TitleCol->AddChildToVerticalBox(TitleText);

	USizeBox* TitleSepSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	TitleSepSize->SetWidthOverride(420.0f);
	TitleSepSize->SetHeightOverride(20.0f);
	UImage* TitleSep = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	TitleSep->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnSepStar),
		FVector2D(420.0f, 20.0f),
		FVelmaraUITheme::Bone(0.85f)));
	TitleSepSize->SetContent(TitleSep);
	if (UVerticalBoxSlot* SepSlot = TitleCol->AddChildToVerticalBox(TitleSepSize))
	{
		SepSlot->SetHorizontalAlignment(HAlign_Left);
		SepSlot->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 0.0f));
	}

	if (UHorizontalBoxSlot* TSlot = HeaderRow->AddChildToHorizontalBox(TitleCol))
	{
		TSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TSlot->SetVerticalAlignment(VAlign_Center);
	}

	UHorizontalBox* CurrencyBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	USizeBox* BloodIconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	BloodIconSize->SetWidthOverride(22.0f);
	BloodIconSize->SetHeightOverride(22.0f);
	UImage* BloodIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	BloodIcon->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::IconStatBlood),
		FVector2D(22.0f, 22.0f),
		FVelmaraUITheme::Bone()));
	BloodIconSize->SetContent(BloodIcon);
	if (UHorizontalBoxSlot* BSlot = CurrencyBox->AddChildToHorizontalBox(BloodIconSize))
	{
		BSlot->SetVerticalAlignment(VAlign_Center);
		BSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
	}

	CurrencyValueText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("ShopCurrencyText"));
	CurrencyValueText->SetFont(FVelmaraUITheme::GetDisplayFont(22, 160));
	CurrencyValueText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
	if (UHorizontalBoxSlot* VSlot = CurrencyBox->AddChildToHorizontalBox(CurrencyValueText))
	{
		VSlot->SetVerticalAlignment(VAlign_Center);
		VSlot->SetPadding(FMargin(0.0f, 0.0f, 18.0f, 0.0f));
	}

	USizeBox* CrestSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	CrestSize->SetWidthOverride(32.0f);
	CrestSize->SetHeightOverride(76.0f);
	UImage* CrestImg = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	CrestImg->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnCrestVelmara),
		FVector2D(32.0f, 76.0f),
		FVelmaraUITheme::BloodBright(0.92f)));
	CrestSize->SetContent(CrestImg);
	CurrencyBox->AddChildToHorizontalBox(CrestSize)->SetVerticalAlignment(VAlign_Center);

	HeaderRow->AddChildToHorizontalBox(CurrencyBox)->SetVerticalAlignment(VAlign_Center);

	UHorizontalBox* BodyColumns = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ShopBodyColumns"));
	if (UVerticalBoxSlot* BodySlot = MainStack->AddChildToVerticalBox(BodyColumns))
	{
		BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		BodySlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
	}

	UVerticalBox* LeftCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	if (UHorizontalBoxSlot* LSlot = BodyColumns->AddChildToHorizontalBox(LeftCol))
	{
		LSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		LSlot->SetPadding(FMargin(0.0f, 0.0f, 40.0f, 0.0f));
	}

	EmptyMessageText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("ShopEmptyMsg"));
	EmptyMessageText->SetText(NSLOCTEXT("VelmaraUI", "ShopEmpty", "No hay ofrendas disponibles en este momento."));
	EmptyMessageText->SetFont(FVelmaraUITheme::GetBodyFont(18, 0));
	EmptyMessageText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted()));
	EmptyMessageText->SetVisibility(ESlateVisibility::Collapsed);
	LeftCol->AddChildToVerticalBox(EmptyMessageText)->SetPadding(FMargin(24.0f, 12.0f, 0.0f, 12.0f));

	UScrollBox* RowsScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ShopRowsScroll"));
	RowsScroll->SetConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible);
	if (UVerticalBoxSlot* ScSlot = LeftCol->AddChildToVerticalBox(RowsScroll))
	{
		ScSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	RowsContainer = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ShopRowsContainer"));
	RowsScroll->AddChild(RowsContainer);

	USizeBox* RightPanelSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	RightPanelSize->SetWidthOverride(460.0f);
	DetailsPanel = WidgetTree->ConstructWidget<UVelmaraBracketPanel>(UVelmaraBracketPanel::StaticClass(), TEXT("ShopDetailsPanel"));
	DetailsPanel->SetUseMinimalBrackets(true);
	DetailsPanel->SetBackdropAlpha(0.78f);
	DetailsPanel->SetShowSignature(true);
	DetailsPanel->SetShowTopStar(true);
	RightPanelSize->SetContent(DetailsPanel);
	if (UHorizontalBoxSlot* RSlot = BodyColumns->AddChildToHorizontalBox(RightPanelSize))
	{
		RSlot->SetHorizontalAlignment(HAlign_Right);
		RSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UVerticalBox* DetailsContent = DetailsPanel->GetContentBox();

	UOverlay* IconBadgeOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	USizeBox* BadgeSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	BadgeSize->SetWidthOverride(84.0f);
	BadgeSize->SetHeightOverride(84.0f);
	BadgeSize->SetContent(IconBadgeOverlay);

	UImage* BadgeFrame = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	BadgeFrame->SetBrush(FVelmaraUITheme::MakeSquareSlotNormalBrush(FVelmaraUITheme::Bone(0.75f)));
	if (UOverlaySlot* BFSlot = IconBadgeOverlay->AddChildToOverlay(BadgeFrame))
	{
		BFSlot->SetHorizontalAlignment(HAlign_Fill);
		BFSlot->SetVerticalAlignment(VAlign_Fill);
	}

	DetailIconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ShopDetailIcon"));
	if (UOverlaySlot* DISlot = IconBadgeOverlay->AddChildToOverlay(DetailIconImage))
	{
		DISlot->SetHorizontalAlignment(HAlign_Center);
		DISlot->SetVerticalAlignment(VAlign_Center);
		DISlot->SetPadding(FMargin(14.0f));
	}

	if (UVerticalBoxSlot* BadgeWrapSlot = DetailsContent->AddChildToVerticalBox(BadgeSize))
	{
		BadgeWrapSlot->SetHorizontalAlignment(HAlign_Center);
		BadgeWrapSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
	}

	DetailStatusText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("ShopDetailStatus"));
	DetailStatusText->SetFont(FVelmaraUITheme::GetDisplayFont(12, 200));
	DetailStatusText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BloodBright()));
	if (UVerticalBoxSlot* StSlot = DetailsContent->AddChildToVerticalBox(DetailStatusText))
	{
		StSlot->SetHorizontalAlignment(HAlign_Center);
		StSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	}

	DetailTitleText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("ShopDetailTitle"));
	DetailTitleText->SetFont(FVelmaraUITheme::GetDisplayFont(24, 180));
	DetailTitleText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
	DetailTitleText->SetJustification(ETextJustify::Center);
	DetailTitleText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* TSlot = DetailsContent->AddChildToVerticalBox(DetailTitleText))
	{
		TSlot->SetHorizontalAlignment(HAlign_Center);
		TSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	USizeBox* DetSepSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	DetSepSize->SetHeightOverride(16.0f);
	UImage* DetSep = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	DetSep->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnSepDiamond),
		FVector2D(340.0f, 16.0f),
		FVelmaraUITheme::BoneMuted(0.8f)));
	DetSepSize->SetContent(DetSep);
	if (UVerticalBoxSlot* SepSlot = DetailsContent->AddChildToVerticalBox(DetSepSize))
	{
		SepSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
	}

	DetailDescText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("ShopDetailDesc"));
	DetailDescText->SetFont(FVelmaraUITheme::GetBodyFont(17, 0));
	DetailDescText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone(0.90f)));
	DetailDescText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* DSlot = DetailsContent->AddChildToVerticalBox(DetailDescText))
	{
		DSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
	}

	DetailRequirementText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("ShopDetailReq"));
	DetailRequirementText->SetFont(FVelmaraUITheme::GetBoldBodyFont(14, 0));
	DetailRequirementText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BloodBright()));
	DetailRequirementText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* ReqSlot = DetailsContent->AddChildToVerticalBox(DetailRequirementText))
	{
		ReqSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	DetailCostText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("ShopDetailCost"));
	DetailCostText->SetFont(FVelmaraUITheme::GetBodyFont(16, 0));
	DetailCostText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted()));
	DetailCostText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* CSlot = DetailsContent->AddChildToVerticalBox(DetailCostText))
	{
		CSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	DetailFeedbackText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("ShopDetailFeedback"));
	DetailFeedbackText->SetFont(FVelmaraUITheme::GetBoldBodyFont(15, 0));
	DetailFeedbackText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* FbSlot = DetailsContent->AddChildToVerticalBox(DetailFeedbackText))
	{
		FbSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	USpacer* DetSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	if (UVerticalBoxSlot* SpSlot = DetailsContent->AddChildToVerticalBox(DetSpacer))
	{
		SpSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	BuyButton = WidgetTree->ConstructWidget<UVelmaraButtonBase>(UVelmaraButtonBase::StaticClass(), TEXT("ShopBuyBtn"));
	BuyButton->SetVariant(EVelmaraButtonVariant::HexAction);
	BuyButton->SetFontSize(16);
	BuyButton->SetButtonMinSize(220.0f, 46.0f);
	BuyButton->SetButtonText(NSLOCTEXT("VelmaraUI", "ShopBtnBuy", "ADQUIRIR"));
	BuyButton->OnClicked().AddUObject(this, &UVelmaraShopMenuWidget::HandleBuyButtonClicked);
	if (UVerticalBoxSlot* BuySlot = DetailsContent->AddChildToVerticalBox(BuyButton))
	{
		BuySlot->SetHorizontalAlignment(HAlign_Center);
		BuySlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 24.0f));
	}

	FooterBar = WidgetTree->ConstructWidget<UVelmaraFooterBar>(UVelmaraFooterBar::StaticClass(), TEXT("ShopFooterBar"));
	FooterBar->SetActions({
		FVelmaraFooterAction(TEXT("Confirm"), NSLOCTEXT("VelmaraUI", "ShopFooterBuy", "COMPRAR")),
		FVelmaraFooterAction(TEXT("Back"), NSLOCTEXT("VelmaraUI", "ShopFooterClose", "CERRAR"))
	});
	MainStack->AddChildToVerticalBox(FooterBar);
}

void UVelmaraShopMenuWidget::SetShopDataTable(UDataTable* InDataTable)
{
	ShopDataTable = InDataTable;
	RefreshShop();
}

void UVelmaraShopMenuWidget::ResolveProgressionComponent()
{
	if (UProgressionComponent* OldProg = ProgressionComp.Get())
	{
		OldProg->OnCurrencyChanged.RemoveDynamic(this, &UVelmaraShopMenuWidget::HandleCurrencyChanged);
		OldProg->OnComboUnlocked.RemoveDynamic(this, &UVelmaraShopMenuWidget::HandleComboUnlocked);
		OldProg->OnStatUpgraded.RemoveDynamic(this, &UVelmaraShopMenuWidget::HandleStatUpgraded);
	}

	ProgressionComp = nullptr;

	if (APlayerController* PC = GetOwningPlayer())
	{
		APawn* TargetPawn = PC->GetPawn();
		if (ACustomPlayerController* CustomPC = Cast<ACustomPlayerController>(PC))
		{
			if (CustomPC->GetOriginalBody())
			{
				TargetPawn = CustomPC->GetOriginalBody();
			}
		}
		if (TargetPawn)
		{
			ProgressionComp = TargetPawn->FindComponentByClass<UProgressionComponent>();
		}
	}

	if (UProgressionComponent* Prog = ProgressionComp.Get())
	{
		Prog->OnCurrencyChanged.AddUniqueDynamic(this, &UVelmaraShopMenuWidget::HandleCurrencyChanged);
		Prog->OnComboUnlocked.AddUniqueDynamic(this, &UVelmaraShopMenuWidget::HandleComboUnlocked);
		Prog->OnStatUpgraded.AddUniqueDynamic(this, &UVelmaraShopMenuWidget::HandleStatUpgraded);
	}
}

void UVelmaraShopMenuWidget::LoadShopEntries()
{
	ShopItems.Reset();

	UDataTable* Table = ShopDataTable ? ShopDataTable.Get() : FVelmaraUITheme::LoadShopItemDataTable();
	if (!Table)
	{
		return;
	}

	TArray<FShopItemData*> Rows;
	Table->GetAllRows<FShopItemData>(TEXT("VelmaraShopMenu"), Rows);
	for (const FShopItemData* Row : Rows)
	{
		if (Row)
		{
			ShopItems.Add(*Row);
		}
	}
}

EVelmaraShopItemState UVelmaraShopMenuWidget::EvaluateItemState(const FShopItemData& Item, int32& OutStatLevel) const
{
	OutStatLevel = 0;
	const UProgressionComponent* Prog = ProgressionComp.Get();
	if (!Prog)
	{
		return EVelmaraShopItemState::Locked;
	}

	if (Item.GrantedTag.IsValid())
	{
		if (Prog->HasComboUnlocked(Item.GrantedTag))
		{
			return EVelmaraShopItemState::Owned;
		}
		OutStatLevel = Prog->GetStatLevel(Item.GrantedTag);
	}

	if (Item.RequiredTag.IsValid())
	{
		const bool bHasReq = Prog->HasComboUnlocked(Item.RequiredTag) || Prog->GetStatLevel(Item.RequiredTag) > 0;
		if (!bHasReq)
		{
			return EVelmaraShopItemState::Locked;
		}
	}

	if (Prog->GetAvailableCurrency() < Item.BaseCost)
	{
		return EVelmaraShopItemState::InsufficientCurrency;
	}

	return EVelmaraShopItemState::Available;
}

void UVelmaraShopMenuWidget::RebuildRows()
{
	if (!RowsContainer) return;

	RowsContainer->ClearChildren();
	RowWidgets.Reset();

	if (EmptyMessageText)
	{
		EmptyMessageText->SetVisibility(ShopItems.IsEmpty() ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	for (int32 Index = 0; Index < ShopItems.Num(); ++Index)
	{
		UVelmaraShopRowButton* Row = WidgetTree->ConstructWidget<UVelmaraShopRowButton>(UVelmaraShopRowButton::StaticClass());
		if (!Row) continue;

		int32 StatLevel = 0;
		const EVelmaraShopItemState State = EvaluateItemState(ShopItems[Index], StatLevel);
		Row->ConfigureRow(Index);
		Row->SetRowData(ShopItems[Index], State, StatLevel);
		Row->SetRowSelected(Index == SelectedIndex);
		Row->OnRowFocusedOrHovered.AddUObject(this, &UVelmaraShopMenuWidget::HandleRowFocused);
		Row->OnRowActivated.AddUObject(this, &UVelmaraShopMenuWidget::HandleRowActivated);
		if (UVerticalBoxSlot* RSlot = RowsContainer->AddChildToVerticalBox(Row))
		{
			RSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
		}
		RowWidgets.Add(Row);
	}
}

void UVelmaraShopMenuWidget::RefreshShop()
{
	if (ShopItems.IsEmpty())
	{
		LoadShopEntries();
	}

	if (RowWidgets.Num() != ShopItems.Num())
	{
		RebuildRows();
	}
	else
	{
		for (int32 Index = 0; Index < RowWidgets.Num(); ++Index)
		{
			if (RowWidgets[Index] && ShopItems.IsValidIndex(Index))
			{
				int32 StatLevel = 0;
				const EVelmaraShopItemState State = EvaluateItemState(ShopItems[Index], StatLevel);
				RowWidgets[Index]->SetRowData(ShopItems[Index], State, StatLevel);
				RowWidgets[Index]->SetRowSelected(Index == SelectedIndex);
			}
		}
	}

	RefreshCurrencyHeader();
	RefreshDetailsPanel();
}

void UVelmaraShopMenuWidget::RefreshCurrencyHeader()
{
	if (!CurrencyValueText) return;

	const int32 Amount = ProgressionComp.IsValid() ? ProgressionComp->GetAvailableCurrency() : 0;
	CurrencyValueText->SetText(FText::Format(
		NSLOCTEXT("VelmaraUI", "ShopCurrencyFmt", "{0} SANGRE"),
		FVelmaraUITheme::FormatGroupedNumber(Amount)));
}

void UVelmaraShopMenuWidget::RefreshDetailsPanel()
{
	if (!ShopItems.IsValidIndex(SelectedIndex))
	{
		if (DetailStatusText) DetailStatusText->SetText(FText::GetEmpty());
		if (DetailTitleText) DetailTitleText->SetText(NSLOCTEXT("VelmaraUI", "ShopNoSelTitle", "TIENDA DE SANGRE"));
		if (DetailDescText) DetailDescText->SetText(NSLOCTEXT("VelmaraUI", "ShopNoSelDesc", "Selecciona una ofrenda para inspeccionar su poder."));
		if (DetailRequirementText) DetailRequirementText->SetVisibility(ESlateVisibility::Collapsed);
		if (DetailCostText) DetailCostText->SetText(FText::GetEmpty());
		if (BuyButton) BuyButton->SetIsEnabled(false);
		return;
	}

	const FShopItemData& Item = ShopItems[SelectedIndex];
	int32 StatLevel = 0;
	const EVelmaraShopItemState State = EvaluateItemState(Item, StatLevel);

	if (DetailIconImage)
	{
		UTexture2D* Tex = Item.Icon ? Item.Icon : FVelmaraUITheme::LoadTexture(VelmaraUIPaths::IconItemVial);
		DetailIconImage->SetBrush(FVelmaraUITheme::MakeImageBrush(Tex, FVector2D(52.0f, 52.0f), FVelmaraUITheme::Bone()));
	}

	if (DetailTitleText) DetailTitleText->SetText(Item.DisplayName.ToUpper());
	if (DetailDescText) DetailDescText->SetText(Item.Description);

	if (DetailStatusText)
	{
		switch (State)
		{
		case EVelmaraShopItemState::Owned:
			DetailStatusText->SetText(NSLOCTEXT("VelmaraUI", "ShopDetOwned", "YA ADQUIRIDO"));
			DetailStatusText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
			break;
		case EVelmaraShopItemState::Locked:
			DetailStatusText->SetText(NSLOCTEXT("VelmaraUI", "ShopDetLocked", "REQUISITO PENDIENTE"));
			DetailStatusText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted()));
			break;
		case EVelmaraShopItemState::InsufficientCurrency:
			DetailStatusText->SetText(NSLOCTEXT("VelmaraUI", "ShopDetNoBlood", "SANGRE INSUFICIENTE"));
			DetailStatusText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BloodBright()));
			break;
		default:
			DetailStatusText->SetText(NSLOCTEXT("VelmaraUI", "ShopDetAvail", "DISPONIBLE"));
			DetailStatusText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BloodBright()));
			break;
		}
	}

	if (DetailRequirementText)
	{
		if (State == EVelmaraShopItemState::Locked && Item.RequiredTag.IsValid())
		{
			DetailRequirementText->SetText(FText::Format(
				NSLOCTEXT("VelmaraUI", "ShopReqFmt", "Requiere: {0}"),
				FText::FromName(Item.RequiredTag.GetTagName())));
			DetailRequirementText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			DetailRequirementText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (DetailCostText)
	{
		const int32 Available = ProgressionComp.IsValid() ? ProgressionComp->GetAvailableCurrency() : 0;
		if (State == EVelmaraShopItemState::Owned)
		{
			DetailCostText->SetText(NSLOCTEXT("VelmaraUI", "ShopCostOwned", "Esta ofrenda ya pertenece a tu sangre."));
			DetailCostText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted()));
		}
		else
		{
			DetailCostText->SetText(FText::Format(
				NSLOCTEXT("VelmaraUI", "ShopCostFmt", "Costo: {0} Sangre  ·  Disponible: {1}"),
				FVelmaraUITheme::FormatGroupedNumber(Item.BaseCost),
				FVelmaraUITheme::FormatGroupedNumber(Available)));
			DetailCostText->SetColorAndOpacity(FSlateColor(
				State == EVelmaraShopItemState::Available ? FVelmaraUITheme::Bone() : FVelmaraUITheme::BloodBright()));
		}
	}

	if (BuyButton)
	{
		BuyButton->SetIsEnabled(State == EVelmaraShopItemState::Available);
		BuyButton->SetButtonText(State == EVelmaraShopItemState::Owned
			? NSLOCTEXT("VelmaraUI", "ShopBtnOwned", "ADQUIRIDO")
			: NSLOCTEXT("VelmaraUI", "ShopBtnBuy", "ADQUIRIR"));
	}
}

void UVelmaraShopMenuWidget::SelectRow(int32 RowIndex)
{
	if (!ShopItems.IsValidIndex(RowIndex)) return;

	SelectedIndex = RowIndex;
	for (int32 Index = 0; Index < RowWidgets.Num(); ++Index)
	{
		if (RowWidgets[Index])
		{
			RowWidgets[Index]->SetRowSelected(Index == SelectedIndex);
		}
	}
	RefreshDetailsPanel();
}

void UVelmaraShopMenuWidget::PurchaseSelected()
{
	if (!ShopItems.IsValidIndex(SelectedIndex)) return;

	UProgressionComponent* Prog = ProgressionComp.Get();
	const FShopItemData& Item = ShopItems[SelectedIndex];
	int32 StatLevel = 0;
	const EVelmaraShopItemState State = EvaluateItemState(Item, StatLevel);

	if (!Prog)
	{
		if (DetailFeedbackText)
		{
			DetailFeedbackText->SetText(NSLOCTEXT("VelmaraUI", "ShopErrNoProg", "No se encontró el componente de progresión."));
			DetailFeedbackText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BloodBright()));
		}
		return;
	}

	if (State == EVelmaraShopItemState::Owned)
	{
		if (DetailFeedbackText)
		{
			DetailFeedbackText->SetText(NSLOCTEXT("VelmaraUI", "ShopErrOwned", "Esta ofrenda ya fue adquirida."));
			DetailFeedbackText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted()));
		}
		return;
	}

	if (State == EVelmaraShopItemState::Locked)
	{
		if (DetailFeedbackText)
		{
			DetailFeedbackText->SetText(NSLOCTEXT("VelmaraUI", "ShopErrLocked", "Debes desbloquear primero el requisito previo."));
			DetailFeedbackText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BloodBright()));
		}
		return;
	}

	if (State == EVelmaraShopItemState::InsufficientCurrency)
	{
		if (DetailFeedbackText)
		{
			DetailFeedbackText->SetText(NSLOCTEXT("VelmaraUI", "ShopErrNoBlood", "No tienes suficiente Sangre para esta ofrenda."));
			DetailFeedbackText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BloodBright()));
		}
		return;
	}

	const bool bBought = Prog->TryBuyUpgrade(Item.GrantedTag, Item.BaseCost);
	if (DetailFeedbackText)
	{
		if (bBought)
		{
			DetailFeedbackText->SetText(FText::Format(
				NSLOCTEXT("VelmaraUI", "ShopSuccessFmt", "Ofrenda aceptada: {0}"),
				Item.DisplayName));
			DetailFeedbackText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
		}
		else
		{
			DetailFeedbackText->SetText(NSLOCTEXT("VelmaraUI", "ShopErrFailed", "No se pudo completar la compra."));
			DetailFeedbackText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BloodBright()));
		}
	}

	RefreshShop();
}

void UVelmaraShopMenuWidget::ApplyPauseState(bool bPaused)
{
	UGameplayStatics::SetGamePaused(GetWorld(), bPaused);
	if (UVelmaraGameInstance* GI = Cast<UVelmaraGameInstance>(GetGameInstance()))
	{
		GI->SetPauseAudioMixActive(bPaused);
	}
	if (ACustomPlayerController* CustomPC = Cast<ACustomPlayerController>(GetOwningPlayer()))
	{
		CustomPC->SetPauseAudioMixState(bPaused);
	}
}

void UVelmaraShopMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ResolveProgressionComponent();
	LoadShopEntries();
	RefreshShop();
}

void UVelmaraShopMenuWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	ResolveProgressionComponent();
	LoadShopEntries();
	RefreshShop();
	ApplyPauseState(true);

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetShowMouseCursor(true);
		FInputModeGameAndUI Mode;
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Mode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(Mode);
	}

	if (UWidget* Target = NativeGetDesiredFocusTarget())
	{
		Target->SetKeyboardFocus();
	}
}

void UVelmaraShopMenuWidget::NativeOnDeactivated()
{
	Super::NativeOnDeactivated();

	if (UProgressionComponent* Prog = ProgressionComp.Get())
	{
		Prog->OnCurrencyChanged.RemoveDynamic(this, &UVelmaraShopMenuWidget::HandleCurrencyChanged);
		Prog->OnComboUnlocked.RemoveDynamic(this, &UVelmaraShopMenuWidget::HandleComboUnlocked);
		Prog->OnStatUpgraded.RemoveDynamic(this, &UVelmaraShopMenuWidget::HandleStatUpgraded);
	}

	UVelmaraPrimaryGameLayout* Layout = UVelmaraPrimaryGameLayout::Get(this);
	const bool bStillHasMenu = Layout && Layout->HasActiveMenuOrModal();
	if (!bStillHasMenu)
	{
		ApplyPauseState(false);
		if (APlayerController* PC = GetOwningPlayer())
		{
			PC->SetShowMouseCursor(false);
			PC->SetInputMode(FInputModeGameOnly());
		}
	}

	if (!Layout || (Layout->GetMenuStack() && !Layout->GetMenuStack()->GetWidgetList().Contains(this)))
	{
		RemoveFromParent();
	}
}

UWidget* UVelmaraShopMenuWidget::NativeGetDesiredFocusTarget() const
{
	if (RowWidgets.IsValidIndex(SelectedIndex) && RowWidgets[SelectedIndex])
	{
		return RowWidgets[SelectedIndex];
	}
	return RowWidgets.Num() > 0 ? RowWidgets[0].Get() : nullptr;
}

bool UVelmaraShopMenuWidget::NativeOnHandleBackAction()
{
	DeactivateWidget();
	return true;
}

FReply UVelmaraShopMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
	{
		DeactivateWidget();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UVelmaraShopMenuWidget::HandleCurrencyChanged(int32 NewAmount)
{
	RefreshShop();
}

void UVelmaraShopMenuWidget::HandleComboUnlocked(FGameplayTag UnlockedTag)
{
	RefreshShop();
}

void UVelmaraShopMenuWidget::HandleStatUpgraded(FGameplayTag StatTag, int32 NewLevel)
{
	RefreshShop();
}

void UVelmaraShopMenuWidget::HandleRowFocused(int32 RowIndex)
{
	SelectRow(RowIndex);
}

void UVelmaraShopMenuWidget::HandleRowActivated(int32 RowIndex)
{
	SelectRow(RowIndex);
	PurchaseSelected();
}

void UVelmaraShopMenuWidget::HandleBuyButtonClicked()
{
	PurchaseSelected();
}
