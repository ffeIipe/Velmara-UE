#include "UI/Widgets/Menus/VelmaraSaveSlotCardWidget.h"
#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/Style/VelmaraUITheme.h"
#include "UI/Widgets/Common/VelmaraCommonStyles.h"

UVelmaraSaveSlotCardWidget::UVelmaraSaveSlotCardWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Style = UVelmaraTransparentButtonStyle::StaticClass();
	SetIsFocusable(true);
}

UCommonButtonInternalBase* UVelmaraSaveSlotCardWidget::ConstructInternalButton()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildCardTree();
	}
	return Super::ConstructInternalButton();
}

TSharedRef<SWidget> UVelmaraSaveSlotCardWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildCardTree();
	}
	return Super::RebuildWidget();
}

void UVelmaraSaveSlotCardWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshContent();
	UpdateVisualState();
}

void UVelmaraSaveSlotCardWidget::BuildCardTree()
{
	USizeBox* RootSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SaveSlotCardSize"));
	RootSize->SetMinDesiredHeight(132.0f);
	WidgetTree->RootWidget = RootSize;

	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("SaveSlotCardOverlay"));
	RootSize->SetContent(RootOverlay);

	SolidBackdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SaveSlotBackdrop"));
	SolidBackdrop->SetPadding(FMargin(0.0f));
	if (UOverlaySlot* BgSlot = RootOverlay->AddChildToOverlay(SolidBackdrop))
	{
		BgSlot->SetHorizontalAlignment(HAlign_Fill);
		BgSlot->SetVerticalAlignment(VAlign_Fill);
	}

	FocusGradient = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SaveSlotGradient"));
	FocusGradient->SetVisibility(ESlateVisibility::Hidden);
	if (UOverlaySlot* GradSlot = RootOverlay->AddChildToOverlay(FocusGradient))
	{
		GradSlot->SetHorizontalAlignment(HAlign_Fill);
		GradSlot->SetVerticalAlignment(VAlign_Fill);
	}

	FrameBorder = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SaveSlotFrame"));
	FrameBorder->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (UOverlaySlot* FrameSlot = RootOverlay->AddChildToOverlay(FrameBorder))
	{
		FrameSlot->SetHorizontalAlignment(HAlign_Fill);
		FrameSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UHorizontalBox* MainRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SaveSlotMainRow"));
	if (UOverlaySlot* RowSlot = RootOverlay->AddChildToOverlay(MainRow))
	{
		RowSlot->SetHorizontalAlignment(HAlign_Fill);
		RowSlot->SetVerticalAlignment(VAlign_Fill);
		RowSlot->SetPadding(FMargin(28.0f, 18.0f, 28.0f, 18.0f));
	}

	USizeBox* EmblemSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SlotEmblemSize"));
	EmblemSize->SetWidthOverride(76.0f);
	EmblemSize->SetHeightOverride(76.0f);
	UOverlay* EmblemOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("SlotEmblemOverlay"));
	EmblemSize->SetContent(EmblemOverlay);

	EmblemFrame = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SlotEmblemFrame"));
	if (UOverlaySlot* EFSlot = EmblemOverlay->AddChildToOverlay(EmblemFrame))
	{
		EFSlot->SetHorizontalAlignment(HAlign_Fill);
		EFSlot->SetVerticalAlignment(VAlign_Fill);
	}

	USizeBox* GlyphSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SlotGlyphSize"));
	GlyphSize->SetWidthOverride(28.0f);
	GlyphSize->SetHeightOverride(42.0f);
	EmblemGlyph = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SlotEmblemGlyph"));
	GlyphSize->SetContent(EmblemGlyph);
	if (UOverlaySlot* EGSlot = EmblemOverlay->AddChildToOverlay(GlyphSize))
	{
		EGSlot->SetHorizontalAlignment(HAlign_Center);
		EGSlot->SetVerticalAlignment(VAlign_Center);
	}

	if (UHorizontalBoxSlot* EmblemHSlot = MainRow->AddChildToHorizontalBox(EmblemSize))
	{
		EmblemHSlot->SetVerticalAlignment(VAlign_Center);
		EmblemHSlot->SetPadding(FMargin(0.0f, 0.0f, 26.0f, 0.0f));
	}

	UOverlay* CenterOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("SlotCenterOverlay"));
	if (UHorizontalBoxSlot* CenterHSlot = MainRow->AddChildToHorizontalBox(CenterOverlay))
	{
		CenterHSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		CenterHSlot->SetVerticalAlignment(VAlign_Center);
	}

	OccupiedInfoBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("OccupiedInfoBox"));
	if (UOverlaySlot* OccSlot = CenterOverlay->AddChildToOverlay(OccupiedInfoBox))
	{
		OccSlot->SetHorizontalAlignment(HAlign_Fill);
		OccSlot->SetVerticalAlignment(VAlign_Center);
	}

	TitleText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("SlotTitleText"));
	TitleText->SetFont(FVelmaraUITheme::GetDisplayFont(22, 190));
	if (UVerticalBoxSlot* TitleSlot = OccupiedInfoBox->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}

	SubtitleText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("SlotSubtitleText"));
	SubtitleText->SetFont(FVelmaraUITheme::GetBodyFont(17, 0));
	OccupiedInfoBox->AddChildToVerticalBox(SubtitleText);

	EmptySlotText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("EmptySlotText"));
	EmptySlotText->SetText(NSLOCTEXT("VelmaraUI", "EmptySaveSlot", "— RANURA VACÍA —"));
	EmptySlotText->SetFont(FVelmaraUITheme::GetDisplayFont(20, 220));
	EmptySlotText->SetJustification(ETextJustify::Center);
	if (UOverlaySlot* EmptySlot = CenterOverlay->AddChildToOverlay(EmptySlotText))
	{
		EmptySlot->SetHorizontalAlignment(HAlign_Center);
		EmptySlot->SetVerticalAlignment(VAlign_Center);
	}

	RightMetaBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SlotRightMetaBox"));
	if (UHorizontalBoxSlot* RightSlot = MainRow->AddChildToHorizontalBox(RightMetaBox))
	{
		RightSlot->SetVerticalAlignment(VAlign_Center);
		RightSlot->SetPadding(FMargin(18.0f, 0.0f, 0.0f, 0.0f));
	}

	USizeBox* BloodIconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SlotBloodIconSize"));
	BloodIconSize->SetWidthOverride(18.0f);
	BloodIconSize->SetHeightOverride(18.0f);
	UImage* BloodIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SlotBloodIcon"));
	BloodIcon->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::IconStatBlood),
		FVector2D(18.0f, 18.0f),
		FVelmaraUITheme::BloodBright()));
	BloodIconSize->SetContent(BloodIcon);
	if (UHorizontalBoxSlot* BIconSlot = RightMetaBox->AddChildToHorizontalBox(BloodIconSize))
	{
		BIconSlot->SetVerticalAlignment(VAlign_Center);
		BIconSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
	}

	ProgressionCountText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("SlotProgressionText"));
	ProgressionCountText->SetFont(FVelmaraUITheme::GetDisplayFont(19, 140));
	if (UHorizontalBoxSlot* ProgSlot = RightMetaBox->AddChildToHorizontalBox(ProgressionCountText))
	{
		ProgSlot->SetVerticalAlignment(VAlign_Center);
		ProgSlot->SetPadding(FMargin(0.0f, 0.0f, 18.0f, 0.0f));
	}

	USizeBox* ChevSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SlotChevSize"));
	ChevSize->SetWidthOverride(12.0f);
	ChevSize->SetHeightOverride(18.0f);
	RightChevron = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SlotRightChev"));
	ChevSize->SetContent(RightChevron);
	if (UHorizontalBoxSlot* ChevSlot = RightMetaBox->AddChildToHorizontalBox(ChevSize))
	{
		ChevSlot->SetVerticalAlignment(VAlign_Center);
	}

	USizeBox* StarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SlotStarSize"));
	StarSize->SetWidthOverride(16.0f);
	StarSize->SetHeightOverride(16.0f);
	BottomStar = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SlotBottomStar"));
	BottomStar->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	StarSize->SetContent(BottomStar);
	if (UOverlaySlot* StarSlot = RootOverlay->AddChildToOverlay(StarSize))
	{
		StarSlot->SetHorizontalAlignment(HAlign_Center);
		StarSlot->SetVerticalAlignment(VAlign_Bottom);
		StarSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, -7.0f));
	}

	RefreshContent();
	UpdateVisualState();
}

void UVelmaraSaveSlotCardWidget::ConfigureCard(const FVelmaraSaveSlotViewData& InData, bool bInLoadMode)
{
	SlotData = InData;
	bLoadMode = bInLoadMode;
	RefreshContent();
	UpdateVisualState();
}

void UVelmaraSaveSlotCardWidget::RefreshContent()
{
	if (!OccupiedInfoBox || !EmptySlotText || !RightMetaBox)
	{
		return;
	}

	if (SlotData.bOccupied)
	{
		OccupiedInfoBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		EmptySlotText->SetVisibility(ESlateVisibility::Collapsed);
		RightMetaBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		const FString Roman = FVelmaraUITheme::FormatSlotRomanNumeral(SlotData.SlotIndex);
		const FText LevelTitle = FVelmaraUITheme::FormatLevelDisplayName(SlotData.RawLevelName);
		TitleText->SetText(FText::Format(
			NSLOCTEXT("VelmaraUI", "SlotTitleFmt", "RANURA {0}  —  {1}"),
			FText::FromString(Roman),
			LevelTitle));

		const FString DateText = SlotData.Timestamp.GetTicks() > 0
			? SlotData.Timestamp.ToString(TEXT("%d/%m/%Y %H:%M"))
			: TEXT("--/--/----");
		SubtitleText->SetText(FText::Format(
			NSLOCTEXT("VelmaraUI", "SlotSubtitleFmt", "Último guardado: {0}  ·  Atributos: {1}  ·  Combos: {2}"),
			FText::FromString(DateText),
			FText::AsNumber(SlotData.TotalAttributeLevels),
			FText::AsNumber(SlotData.UnlockedCombosCount)));

		const int32 TotalRank = SlotData.TotalAttributeLevels + SlotData.UnlockedCombosCount;
		ProgressionCountText->SetText(FText::Format(
			NSLOCTEXT("VelmaraUI", "SlotRankFmt", "NV. {0}"),
			FText::AsNumber(FMath::Max(1, TotalRank + 1))));
	}
	else
	{
		OccupiedInfoBox->SetVisibility(ESlateVisibility::Collapsed);
		EmptySlotText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		RightMetaBox->SetVisibility(ESlateVisibility::Collapsed);
	}
}

bool UVelmaraSaveSlotCardWidget::IsVisuallyFocused() const
{
	return IsHovered() || HasAnyUserFocus() || HasKeyboardFocus() || bIsInFocusPath;
}

void UVelmaraSaveSlotCardWidget::UpdateVisualState()
{
	if (!SolidBackdrop || !FrameBorder || !EmblemFrame || !EmblemGlyph)
	{
		return;
	}

	const bool bDimmedEmpty = bLoadMode && !SlotData.bOccupied;
	const bool bFocused = IsVisuallyFocused();

	SolidBackdrop->SetBrush(FVelmaraUITheme::MakeSolidBrush(
		bFocused && !bDimmedEmpty ? FVelmaraUITheme::BloodDried(0.50f) : FVelmaraUITheme::Void(bDimmedEmpty ? 0.55f : 0.78f)));

	FrameBorder->SetBrush(FVelmaraUITheme::MakeOptionRowBrush(
		bFocused && !bDimmedEmpty ? FVelmaraUITheme::BloodBright() : FVelmaraUITheme::Ash(bDimmedEmpty ? 0.55f : 1.0f)));

	if (FocusGradient)
	{
		if (bFocused && !bDimmedEmpty)
		{
			UTexture2D* GradTex = FVelmaraUITheme::LoadTexture(VelmaraUIPaths::GradHFadeLeftToRight);
			FocusGradient->SetBrush(FVelmaraUITheme::MakeImageBrush(GradTex, FVector2D(512.0f, 8.0f), FVelmaraUITheme::Blood(0.55f)));
			FocusGradient->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			FocusGradient->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	UTexture2D* DiamondTex = FVelmaraUITheme::LoadTexture(
		bFocused && !bDimmedEmpty ? VelmaraUIPaths::SlotDiamondSelected : VelmaraUIPaths::SlotDiamondNormal);
	EmblemFrame->SetBrush(FVelmaraUITheme::MakeImageBrush(
		DiamondTex,
		FVector2D(76.0f, 76.0f),
		bFocused && !bDimmedEmpty ? FVelmaraUITheme::BloodBright() : FVelmaraUITheme::Ash(bDimmedEmpty ? 0.5f : 0.9f)));

	UTexture2D* GlyphTex = FVelmaraUITheme::LoadTexture(
		SlotData.bOccupied ? VelmaraUIPaths::OrnCrestVelmara : VelmaraUIPaths::MarkerDiamondHollow);
	const FVector2D GlyphDim = SlotData.bOccupied ? FVector2D(20.0f, 44.0f) : FVector2D(20.0f, 20.0f);
	EmblemGlyph->SetBrush(FVelmaraUITheme::MakeImageBrush(
		GlyphTex,
		GlyphDim,
		SlotData.bOccupied ? (bFocused ? FVelmaraUITheme::Bone() : FVelmaraUITheme::BloodBright(0.85f)) : FVelmaraUITheme::Ash(0.5f)));

	if (TitleText)
	{
		TitleText->SetColorAndOpacity(FSlateColor(bFocused ? FVelmaraUITheme::Bone() : FVelmaraUITheme::BoneMuted()));
	}
	if (SubtitleText)
	{
		SubtitleText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted(bFocused ? 0.95f : 0.72f)));
	}
	if (EmptySlotText)
	{
		const FLinearColor EmptyColor = bDimmedEmpty ? FVelmaraUITheme::Ash(0.55f) : (bFocused ? FVelmaraUITheme::Bone() : FVelmaraUITheme::BoneMuted(0.65f));
		EmptySlotText->SetColorAndOpacity(FSlateColor(EmptyColor));
	}
	if (ProgressionCountText)
	{
		ProgressionCountText->SetColorAndOpacity(FSlateColor(bFocused ? FVelmaraUITheme::Bone() : FVelmaraUITheme::BoneMuted()));
	}
	if (RightChevron)
	{
		RightChevron->SetBrush(FVelmaraUITheme::MakeImageBrush(
			FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnChevronR),
			FVector2D(12.0f, 18.0f),
			bFocused ? FVelmaraUITheme::BloodBright() : FVelmaraUITheme::Ash()));
	}
	if (BottomStar)
	{
		BottomStar->SetBrush(FVelmaraUITheme::MakeImageBrush(
			FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnRowStar),
			FVector2D(16.0f, 16.0f),
			bFocused && !bDimmedEmpty ? FVelmaraUITheme::Bone() : FVelmaraUITheme::BoneMuted(0.65f)));
	}
}

void UVelmaraSaveSlotCardWidget::NativeOnClicked()
{
	Super::NativeOnClicked();
	if (bLoadMode && !SlotData.bOccupied)
	{
		return;
	}
	OnSlotActivated.Broadcast(SlotData.SlotIndex);
}

void UVelmaraSaveSlotCardWidget::NativeOnHovered()
{
	Super::NativeOnHovered();
	SetKeyboardFocus();
	OnSlotHighlighted.Broadcast(SlotData.SlotIndex);
	UpdateVisualState();
}

void UVelmaraSaveSlotCardWidget::NativeOnUnhovered()
{
	Super::NativeOnUnhovered();
	UpdateVisualState();
}

void UVelmaraSaveSlotCardWidget::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnAddedToFocusPath(InFocusEvent);
	bIsInFocusPath = true;
	OnSlotHighlighted.Broadcast(SlotData.SlotIndex);
	UpdateVisualState();
}

void UVelmaraSaveSlotCardWidget::NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnRemovedFromFocusPath(InFocusEvent);
	bIsInFocusPath = false;
	UpdateVisualState();
}

FReply UVelmaraSaveSlotCardWidget::NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent)
{
	const FReply Reply = Super::NativeOnFocusReceived(InGeometry, InFocusEvent);
	bIsInFocusPath = true;
	OnSlotHighlighted.Broadcast(SlotData.SlotIndex);
	UpdateVisualState();
	return Reply;
}

void UVelmaraSaveSlotCardWidget::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnFocusLost(InFocusEvent);
	bIsInFocusPath = false;
	UpdateVisualState();
}

FReply UVelmaraSaveSlotCardWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (SlotData.bOccupied && (Key == EKeys::Delete || Key == EKeys::X || Key == EKeys::Gamepad_FaceButton_Left))
	{
		OnSlotDeleteRequested.Broadcast(SlotData.SlotIndex);
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
