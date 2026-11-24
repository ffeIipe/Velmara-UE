#include "UI/Widgets/Common/VelmaraBracketPanel.h"
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
#include "UI/Style/VelmaraUITheme.h"

UVelmaraBracketPanel::UVelmaraBracketPanel(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedRef<SWidget> UVelmaraBracketPanel::RebuildWidget()
{
	EnsurePanelBuilt();
	return Super::RebuildWidget();
}

void UVelmaraBracketPanel::EnsurePanelBuilt()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildPanelTree();
	}
}

UVerticalBox* UVelmaraBracketPanel::GetContentBox()
{
	EnsurePanelBuilt();
	return ContentBox;
}

void UVelmaraBracketPanel::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshStyle();
}

void UVelmaraBracketPanel::BuildPanelTree()
{
	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("BracketRootOverlay"));
	WidgetTree->RootWidget = RootOverlay;

	BackdropBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BracketBackdrop"));
	BackdropBorder->SetPadding(FMargin(0.0f));
	if (UOverlaySlot* BgSlot = RootOverlay->AddChildToOverlay(BackdropBorder))
	{
		BgSlot->SetHorizontalAlignment(HAlign_Fill);
		BgSlot->SetVerticalAlignment(VAlign_Fill);
	}

	BracketFrame = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BracketFrameImage"));
	BracketFrame->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (UOverlaySlot* FrameSlot = RootOverlay->AddChildToOverlay(BracketFrame))
	{
		FrameSlot->SetHorizontalAlignment(HAlign_Fill);
		FrameSlot->SetVerticalAlignment(VAlign_Fill);
	}

	USizeBox* TopStarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BracketTopStarSize"));
	TopStarSize->SetWidthOverride(22.0f);
	TopStarSize->SetHeightOverride(22.0f);
	TopStarImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BracketTopStar"));
	TopStarImage->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnRowStar),
		FVector2D(22.0f, 22.0f),
		FVelmaraUITheme::Bone()));
	TopStarSize->SetContent(TopStarImage);
	if (UOverlaySlot* StarSlot = RootOverlay->AddChildToOverlay(TopStarSize))
	{
		StarSlot->SetHorizontalAlignment(HAlign_Center);
		StarSlot->SetVerticalAlignment(VAlign_Top);
		StarSlot->SetPadding(FMargin(0.0f, -10.0f, 0.0f, 0.0f));
	}

	ContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BracketContentBox"));
	if (UOverlaySlot* ContentSlot = RootOverlay->AddChildToOverlay(ContentBox))
	{
		ContentSlot->SetHorizontalAlignment(HAlign_Fill);
		ContentSlot->SetVerticalAlignment(VAlign_Fill);
		ContentSlot->SetPadding(ContentPadding);
	}

	SignatureRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BracketSignatureRow"));
	SignatureRow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (UOverlaySlot* SigSlot = RootOverlay->AddChildToOverlay(SignatureRow))
	{
		SigSlot->SetHorizontalAlignment(HAlign_Right);
		SigSlot->SetVerticalAlignment(VAlign_Bottom);
		SigSlot->SetPadding(FMargin(0.0f, 0.0f, 24.0f, 16.0f));
	}

	UCommonTextBlock* SigText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("VelmaraSigText"));
	SigText->SetText(FText::FromString(TEXT("VELMARA")));
	SigText->SetFont(FVelmaraUITheme::GetDisplayFont(12, 240));
	SigText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted(0.65f)));
	if (UHorizontalBoxSlot* SigTextSlot = SignatureRow->AddChildToHorizontalBox(SigText))
	{
		SigTextSlot->SetVerticalAlignment(VAlign_Center);
		SigTextSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
	}

	USizeBox* CrestSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("VelmaraSigCrestSize"));
	CrestSize->SetWidthOverride(10.0f);
	CrestSize->SetHeightOverride(28.0f);
	UImage* CrestImg = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("VelmaraSigCrest"));
	CrestImg->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnCrestVelmara),
		FVector2D(10.0f, 28.0f),
		FVelmaraUITheme::BloodBright(0.80f)));
	CrestSize->SetContent(CrestImg);
	if (UHorizontalBoxSlot* CrestSlot = SignatureRow->AddChildToHorizontalBox(CrestSize))
	{
		CrestSlot->SetVerticalAlignment(VAlign_Center);
	}

	RefreshStyle();
}

void UVelmaraBracketPanel::SetUseMinimalBrackets(bool bMinimal)
{
	bUseMinimalBrackets = bMinimal;
	RefreshStyle();
}

void UVelmaraBracketPanel::SetBackdropAlpha(float InAlpha)
{
	BackdropAlpha = FMath::Clamp(InAlpha, 0.0f, 1.0f);
	RefreshStyle();
}

void UVelmaraBracketPanel::SetContentPadding(const FMargin& InPadding)
{
	ContentPadding = InPadding;
	if (ContentBox)
	{
		if (UOverlaySlot* ContentSlot = Cast<UOverlaySlot>(ContentBox->Slot))
		{
			ContentSlot->SetPadding(ContentPadding);
		}
	}
}

void UVelmaraBracketPanel::SetShowSignature(bool bShow)
{
	bShowSignature = bShow;
	RefreshStyle();
}

void UVelmaraBracketPanel::SetShowTopStar(bool bShow)
{
	bShowTopStar = bShow;
	RefreshStyle();
}

void UVelmaraBracketPanel::ClearContent()
{
	if (ContentBox)
	{
		ContentBox->ClearChildren();
	}
}

void UVelmaraBracketPanel::RefreshStyle()
{
	if (BackdropBorder)
	{
		BackdropBorder->SetBrush(FVelmaraUITheme::MakeSolidBrush(FVelmaraUITheme::Void(BackdropAlpha)));
	}
	if (BracketFrame)
	{
		BracketFrame->SetBrush(bUseMinimalBrackets
			? FVelmaraUITheme::MakeBracketsMinimalBrush(FVelmaraUITheme::Bone())
			: FVelmaraUITheme::MakeBracketsPanelBrush(FVelmaraUITheme::Bone()));
	}
	if (SignatureRow)
	{
		SignatureRow->SetVisibility(bShowSignature ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (TopStarImage)
	{
		TopStarImage->SetVisibility(bShowTopStar ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}
