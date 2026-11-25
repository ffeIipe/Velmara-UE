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

static void RebuildProceduralBrackets(UWidgetTree* WidgetTree, UOverlay* ProceduralLayer, bool bUseMinimalBrackets)
{
	if (!WidgetTree || !ProceduralLayer)
	{
		return;
	}

	ProceduralLayer->ClearChildren();

	constexpr float Inset = 6.0f;
	constexpr float Thickness = 1.5f;
	const float ArmLen = bUseMinimalBrackets ? 22.0f : 30.0f;
	constexpr float DiamondSize = 6.0f;
	constexpr float DiamondInset = Inset - (DiamondSize - Thickness) * 0.5f;
	const FSlateBrush BarBrush = FVelmaraUITheme::MakeSolidBrush(FVelmaraUITheme::Bone(0.88f));
	const FSlateBrush DiamondBrush = FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::MarkerDiamondSolid),
		FVector2D(DiamondSize, DiamondSize),
		FVelmaraUITheme::Bone(0.95f));

	UBorder* PerimeterBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	PerimeterBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
	PerimeterBorder->SetPadding(FMargin(0.0f));
	PerimeterBorder->SetBrush(FVelmaraUITheme::MakeOptionRowBrush(FVelmaraUITheme::Ash(0.28f)));
	if (UOverlaySlot* PerimeterSlot = ProceduralLayer->AddChildToOverlay(PerimeterBorder))
	{
		PerimeterSlot->SetHorizontalAlignment(HAlign_Fill);
		PerimeterSlot->SetVerticalAlignment(VAlign_Fill);
		PerimeterSlot->SetPadding(FMargin(Inset));
	}

	auto AddBar = [&](float Width, float Height, EHorizontalAlignment HAlign, EVerticalAlignment VAlign, const FMargin& Pad)
	{
		USizeBox* BarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		BarSize->SetWidthOverride(Width);
		BarSize->SetHeightOverride(Height);
		BarSize->SetVisibility(ESlateVisibility::HitTestInvisible);

		UBorder* BarBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		BarBorder->SetPadding(FMargin(0.0f));
		BarBorder->SetBrush(BarBrush);
		BarBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
		BarSize->SetContent(BarBorder);

		if (UOverlaySlot* BarSlot = ProceduralLayer->AddChildToOverlay(BarSize))
		{
			BarSlot->SetHorizontalAlignment(HAlign);
			BarSlot->SetVerticalAlignment(VAlign);
			BarSlot->SetPadding(Pad);
		}
	};

	auto AddCornerDiamond = [&](EHorizontalAlignment HAlign, EVerticalAlignment VAlign, const FMargin& Pad)
	{
		USizeBox* DiamondBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		DiamondBox->SetWidthOverride(DiamondSize);
		DiamondBox->SetHeightOverride(DiamondSize);
		DiamondBox->SetVisibility(ESlateVisibility::HitTestInvisible);

		UImage* DiamondImg = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		DiamondImg->SetBrush(DiamondBrush);
		DiamondImg->SetVisibility(ESlateVisibility::HitTestInvisible);
		DiamondBox->SetContent(DiamondImg);

		if (UOverlaySlot* DiamondSlot = ProceduralLayer->AddChildToOverlay(DiamondBox))
		{
			DiamondSlot->SetHorizontalAlignment(HAlign);
			DiamondSlot->SetVerticalAlignment(VAlign);
			DiamondSlot->SetPadding(Pad);
		}
	};

	AddBar(ArmLen, Thickness, HAlign_Left, VAlign_Top, FMargin(Inset, Inset, 0.0f, 0.0f));
	AddBar(Thickness, ArmLen, HAlign_Left, VAlign_Top, FMargin(Inset, Inset, 0.0f, 0.0f));
	AddCornerDiamond(HAlign_Left, VAlign_Top, FMargin(DiamondInset, DiamondInset, 0.0f, 0.0f));

	AddBar(ArmLen, Thickness, HAlign_Right, VAlign_Top, FMargin(0.0f, Inset, Inset, 0.0f));
	AddBar(Thickness, ArmLen, HAlign_Right, VAlign_Top, FMargin(0.0f, Inset, Inset, 0.0f));
	AddCornerDiamond(HAlign_Right, VAlign_Top, FMargin(0.0f, DiamondInset, DiamondInset, 0.0f));

	AddBar(ArmLen, Thickness, HAlign_Left, VAlign_Bottom, FMargin(Inset, 0.0f, 0.0f, Inset));
	AddBar(Thickness, ArmLen, HAlign_Left, VAlign_Bottom, FMargin(Inset, 0.0f, 0.0f, Inset));
	AddCornerDiamond(HAlign_Left, VAlign_Bottom, FMargin(DiamondInset, 0.0f, 0.0f, DiamondInset));

	AddBar(ArmLen, Thickness, HAlign_Right, VAlign_Bottom, FMargin(0.0f, 0.0f, Inset, Inset));
	AddBar(Thickness, ArmLen, HAlign_Right, VAlign_Bottom, FMargin(0.0f, 0.0f, Inset, Inset));
	AddCornerDiamond(HAlign_Right, VAlign_Bottom, FMargin(0.0f, 0.0f, DiamondInset, DiamondInset));
}

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
	BracketFrame->SetVisibility(ESlateVisibility::Collapsed);
	if (UOverlaySlot* FrameSlot = RootOverlay->AddChildToOverlay(BracketFrame))
	{
		FrameSlot->SetHorizontalAlignment(HAlign_Fill);
		FrameSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UOverlay* ProceduralLayer = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("BracketProceduralLayer"));
	ProceduralLayer->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UOverlaySlot* ProceduralSlot = RootOverlay->AddChildToOverlay(ProceduralLayer))
	{
		ProceduralSlot->SetHorizontalAlignment(HAlign_Fill);
		ProceduralSlot->SetVerticalAlignment(VAlign_Fill);
	}

	USizeBox* TopStarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BracketTopStarSize"));
	TopStarSize->SetWidthOverride(12.0f);
	TopStarSize->SetHeightOverride(12.0f);
	TopStarImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BracketTopStar"));
	TopStarImage->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnRowStar),
		FVector2D(12.0f, 12.0f),
		FVelmaraUITheme::Bone()));
	TopStarImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	TopStarSize->SetContent(TopStarImage);
	if (UOverlaySlot* StarSlot = RootOverlay->AddChildToOverlay(TopStarSize))
	{
		StarSlot->SetHorizontalAlignment(HAlign_Center);
		StarSlot->SetVerticalAlignment(VAlign_Top);
		StarSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 0.0f));
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
		BracketFrame->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (WidgetTree)
	{
		UOverlay* ProceduralLayer = Cast<UOverlay>(WidgetTree->FindWidget(TEXT("BracketProceduralLayer")));
		RebuildProceduralBrackets(WidgetTree, ProceduralLayer, bUseMinimalBrackets);
	}
	if (SignatureRow)
	{
		SignatureRow->SetVisibility(bShowSignature ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (TopStarImage)
	{
		TopStarImage->SetVisibility(bShowTopStar ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}
