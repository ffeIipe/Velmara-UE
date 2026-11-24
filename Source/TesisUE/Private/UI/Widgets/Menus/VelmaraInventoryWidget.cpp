#include "UI/Widgets/Menus/VelmaraInventoryWidget.h"
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
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "DataAssets/Items/Weapons/PistolData.h"
#include "DataAssets/Items/Weapons/SwordData.h"
#include "DataAssets/Items/Weapons/WeaponData.h"
#include "Features/InventorySystem/Core/InventoryComponent.h"
#include "Features/InventorySystem/Interfaces/Pickable.h"
#include "Features/InventorySystem/Items/Item.h"
#include "Features/InventorySystem/Items/Weapon.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Items/Weapons/DualSword.h"
#include "Items/Weapons/Pistol.h"
#include "Items/Weapons/Sword.h"
#include "Player/CustomPlayerController.h"
#include "UI/Style/VelmaraUITheme.h"
#include "UI/Widgets/Common/VelmaraBracketPanel.h"
#include "UI/Widgets/Common/VelmaraButtonBase.h"
#include "UI/Widgets/Common/VelmaraCommonStyles.h"
#include "UI/Widgets/Common/VelmaraFooterBar.h"

UVelmaraItemSlotButton::UVelmaraItemSlotButton(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetStyle(UVelmaraTransparentButtonStyle::StaticClass());
	SetIsFocusable(true);
}

void UVelmaraItemSlotButton::ConfigureSlot(int32 InSlotIndex, float InSlotDimension)
{
	SlotIndex = InSlotIndex;
	SlotDimension = InSlotDimension;
}

UCommonButtonInternalBase* UVelmaraItemSlotButton::ConstructInternalButton()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildSlotTree();
	}
	return Super::ConstructInternalButton();
}

TSharedRef<SWidget> UVelmaraItemSlotButton::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildSlotTree();
	}
	return Super::RebuildWidget();
}

void UVelmaraItemSlotButton::BuildSlotTree()
{
	USizeBox* RootSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SlotRootSize"));
	RootSize->SetWidthOverride(SlotDimension);
	RootSize->SetHeightOverride(SlotDimension);
	WidgetTree->RootWidget = RootSize;

	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("SlotOverlay"));
	RootSize->SetContent(Overlay);

	FillImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SlotFillImg"));
	FillImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	FillImage->SetBrush(FVelmaraUITheme::MakeSquareSlotFillBrush(FVelmaraUITheme::Charcoal(0.75f)));
	if (UOverlaySlot* FillSlot = Overlay->AddChildToOverlay(FillImage))
	{
		FillSlot->SetHorizontalAlignment(HAlign_Fill);
		FillSlot->SetVerticalAlignment(VAlign_Fill);
	}

	FrameImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SlotFrameImg"));
	FrameImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	FrameImage->SetBrush(FVelmaraUITheme::MakeSquareSlotNormalBrush(FVelmaraUITheme::Ash()));
	if (UOverlaySlot* FrameSlot = Overlay->AddChildToOverlay(FrameImage))
	{
		FrameSlot->SetHorizontalAlignment(HAlign_Fill);
		FrameSlot->SetVerticalAlignment(VAlign_Fill);
	}

	SelectedOverlayImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SlotSelImg"));
	SelectedOverlayImage->SetVisibility(ESlateVisibility::Hidden);
	SelectedOverlayImage->SetBrush(FVelmaraUITheme::MakeSquareSlotSelectedBrush(FVelmaraUITheme::BloodBright()));
	if (UOverlaySlot* SelSlot = Overlay->AddChildToOverlay(SelectedOverlayImage))
	{
		SelSlot->SetHorizontalAlignment(HAlign_Fill);
		SelSlot->SetVerticalAlignment(VAlign_Fill);
	}

	ItemIconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SlotIconImg"));
	ItemIconImage->SetVisibility(ESlateVisibility::Hidden);
	if (UOverlaySlot* IconSlot = Overlay->AddChildToOverlay(ItemIconImage))
	{
		IconSlot->SetHorizontalAlignment(HAlign_Center);
		IconSlot->SetVerticalAlignment(VAlign_Center);
		IconSlot->SetPadding(FMargin(14.0f));
	}

	USizeBox* MarkerSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SlotMarkerSize"));
	MarkerSize->SetWidthOverride(12.0f);
	MarkerSize->SetHeightOverride(12.0f);
	EquippedMarkerImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SlotMarkerImg"));
	EquippedMarkerImage->SetVisibility(ESlateVisibility::Hidden);
	EquippedMarkerImage->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::MarkerDiamondSolid),
		FVector2D(12.0f, 12.0f),
		FVelmaraUITheme::BloodBright()));
	MarkerSize->SetContent(EquippedMarkerImage);
	if (UOverlaySlot* MarkerSlot = Overlay->AddChildToOverlay(MarkerSize))
	{
		MarkerSlot->SetHorizontalAlignment(HAlign_Right);
		MarkerSlot->SetVerticalAlignment(VAlign_Top);
		MarkerSlot->SetPadding(FMargin(0.0f, 7.0f, 7.0f, 0.0f));
	}

	RefreshVisuals();
}

void UVelmaraItemSlotButton::SetSlotData(UTexture2D* InIcon, bool bInOccupied, bool bInEquipped)
{
	bOccupied = bInOccupied;
	bEquipped = bInEquipped;

	if (ItemIconImage)
	{
		if (bOccupied && InIcon)
		{
			const float IconDim = FMath::Max(32.0f, SlotDimension - 28.0f);
			ItemIconImage->SetBrush(FVelmaraUITheme::MakeImageBrush(InIcon, FVector2D(IconDim, IconDim), FVelmaraUITheme::Bone()));
			ItemIconImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			ItemIconImage->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (EquippedMarkerImage)
	{
		EquippedMarkerImage->SetVisibility(bEquipped ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	}

	RefreshVisuals();
}

void UVelmaraItemSlotButton::SetSlotHighlighted(bool bInSelected)
{
	bHighlighted = bInSelected;
	RefreshVisuals();
}

void UVelmaraItemSlotButton::RefreshVisuals()
{
	const bool bActive = bIsVisualHovered || bIsVisualFocused || bHighlighted;

	if (FillImage)
	{
		FillImage->SetBrush(FVelmaraUITheme::MakeSquareSlotFillBrush(
			bActive ? FVelmaraUITheme::BloodDried(0.52f) : FVelmaraUITheme::Charcoal(bOccupied ? 0.78f : 0.50f)));
	}

	if (FrameImage)
	{
		FLinearColor Tint = FVelmaraUITheme::Ash(bOccupied ? 0.95f : 0.55f);
		if (bEquipped)
		{
			Tint = FVelmaraUITheme::Bone(0.85f);
		}
		FrameImage->SetBrush(FVelmaraUITheme::MakeSquareSlotNormalBrush(Tint));
	}

	if (SelectedOverlayImage)
	{
		SelectedOverlayImage->SetVisibility(bActive ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	}
}

void UVelmaraItemSlotButton::NativeOnHovered()
{
	Super::NativeOnHovered();
	bIsVisualHovered = true;
	SetKeyboardFocus();
	RefreshVisuals();
	OnSlotFocusedOrHovered.Broadcast(SlotIndex);
}

void UVelmaraItemSlotButton::NativeOnUnhovered()
{
	Super::NativeOnUnhovered();
	bIsVisualHovered = false;
	RefreshVisuals();
}

void UVelmaraItemSlotButton::NativeOnClicked()
{
	Super::NativeOnClicked();
	OnSlotActivated.Broadcast(SlotIndex);
}

void UVelmaraItemSlotButton::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnAddedToFocusPath(InFocusEvent);
	bIsVisualFocused = true;
	RefreshVisuals();
	OnSlotFocusedOrHovered.Broadcast(SlotIndex);
}

void UVelmaraItemSlotButton::NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnRemovedFromFocusPath(InFocusEvent);
	bIsVisualFocused = false;
	RefreshVisuals();
}

UVelmaraInventoryWidget::UVelmaraInventoryWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = true;
	bSupportsActivationFocus = true;
	bAutoActivate = false;
}

TOptional<FUIInputConfig> UVelmaraInventoryWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture, false);
}

TSharedRef<SWidget> UVelmaraInventoryWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildScreenTree();
	}
	return Super::RebuildWidget();
}

void UVelmaraInventoryWidget::BuildScreenTree()
{
	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("InvRootOverlay"));
	WidgetTree->RootWidget = RootOverlay;

	UBackgroundBlur* Blur = WidgetTree->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass(), TEXT("InvBlur"));
	Blur->SetBlurStrength(12.0f);
	if (UOverlaySlot* BlurSlot = RootOverlay->AddChildToOverlay(Blur))
	{
		BlurSlot->SetHorizontalAlignment(HAlign_Fill);
		BlurSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UBorder* DimBg = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InvDimBg"));
	DimBg->SetBrush(FVelmaraUITheme::MakeSolidBrush(FVelmaraUITheme::Void(0.80f)));
	if (UOverlaySlot* DimSlot = RootOverlay->AddChildToOverlay(DimBg))
	{
		DimSlot->SetHorizontalAlignment(HAlign_Fill);
		DimSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UVerticalBox* MainStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("InvMainStack"));
	if (UOverlaySlot* MainSlot = RootOverlay->AddChildToOverlay(MainStack))
	{
		MainSlot->SetHorizontalAlignment(HAlign_Fill);
		MainSlot->SetVerticalAlignment(VAlign_Fill);
		MainSlot->SetPadding(FMargin(64.0f, 38.0f, 64.0f, 34.0f));
	}

	UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("InvHeaderRow"));
	if (UVerticalBoxSlot* HeadSlot = MainStack->AddChildToVerticalBox(HeaderRow))
	{
		HeadSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 26.0f));
	}

	UVerticalBox* TitleCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("InvTitleCol"));
	UCommonTextBlock* TitleText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("InvTitleText"));
	TitleText->SetText(NSLOCTEXT("VelmaraUI", "InventoryHeader", "I N V E N T A R I O"));
	TitleText->SetFont(FVelmaraUITheme::GetDisplayFont(36, 240));
	TitleText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
	TitleCol->AddChildToVerticalBox(TitleText);

	USizeBox* TitleSepSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("InvTitleSepSize"));
	TitleSepSize->SetWidthOverride(380.0f);
	TitleSepSize->SetHeightOverride(20.0f);
	UImage* TitleSep = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("InvTitleSep"));
	TitleSep->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnSepStar),
		FVector2D(380.0f, 20.0f),
		FVelmaraUITheme::Bone(0.85f)));
	TitleSepSize->SetContent(TitleSep);
	if (UVerticalBoxSlot* SepSlot = TitleCol->AddChildToVerticalBox(TitleSepSize))
	{
		SepSlot->SetHorizontalAlignment(HAlign_Left);
		SepSlot->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 0.0f));
	}

	if (UHorizontalBoxSlot* TitleSlot = HeaderRow->AddChildToHorizontalBox(TitleCol))
	{
		TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TitleSlot->SetVerticalAlignment(VAlign_Center);
	}

	UHorizontalBox* CurrencyBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("InvCurrencyBox"));
	USizeBox* BloodIconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("InvBloodIconSize"));
	BloodIconSize->SetWidthOverride(22.0f);
	BloodIconSize->SetHeightOverride(22.0f);
	UImage* BloodIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("InvBloodIcon"));
	BloodIcon->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::IconStatBlood),
		FVector2D(22.0f, 22.0f),
		FVelmaraUITheme::Bone()));
	BloodIconSize->SetContent(BloodIcon);
	if (UHorizontalBoxSlot* BISlot = CurrencyBox->AddChildToHorizontalBox(BloodIconSize))
	{
		BISlot->SetVerticalAlignment(VAlign_Center);
		BISlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
	}

	CurrencyValueText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("InvCurrencyText"));
	CurrencyValueText->SetText(FText::FromString(TEXT("0")));
	CurrencyValueText->SetFont(FVelmaraUITheme::GetDisplayFont(22, 160));
	CurrencyValueText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
	if (UHorizontalBoxSlot* ValSlot = CurrencyBox->AddChildToHorizontalBox(CurrencyValueText))
	{
		ValSlot->SetVerticalAlignment(VAlign_Center);
		ValSlot->SetPadding(FMargin(0.0f, 0.0f, 18.0f, 0.0f));
	}

	USizeBox* CrestSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("InvCrestSize"));
	CrestSize->SetWidthOverride(32.0f);
	CrestSize->SetHeightOverride(76.0f);
	UImage* CrestImg = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("InvCrestImg"));
	CrestImg->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnCrestVelmara),
		FVector2D(32.0f, 76.0f),
		FVelmaraUITheme::BloodBright(0.92f)));
	CrestSize->SetContent(CrestImg);
	CurrencyBox->AddChildToHorizontalBox(CrestSize)->SetVerticalAlignment(VAlign_Center);

	HeaderRow->AddChildToHorizontalBox(CurrencyBox)->SetVerticalAlignment(VAlign_Center);

	UHorizontalBox* Columns = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("InvColumns"));
	if (UVerticalBoxSlot* ColsSlot = MainStack->AddChildToVerticalBox(Columns))
	{
		ColsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ColsSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f));
	}

	UVerticalBox* EquippedCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("InvEquippedCol"));
	USizeBox* EquippedColSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("InvEquippedColSize"));
	EquippedColSize->SetWidthOverride(220.0f);
	EquippedColSize->SetContent(EquippedCol);
	if (UHorizontalBoxSlot* EqSlot = Columns->AddChildToHorizontalBox(EquippedColSize))
	{
		EqSlot->SetVerticalAlignment(VAlign_Top);
		EqSlot->SetPadding(FMargin(0.0f, 0.0f, 44.0f, 0.0f));
	}

	UCommonTextBlock* EquippedHeader = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("InvEquippedHeader"));
	EquippedHeader->SetText(NSLOCTEXT("VelmaraUI", "InvEquippedLabel", "EQUIPADO"));
	EquippedHeader->SetFont(FVelmaraUITheme::GetDisplayFont(15, 220));
	EquippedHeader->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted()));
	if (UVerticalBoxSlot* EHSlot = EquippedCol->AddChildToVerticalBox(EquippedHeader))
	{
		EHSlot->SetHorizontalAlignment(HAlign_Center);
		EHSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f));
	}

	PrimaryEquippedSlot = WidgetTree->ConstructWidget<UVelmaraItemSlotButton>(UVelmaraItemSlotButton::StaticClass(), TEXT("InvPrimaryEqSlot"));
	PrimaryEquippedSlot->ConfigureSlot(INDEX_NONE, 118.0f);
	PrimaryEquippedSlot->SetIsFocusable(false);
	if (UVerticalBoxSlot* PSlot = EquippedCol->AddChildToVerticalBox(PrimaryEquippedSlot))
	{
		PSlot->SetHorizontalAlignment(HAlign_Center);
		PSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	PrimaryEquippedNameText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("InvPrimaryEqName"));
	PrimaryEquippedNameText->SetFont(FVelmaraUITheme::GetDisplayFont(14, 140));
	PrimaryEquippedNameText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
	PrimaryEquippedNameText->SetJustification(ETextJustify::Center);
	PrimaryEquippedNameText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* PNSlot = EquippedCol->AddChildToVerticalBox(PrimaryEquippedNameText))
	{
		PNSlot->SetHorizontalAlignment(HAlign_Center);
		PNSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 28.0f));
	}

	SecondaryEquippedSlot = WidgetTree->ConstructWidget<UVelmaraItemSlotButton>(UVelmaraItemSlotButton::StaticClass(), TEXT("InvSecondaryEqSlot"));
	SecondaryEquippedSlot->ConfigureSlot(INDEX_NONE, 104.0f);
	SecondaryEquippedSlot->SetIsFocusable(false);
	if (UVerticalBoxSlot* SSlot = EquippedCol->AddChildToVerticalBox(SecondaryEquippedSlot))
	{
		SSlot->SetHorizontalAlignment(HAlign_Center);
		SSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	SecondaryEquippedNameText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("InvSecondaryEqName"));
	SecondaryEquippedNameText->SetFont(FVelmaraUITheme::GetDisplayFont(13, 140));
	SecondaryEquippedNameText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted()));
	SecondaryEquippedNameText->SetJustification(ETextJustify::Center);
	SecondaryEquippedNameText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* SNSlot = EquippedCol->AddChildToVerticalBox(SecondaryEquippedNameText))
	{
		SNSlot->SetHorizontalAlignment(HAlign_Center);
	}

	UVerticalBox* GridCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("InvGridCol"));
	if (UHorizontalBoxSlot* GSlot = Columns->AddChildToHorizontalBox(GridCol))
	{
		GSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		GSlot->SetHorizontalAlignment(HAlign_Center);
		GSlot->SetVerticalAlignment(VAlign_Top);
		GSlot->SetPadding(FMargin(0.0f, 0.0f, 44.0f, 0.0f));
	}

	UCommonTextBlock* GridHeader = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("InvGridHeader"));
	GridHeader->SetText(NSLOCTEXT("VelmaraUI", "InvArsenalLabel", "ARSENAL Y RELIQUIAS"));
	GridHeader->SetFont(FVelmaraUITheme::GetDisplayFont(15, 220));
	GridHeader->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted()));
	if (UVerticalBoxSlot* GHSlot = GridCol->AddChildToVerticalBox(GridHeader))
	{
		GHSlot->SetHorizontalAlignment(HAlign_Left);
		GHSlot->SetPadding(FMargin(4.0f, 0.0f, 0.0f, 16.0f));
	}

	ItemGridPanel = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), TEXT("InvItemGrid"));
	ItemGridPanel->SetSlotPadding(FMargin(6.0f));
	GridCol->AddChildToVerticalBox(ItemGridPanel);

	GridSlots.Reset();
	for (int32 Index = 0; Index < TotalGridSlots; ++Index)
	{
		UVelmaraItemSlotButton* SlotBtn = WidgetTree->ConstructWidget<UVelmaraItemSlotButton>(UVelmaraItemSlotButton::StaticClass());
		SlotBtn->ConfigureSlot(Index, 92.0f);
		SlotBtn->OnSlotFocusedOrHovered.AddUObject(this, &UVelmaraInventoryWidget::HandleGridSlotFocused);
		SlotBtn->OnSlotActivated.AddUObject(this, &UVelmaraInventoryWidget::HandleGridSlotActivated);
		ItemGridPanel->AddChildToUniformGrid(SlotBtn, Index / GridColumns, Index % GridColumns);
		GridSlots.Add(SlotBtn);
	}

	USizeBox* RightPanelSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("InvDetailsSize"));
	RightPanelSize->SetWidthOverride(460.0f);
	DetailsPanel = WidgetTree->ConstructWidget<UVelmaraBracketPanel>(UVelmaraBracketPanel::StaticClass(), TEXT("InvDetailsPanel"));
	DetailsPanel->SetUseMinimalBrackets(true);
	DetailsPanel->SetBackdropAlpha(0.78f);
	DetailsPanel->SetShowSignature(true);
	DetailsPanel->SetShowTopStar(true);
	RightPanelSize->SetContent(DetailsPanel);
	if (UHorizontalBoxSlot* RSlot = Columns->AddChildToHorizontalBox(RightPanelSize))
	{
		RSlot->SetHorizontalAlignment(HAlign_Right);
		RSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UVerticalBox* DetailsContent = DetailsPanel->GetContentBox();

	DetailCategoryText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("InvDetailCat"));
	DetailCategoryText->SetFont(FVelmaraUITheme::GetDisplayFont(12, 200));
	DetailCategoryText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BloodBright()));
	if (UVerticalBoxSlot* CatSlot = DetailsContent->AddChildToVerticalBox(DetailCategoryText))
	{
		CatSlot->SetHorizontalAlignment(HAlign_Center);
		CatSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	}

	DetailTitleText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("InvDetailTitle"));
	DetailTitleText->SetFont(FVelmaraUITheme::GetDisplayFont(24, 180));
	DetailTitleText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
	DetailTitleText->SetJustification(ETextJustify::Center);
	DetailTitleText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* TSlot = DetailsContent->AddChildToVerticalBox(DetailTitleText))
	{
		TSlot->SetHorizontalAlignment(HAlign_Center);
		TSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	USizeBox* DetSepSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("InvDetSepSize"));
	DetSepSize->SetHeightOverride(16.0f);
	UImage* DetSep = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("InvDetSep"));
	DetSep->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnSepDiamond),
		FVector2D(340.0f, 16.0f),
		FVelmaraUITheme::BoneMuted(0.8f)));
	DetSepSize->SetContent(DetSep);
	if (UVerticalBoxSlot* SepSlot = DetailsContent->AddChildToVerticalBox(DetSepSize))
	{
		SepSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
	}

	DetailDescriptionText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("InvDetailDesc"));
	DetailDescriptionText->SetFont(FVelmaraUITheme::GetBodyFont(17, 0));
	DetailDescriptionText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted()));
	DetailDescriptionText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* DSlot = DetailsContent->AddChildToVerticalBox(DetailDescriptionText))
	{
		DSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
	}

	DetailStatsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("InvDetailStatsBox"));
	if (UVerticalBoxSlot* StSlot = DetailsContent->AddChildToVerticalBox(DetailStatsBox))
	{
		StSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
	}

	USpacer* DetSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	if (UVerticalBoxSlot* SpSlot = DetailsContent->AddChildToVerticalBox(DetSpacer))
	{
		SpSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UHorizontalBox* ActionsRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("InvActionsRow"));
	if (UVerticalBoxSlot* ActSlot = DetailsContent->AddChildToVerticalBox(ActionsRow))
	{
		ActSlot->SetHorizontalAlignment(HAlign_Center);
		ActSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 24.0f));
	}

	EquipActionButton = WidgetTree->ConstructWidget<UVelmaraButtonBase>(UVelmaraButtonBase::StaticClass(), TEXT("InvEquipBtn"));
	EquipActionButton->SetVariant(EVelmaraButtonVariant::HexAction);
	EquipActionButton->SetFontSize(15);
	EquipActionButton->SetButtonMinSize(170.0f, 44.0f);
	EquipActionButton->SetButtonText(NSLOCTEXT("VelmaraUI", "InvBtnEquip", "EQUIPAR"));
	EquipActionButton->OnClicked().AddUObject(this, &UVelmaraInventoryWidget::HandleEquipButtonClicked);
	if (UHorizontalBoxSlot* EqBtnSlot = ActionsRow->AddChildToHorizontalBox(EquipActionButton))
	{
		EqBtnSlot->SetPadding(FMargin(0.0f, 0.0f, 14.0f, 0.0f));
	}

	DropActionButton = WidgetTree->ConstructWidget<UVelmaraButtonBase>(UVelmaraButtonBase::StaticClass(), TEXT("InvDropBtn"));
	DropActionButton->SetVariant(EVelmaraButtonVariant::HexAction);
	DropActionButton->SetFontSize(15);
	DropActionButton->SetButtonMinSize(170.0f, 44.0f);
	DropActionButton->SetButtonText(NSLOCTEXT("VelmaraUI", "InvBtnDrop", "SOLTAR"));
	DropActionButton->OnClicked().AddUObject(this, &UVelmaraInventoryWidget::HandleDropButtonClicked);
	ActionsRow->AddChildToHorizontalBox(DropActionButton);

	FooterBar = WidgetTree->ConstructWidget<UVelmaraFooterBar>(UVelmaraFooterBar::StaticClass(), TEXT("InvFooterBar"));
	FooterBar->SetActions({
		FVelmaraFooterAction(TEXT("Confirm"), NSLOCTEXT("VelmaraUI", "InvFooterEquip", "EQUIPAR")),
		FVelmaraFooterAction(TEXT("Secondary"), NSLOCTEXT("VelmaraUI", "InvFooterDrop", "SOLTAR")),
		FVelmaraFooterAction(TEXT("Back"), NSLOCTEXT("VelmaraUI", "InvFooterClose", "CERRAR"))
	});
	MainStack->AddChildToVerticalBox(FooterBar);
}

void UVelmaraInventoryWidget::BindInventoryComponent(UInventoryComponent* InInventoryComponent)
{
	if (UProgressionComponent* OldProg = ProgressionComp.Get())
	{
		OldProg->OnCurrencyChanged.RemoveDynamic(this, &UVelmaraInventoryWidget::HandleCurrencyChanged);
	}

	InventoryComp = InInventoryComponent;
	ProgressionComp = nullptr;

	if (InInventoryComponent && InInventoryComponent->GetOwner())
	{
		ProgressionComp = InInventoryComponent->GetOwner()->FindComponentByClass<UProgressionComponent>();
	}

	if (!ProgressionComp.IsValid())
	{
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
	}

	if (UProgressionComponent* Prog = ProgressionComp.Get())
	{
		Prog->OnCurrencyChanged.AddUniqueDynamic(this, &UVelmaraInventoryWidget::HandleCurrencyChanged);
	}

	RefreshCurrencyHeader();

	if (InventoryComp.IsValid())
	{
		RefreshInventoryUI(InventoryComp->GetInventoryItems());
	}
}

void UVelmaraInventoryWidget::RefreshInventoryUI_Implementation(const TArray<TScriptInterface<IPickable>>& Items)
{
	CachedItems = Items;
	RebuildGridDisplay();
	RefreshEquippedColumn();
	RefreshDetailsPanel();
	RefreshCurrencyHeader();
}

void UVelmaraInventoryWidget::RebuildGridDisplay()
{
	const int32 EquippedIdx = InventoryComp.IsValid() ? InventoryComp->EquippedSlotIndex : INDEX_NONE;

	for (int32 Index = 0; Index < GridSlots.Num(); ++Index)
	{
		UVelmaraItemSlotButton* SlotBtn = GridSlots[Index];
		if (!SlotBtn) continue;

		UObject* ItemObj = CachedItems.IsValidIndex(Index) ? CachedItems[Index].GetObject() : nullptr;
		const bool bOccupied = ItemObj != nullptr;
		const bool bEquipped = bOccupied && (Index == EquippedIdx);
		UTexture2D* IconTex = bOccupied ? ResolveItemIcon(ItemObj) : nullptr;

		SlotBtn->SetSlotData(IconTex, bOccupied, bEquipped);
		SlotBtn->SetSlotHighlighted(Index == SelectedSlotIndex);
	}
}

void UVelmaraInventoryWidget::RefreshEquippedColumn()
{
	UObject* PrimaryObj = InventoryComp.IsValid() ? InventoryComp->GetCurrentWeapon().GetObject() : nullptr;
	if (PrimaryEquippedSlot)
	{
		PrimaryEquippedSlot->SetSlotData(PrimaryObj ? ResolveItemIcon(PrimaryObj) : nullptr, PrimaryObj != nullptr, PrimaryObj != nullptr);
	}
	if (PrimaryEquippedNameText)
	{
		PrimaryEquippedNameText->SetText(PrimaryObj
			? ResolveItemName(PrimaryObj).ToUpper()
			: NSLOCTEXT("VelmaraUI", "InvEmptyPrimary", "SIN ARMA EQUIPADA"));
	}

	UObject* SecondaryObj = nullptr;
	const int32 EquippedIdx = InventoryComp.IsValid() ? InventoryComp->EquippedSlotIndex : INDEX_NONE;
	for (int32 Index = 0; Index < CachedItems.Num(); ++Index)
	{
		if (Index != EquippedIdx && CachedItems[Index].GetObject() != nullptr)
		{
			SecondaryObj = CachedItems[Index].GetObject();
			break;
		}
	}

	if (SecondaryEquippedSlot)
	{
		SecondaryEquippedSlot->SetSlotData(SecondaryObj ? ResolveItemIcon(SecondaryObj) : nullptr, SecondaryObj != nullptr, false);
	}
	if (SecondaryEquippedNameText)
	{
		SecondaryEquippedNameText->SetText(SecondaryObj
			? ResolveItemName(SecondaryObj).ToUpper()
			: NSLOCTEXT("VelmaraUI", "InvEmptySecondary", "SECUNDARIA VACÍA"));
	}
}

void UVelmaraInventoryWidget::RefreshDetailsPanel()
{
	UObject* SelectedObj = CachedItems.IsValidIndex(SelectedSlotIndex) ? CachedItems[SelectedSlotIndex].GetObject() : nullptr;
	const int32 EquippedIdx = InventoryComp.IsValid() ? InventoryComp->EquippedSlotIndex : INDEX_NONE;
	const bool bIsEquipped = SelectedObj != nullptr && SelectedSlotIndex == EquippedIdx;

	if (!SelectedObj)
	{
		if (DetailCategoryText) DetailCategoryText->SetText(NSLOCTEXT("VelmaraUI", "InvSlotEmptyCat", "RANURA VACÍA"));
		if (DetailTitleText) DetailTitleText->SetText(NSLOCTEXT("VelmaraUI", "InvSlotEmptyTitle", "SIN OBJETO"));
		if (DetailDescriptionText) DetailDescriptionText->SetText(NSLOCTEXT("VelmaraUI", "InvSlotEmptyDesc", "Explora la catedral y recoge armas para añadirlas a tu arsenal."));
		if (DetailStatsBox) DetailStatsBox->ClearChildren();
		if (EquipActionButton) EquipActionButton->SetIsEnabled(false);
		if (DropActionButton) DropActionButton->SetIsEnabled(false);
		return;
	}

	if (DetailCategoryText) DetailCategoryText->SetText(ResolveItemCategory(SelectedObj, bIsEquipped));
	if (DetailTitleText) DetailTitleText->SetText(ResolveItemName(SelectedObj).ToUpper());
	if (DetailDescriptionText) DetailDescriptionText->SetText(ResolveItemDescription(SelectedObj));

	PopulateWeaponStats(SelectedObj);

	if (EquipActionButton)
	{
		EquipActionButton->SetIsEnabled(!bIsEquipped);
		EquipActionButton->SetButtonText(bIsEquipped
			? NSLOCTEXT("VelmaraUI", "InvBtnEquipped", "EQUIPADO")
			: NSLOCTEXT("VelmaraUI", "InvBtnEquip", "EQUIPAR"));
	}
	if (DropActionButton)
	{
		DropActionButton->SetIsEnabled(true);
	}
}

void UVelmaraInventoryWidget::PopulateWeaponStats(UObject* ItemObject)
{
	if (!DetailStatsBox) return;
	DetailStatsBox->ClearChildren();

	auto AddStatRow = [&](const FText& Label, const FText& Value)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UCommonTextBlock* LblTxt = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass());
		LblTxt->SetText(Label);
		LblTxt->SetFont(FVelmaraUITheme::GetDisplayFont(14, 140));
		LblTxt->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted()));
		if (UHorizontalBoxSlot* LSlot = Row->AddChildToHorizontalBox(LblTxt))
		{
			LSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			LSlot->SetVerticalAlignment(VAlign_Center);
		}

		UCommonTextBlock* ValTxt = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass());
		ValTxt->SetText(Value);
		ValTxt->SetFont(FVelmaraUITheme::GetBoldBodyFont(16, 0));
		ValTxt->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
		Row->AddChildToHorizontalBox(ValTxt)->SetVerticalAlignment(VAlign_Center);

		if (UVerticalBoxSlot* RSlot = DetailStatsBox->AddChildToVerticalBox(Row))
		{
			RSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		}
	};

	const UWeaponData* WeaponData = nullptr;
	const UPistolData* PistolData = nullptr;
	if (const ASword* Sword = Cast<ASword>(ItemObject))
	{
		WeaponData = Sword->GetBaseSwordData();
	}
	else if (const APistol* Pistol = Cast<APistol>(ItemObject))
	{
		PistolData = Pistol->GetPistolData();
		WeaponData = PistolData;
	}

	if (WeaponData)
	{
		const float RawDamage = PistolData && PistolData->PistolTypeStats.BaseDamage != 0.0f
			? FMath::Abs(PistolData->PistolTypeStats.BaseDamage)
			: FMath::Abs(WeaponData->BaseDamage);
		AddStatRow(NSLOCTEXT("VelmaraUI", "StatBaseDmg", "DAÑO BASE"), FText::AsNumber(FMath::RoundToInt(RawDamage)));
		AddStatRow(NSLOCTEXT("VelmaraUI", "StatCritChance", "PROB. CRÍTICO"), FText::Format(NSLOCTEXT("VelmaraUI", "PctFmt", "{0}%"), FMath::RoundToInt(WeaponData->CriticalChance * 100.0f)));
		AddStatRow(NSLOCTEXT("VelmaraUI", "StatCritMult", "DAÑO CRÍTICO"), FText::FromString(FString::Printf(TEXT("%.1fx"), WeaponData->CriticalDamageMultiplier)));

		if (PistolData && PistolData->PistolTypeStats.MaxAmmo > 0)
		{
			AddStatRow(NSLOCTEXT("VelmaraUI", "StatAmmo", "MUNICIÓN MÁXIMA"), FText::AsNumber(PistolData->PistolTypeStats.MaxAmmo));
		}
	}
}

void UVelmaraInventoryWidget::RefreshCurrencyHeader()
{
	if (!CurrencyValueText) return;

	const int32 Amount = ProgressionComp.IsValid() ? ProgressionComp->GetAvailableCurrency() : 0;
	CurrencyValueText->SetText(FText::Format(
		NSLOCTEXT("VelmaraUI", "InvCurrencyFmt", "{0} SANGRE"),
		FVelmaraUITheme::FormatGroupedNumber(Amount)));
}

UTexture2D* UVelmaraInventoryWidget::ResolveItemIcon(UObject* ItemObject) const
{
	if (const AItem* Item = Cast<AItem>(ItemObject))
	{
		if (Item->GetUIItemProperties().ItemIcon)
		{
			return Item->GetUIItemProperties().ItemIcon;
		}
	}

	if (const ADualSword* Dual = Cast<ADualSword>(ItemObject))
	{
		if (Dual->GetBaseSwordData() && Dual->GetBaseSwordData()->Icon)
		{
			return Dual->GetBaseSwordData()->Icon;
		}
		return FVelmaraUITheme::LoadTexture(VelmaraUIPaths::IconWeaponDualBlades);
	}

	if (const ASword* Sword = Cast<ASword>(ItemObject))
	{
		if (Sword->GetBaseSwordData() && Sword->GetBaseSwordData()->Icon)
		{
			return Sword->GetBaseSwordData()->Icon;
		}
		const FString Name = Sword->GetName().ToLower();
		if (Name.Contains(TEXT("spear")) || Name.Contains(TEXT("lanza")))
		{
			return FVelmaraUITheme::LoadTexture(VelmaraUIPaths::IconWeaponSpear);
		}
		if (Name.Contains(TEXT("scythe")) || Name.Contains(TEXT("guada")))
		{
			return FVelmaraUITheme::LoadTexture(VelmaraUIPaths::IconWeaponScythe);
		}
		return FVelmaraUITheme::LoadTexture(VelmaraUIPaths::IconWeaponLongsword);
	}

	if (const APistol* Pistol = Cast<APistol>(ItemObject))
	{
		if (Pistol->GetPistolData() && Pistol->GetPistolData()->Icon)
		{
			return Pistol->GetPistolData()->Icon;
		}
		return FVelmaraUITheme::LoadTexture(VelmaraUIPaths::IconWeaponFlintlock);
	}

	return FVelmaraUITheme::LoadTexture(VelmaraUIPaths::IconItemVial);
}

FText UVelmaraInventoryWidget::ResolveItemName(UObject* ItemObject) const
{
	if (const AItem* Item = Cast<AItem>(ItemObject))
	{
		if (!Item->GetUIItemProperties().ItemDisplayName.IsEmpty())
		{
			return Item->GetUIItemProperties().ItemDisplayName;
		}
	}

	if (const ASword* Sword = Cast<ASword>(ItemObject))
	{
		if (Sword->GetBaseSwordData() && !Sword->GetBaseSwordData()->DisplayName.IsEmpty())
		{
			return Sword->GetBaseSwordData()->DisplayName;
		}
	}
	else if (const APistol* Pistol = Cast<APistol>(ItemObject))
	{
		if (Pistol->GetPistolData() && !Pistol->GetPistolData()->DisplayName.IsEmpty())
		{
			return Pistol->GetPistolData()->DisplayName;
		}
	}

	return ItemObject ? FText::FromString(ItemObject->GetName()) : FText::GetEmpty();
}

FText UVelmaraInventoryWidget::ResolveItemDescription(UObject* ItemObject) const
{
	if (const AItem* Item = Cast<AItem>(ItemObject))
	{
		if (!Item->GetUIItemProperties().ItemDescription.IsEmpty())
		{
			return Item->GetUIItemProperties().ItemDescription;
		}
	}
	return NSLOCTEXT("VelmaraUI", "InvDefaultDesc", "Reliquia forjada para la cacería nocturna en Velmara.");
}

FText UVelmaraInventoryWidget::ResolveItemCategory(UObject* ItemObject, bool bIsEquipped) const
{
	if (Cast<AWeapon>(ItemObject))
	{
		return bIsEquipped
			? NSLOCTEXT("VelmaraUI", "InvCatWeaponEq", "ARMA  ·  EQUIPADA")
			: NSLOCTEXT("VelmaraUI", "InvCatWeapon", "ARMA  ·  EQUIPABLE");
	}
	return NSLOCTEXT("VelmaraUI", "InvCatItem", "OBJETO");
}

void UVelmaraInventoryWidget::SelectSlot(int32 SlotIndex)
{
	SelectedSlotIndex = FMath::Clamp(SlotIndex, 0, TotalGridSlots - 1);
	for (int32 Index = 0; Index < GridSlots.Num(); ++Index)
	{
		if (GridSlots[Index])
		{
			GridSlots[Index]->SetSlotHighlighted(Index == SelectedSlotIndex);
		}
	}
	RefreshDetailsPanel();
}

void UVelmaraInventoryWidget::EquipSelectedSlot()
{
	if (UInventoryComponent* Inv = InventoryComp.Get())
	{
		if (CachedItems.IsValidIndex(SelectedSlotIndex) && CachedItems[SelectedSlotIndex].GetObject() != nullptr)
		{
			Inv->EquipItemFromSlot(SelectedSlotIndex);
			RefreshInventoryUI(Inv->GetInventoryItems());
		}
	}
}

void UVelmaraInventoryWidget::DropSelectedSlot()
{
	if (UInventoryComponent* Inv = InventoryComp.Get())
	{
		if (CachedItems.IsValidIndex(SelectedSlotIndex) && CachedItems[SelectedSlotIndex].GetObject() != nullptr)
		{
			Inv->DropWeaponFromSlot(SelectedSlotIndex);
			RefreshInventoryUI(Inv->GetInventoryItems());
		}
	}
}

void UVelmaraInventoryWidget::HandleCurrencyChanged(int32 NewAmount)
{
	RefreshCurrencyHeader();
}

void UVelmaraInventoryWidget::HandleGridSlotFocused(int32 SlotIndex)
{
	SelectSlot(SlotIndex);
}

void UVelmaraInventoryWidget::HandleGridSlotActivated(int32 SlotIndex)
{
	SelectSlot(SlotIndex);
	EquipSelectedSlot();
}

void UVelmaraInventoryWidget::HandleEquipButtonClicked()
{
	EquipSelectedSlot();
}

void UVelmaraInventoryWidget::HandleDropButtonClicked()
{
	DropSelectedSlot();
}

void UVelmaraInventoryWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	if (!InventoryComp.IsValid())
	{
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
				BindInventoryComponent(TargetPawn->FindComponentByClass<UInventoryComponent>());
			}
		}
	}

	if (GridSlots.IsValidIndex(SelectedSlotIndex) && GridSlots[SelectedSlotIndex])
	{
		GridSlots[SelectedSlotIndex]->SetKeyboardFocus();
	}
}

void UVelmaraInventoryWidget::NativeOnDeactivated()
{
	Super::NativeOnDeactivated();

	if (UProgressionComponent* Prog = ProgressionComp.Get())
	{
		Prog->OnCurrencyChanged.RemoveDynamic(this, &UVelmaraInventoryWidget::HandleCurrencyChanged);
	}

	if (UInventoryComponent* Inv = InventoryComp.Get())
	{
		Inv->NotifyInventoryClosed();
	}
}

UWidget* UVelmaraInventoryWidget::NativeGetDesiredFocusTarget() const
{
	if (GridSlots.IsValidIndex(SelectedSlotIndex) && GridSlots[SelectedSlotIndex])
	{
		return GridSlots[SelectedSlotIndex];
	}
	return GridSlots.Num() > 0 ? GridSlots[0].Get() : nullptr;
}

bool UVelmaraInventoryWidget::NativeOnHandleBackAction()
{
	DeactivateWidget();
	return true;
}

FReply UVelmaraInventoryWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::I || Key == EKeys::Tab)
	{
		DeactivateWidget();
		return FReply::Handled();
	}
	if (Key == EKeys::X || Key == EKeys::Delete || Key == EKeys::Gamepad_FaceButton_Left)
	{
		DropSelectedSlot();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
