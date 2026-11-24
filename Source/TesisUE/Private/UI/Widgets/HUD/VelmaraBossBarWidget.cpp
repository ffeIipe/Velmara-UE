#include "UI/Widgets/HUD/VelmaraBossBarWidget.h"
#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/Style/VelmaraUITheme.h"

UVelmaraBossBarWidget::UVelmaraBossBarWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	BossName = NSLOCTEXT("VelmaraUI", "DefaultBossName", "EL PALADÍN CONSAGRADO");
}

TSharedRef<SWidget> UVelmaraBossBarWidget::RebuildWidget()
{
	if (WidgetTree && (!bBuiltCppTree || !WidgetTree->RootWidget))
	{
		bBuiltCppTree = true;
		BuildBossBarTree();
	}
	return Super::RebuildWidget();
}

void UVelmaraBossBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UVelmaraBossBarWidget::BuildBossBarTree()
{
	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("BossBarRoot"));
	RootOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = RootOverlay;

	UVerticalBox* BottomAnchorVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BossBarContainer"));
	if (UOverlaySlot* ContainerSlot = RootOverlay->AddChildToOverlay(BottomAnchorVBox))
	{
		ContainerSlot->SetHorizontalAlignment(HAlign_Center);
		ContainerSlot->SetVerticalAlignment(VAlign_Bottom);
		ContainerSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 52.0f));
	}

	USizeBox* CrestSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BossCrestSize"));
	CrestSize->SetWidthOverride(340.0f);
	CrestSize->SetHeightOverride(48.0f);
	UImage* CrestImg = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BossCrestImg"));
	CrestImg->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnCrestBoss),
		FVector2D(340.0f, 48.0f),
		FVelmaraUITheme::BoneMuted(0.90f)));
	CrestSize->SetContent(CrestImg);
	if (UVerticalBoxSlot* CrestSlot = BottomAnchorVBox->AddChildToVerticalBox(CrestSize))
	{
		CrestSlot->SetHorizontalAlignment(HAlign_Center);
		CrestSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 2.0f));
	}

	BossDisplayNameText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("BossDisplayNameText"));
	BossDisplayNameText->SetText(BossName.ToUpper());
	BossDisplayNameText->SetFont(FVelmaraUITheme::GetDisplayFont(26, 260));
	BossDisplayNameText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
	BossDisplayNameText->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* NameSlot = BottomAnchorVBox->AddChildToVerticalBox(BossDisplayNameText))
	{
		NameSlot->SetHorizontalAlignment(HAlign_Center);
		NameSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	ShieldRowSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BossShieldRowSize"));
	ShieldRowSize->SetWidthOverride(1050.0f);
	ShieldRowSize->SetHeightOverride(8.0f);
	ShieldRowSize->SetVisibility(ESlateVisibility::Collapsed);

	UOverlay* ShieldOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("BossShieldOverlay"));
	ShieldRowSize->SetContent(ShieldOverlay);

	ShieldProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("BossShieldProgress"));
	FProgressBarStyle ShieldStyle;
	ShieldStyle.SetBackgroundImage(FVelmaraUITheme::MakeSolidBrush(FVelmaraUITheme::Charcoal(0.75f)));
	ShieldStyle.SetFillImage(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::BarShieldFillMask),
		FVector2D(1050.0f, 8.0f),
		FVelmaraUITheme::Shield()));
	ShieldProgressBar->SetWidgetStyle(ShieldStyle);
	ShieldProgressBar->SetFillColorAndOpacity(FVelmaraUITheme::Shield());
	ShieldProgressBar->SetPercent(0.0f);
	if (UOverlaySlot* ShBarSlot = ShieldOverlay->AddChildToOverlay(ShieldProgressBar))
	{
		ShBarSlot->SetHorizontalAlignment(HAlign_Fill);
		ShBarSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UImage* ShieldFrame = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BossShieldFrame"));
	ShieldFrame->SetBrush(FVelmaraUITheme::MakeShieldBarFrameBrush(FVelmaraUITheme::BoneMuted(0.85f)));
	if (UOverlaySlot* ShFrameSlot = ShieldOverlay->AddChildToOverlay(ShieldFrame))
	{
		ShFrameSlot->SetHorizontalAlignment(HAlign_Fill);
		ShFrameSlot->SetVerticalAlignment(VAlign_Fill);
	}

	if (UVerticalBoxSlot* ShRowSlot = BottomAnchorVBox->AddChildToVerticalBox(ShieldRowSize))
	{
		ShRowSlot->SetHorizontalAlignment(HAlign_Center);
		ShRowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
	}

	UHorizontalBox* BarAssembly = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BossBarAssembly"));
	if (UVerticalBoxSlot* BarSlot = BottomAnchorVBox->AddChildToVerticalBox(BarAssembly))
	{
		BarSlot->SetHorizontalAlignment(HAlign_Center);
	}

	USizeBox* LeftFinialSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BossFinialLSize"));
	LeftFinialSize->SetWidthOverride(72.0f);
	LeftFinialSize->SetHeightOverride(36.0f);
	UImage* LeftFinial = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BossFinialL"));
	LeftFinial->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::BarFinialBossL),
		FVector2D(72.0f, 36.0f),
		FVelmaraUITheme::Bone(0.90f)));
	LeftFinialSize->SetContent(LeftFinial);
	if (UHorizontalBoxSlot* LFSlot = BarAssembly->AddChildToHorizontalBox(LeftFinialSize))
	{
		LFSlot->SetVerticalAlignment(VAlign_Center);
		LFSlot->SetPadding(FMargin(0.0f, 0.0f, -14.0f, 0.0f));
	}

	USizeBox* BodySize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BossBarBodySize"));
	BodySize->SetWidthOverride(1050.0f);
	BodySize->SetHeightOverride(20.0f);
	UOverlay* BodyOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("BossBarBodyOverlay"));
	BodySize->SetContent(BodyOverlay);

	UBorder* TrackBg = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BossBarTrackBg"));
	TrackBg->SetBrush(FVelmaraUITheme::MakeSolidBrush(FVelmaraUITheme::Charcoal(0.88f)));
	if (UOverlaySlot* BgSlot = BodyOverlay->AddChildToOverlay(TrackBg))
	{
		BgSlot->SetHorizontalAlignment(HAlign_Fill);
		BgSlot->SetVerticalAlignment(VAlign_Fill);
		BgSlot->SetPadding(FMargin(2.0f));
	}

	UTexture2D* FillTex = FVelmaraUITheme::LoadTexture(VelmaraUIPaths::BarBossFillMask);
	FSlateBrush EmptyBg;
	EmptyBg.DrawAs = ESlateBrushDrawType::NoDrawType;

	DelayedHealthBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("BossDelayedBar"));
	FProgressBarStyle DelayedStyle;
	DelayedStyle.SetBackgroundImage(EmptyBg);
	DelayedStyle.SetFillImage(FVelmaraUITheme::MakeImageBrush(FillTex, FVector2D(1050.0f, 20.0f), FVelmaraUITheme::BloodDried()));
	DelayedHealthBar->SetWidgetStyle(DelayedStyle);
	DelayedHealthBar->SetFillColorAndOpacity(FVelmaraUITheme::BloodDried());
	DelayedHealthBar->SetPercent(1.0f);
	if (UOverlaySlot* DSlot = BodyOverlay->AddChildToOverlay(DelayedHealthBar))
	{
		DSlot->SetHorizontalAlignment(HAlign_Fill);
		DSlot->SetVerticalAlignment(VAlign_Fill);
		DSlot->SetPadding(FMargin(2.0f));
	}

	HealthProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("BossHealthBar"));
	FProgressBarStyle HealthStyle;
	HealthStyle.SetBackgroundImage(EmptyBg);
	HealthStyle.SetFillImage(FVelmaraUITheme::MakeImageBrush(FillTex, FVector2D(1050.0f, 20.0f), FVelmaraUITheme::Blood()));
	HealthProgressBar->SetWidgetStyle(HealthStyle);
	HealthProgressBar->SetFillColorAndOpacity(FVelmaraUITheme::Blood());
	HealthProgressBar->SetPercent(1.0f);
	if (UOverlaySlot* HSlot = BodyOverlay->AddChildToOverlay(HealthProgressBar))
	{
		HSlot->SetHorizontalAlignment(HAlign_Fill);
		HSlot->SetVerticalAlignment(VAlign_Fill);
		HSlot->SetPadding(FMargin(2.0f));
	}

	UImage* BossFrame = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BossBarFrame"));
	BossFrame->SetBrush(FVelmaraUITheme::MakeBossBarFrameBrush(FVelmaraUITheme::Bone(0.90f)));
	if (UOverlaySlot* FSlot = BodyOverlay->AddChildToOverlay(BossFrame))
	{
		FSlot->SetHorizontalAlignment(HAlign_Fill);
		FSlot->SetVerticalAlignment(VAlign_Fill);
	}

	auto AddPhaseMarker = [&](float LeftPaddingPx)
	{
		USizeBox* MarkerSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		MarkerSize->SetWidthOverride(16.0f);
		MarkerSize->SetHeightOverride(16.0f);
		UImage* MarkerImg = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		MarkerImg->SetBrush(FVelmaraUITheme::MakeImageBrush(
			FVelmaraUITheme::LoadTexture(VelmaraUIPaths::BarPhaseMarker),
			FVector2D(16.0f, 16.0f),
			FVelmaraUITheme::Bone()));
		MarkerSize->SetContent(MarkerImg);
		if (UOverlaySlot* MSlot = BodyOverlay->AddChildToOverlay(MarkerSize))
		{
			MSlot->SetHorizontalAlignment(HAlign_Left);
			MSlot->SetVerticalAlignment(VAlign_Center);
			MSlot->SetPadding(FMargin(LeftPaddingPx, 0.0f, 0.0f, 0.0f));
		}
	};

	AddPhaseMarker(1050.0f * 0.33f - 8.0f);
	AddPhaseMarker(1050.0f * 0.66f - 8.0f);

	if (UHorizontalBoxSlot* BodyHSlot = BarAssembly->AddChildToHorizontalBox(BodySize))
	{
		BodyHSlot->SetVerticalAlignment(VAlign_Center);
	}

	USizeBox* RightFinialSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BossFinialRSize"));
	RightFinialSize->SetWidthOverride(72.0f);
	RightFinialSize->SetHeightOverride(36.0f);
	UImage* RightFinial = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BossFinialR"));
	RightFinial->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::BarFinialBossR),
		FVector2D(72.0f, 36.0f),
		FVelmaraUITheme::Bone(0.90f)));
	RightFinialSize->SetContent(RightFinial);
	if (UHorizontalBoxSlot* RFSlot = BarAssembly->AddChildToHorizontalBox(RightFinialSize))
	{
		RFSlot->SetVerticalAlignment(VAlign_Center);
		RFSlot->SetPadding(FMargin(-14.0f, 0.0f, 0.0f, 0.0f));
	}
}

void UVelmaraBossBarWidget::SetBossDisplayName(const FText& InBossName)
{
	BossName = InBossName;
	if (BossDisplayNameText)
	{
		BossDisplayNameText->SetText(BossName.ToUpper());
	}
}

void UVelmaraBossBarWidget::ReceivePercentagesFromBoss_Implementation(float HealthHP, float ShieldHP)
{
	const float ClampedHP = FMath::Clamp(HealthHP, 0.0f, 1.0f);
	const float ClampedShield = FMath::Clamp(ShieldHP, 0.0f, 1.0f);

	if (ClampedHP < TargetHealthPercent)
	{
		DelayedHoldTimer = 0.35f;
	}
	else if (ClampedHP > DisplayedDelayedHealth)
	{
		DisplayedDelayedHealth = ClampedHP;
	}

	TargetHealthPercent = ClampedHP;
	TargetShieldPercent = ClampedShield;

	if (HealthProgressBar)
	{
		HealthProgressBar->SetPercent(TargetHealthPercent);
	}
	if (ShieldRowSize && ShieldProgressBar)
	{
		ShieldProgressBar->SetPercent(TargetShieldPercent);
		ShieldRowSize->SetVisibility(TargetShieldPercent > 0.001f ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UVelmaraBossBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (DisplayedDelayedHealth > TargetHealthPercent)
	{
		if (DelayedHoldTimer > 0.0f)
		{
			DelayedHoldTimer -= InDeltaTime;
		}
		else
		{
			DisplayedDelayedHealth = FMath::FInterpTo(DisplayedDelayedHealth, TargetHealthPercent, InDeltaTime, 3.2f);
			if (FMath::IsNearlyEqual(DisplayedDelayedHealth, TargetHealthPercent, 0.002f))
			{
				DisplayedDelayedHealth = TargetHealthPercent;
			}
		}
		if (DelayedHealthBar)
		{
			DelayedHealthBar->SetPercent(DisplayedDelayedHealth);
		}
	}
}
