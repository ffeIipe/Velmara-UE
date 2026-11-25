#include "UI/Widgets/HUD/VelmaraGameplayHUDWidget.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "CommonInputSubsystem.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressionComponent.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "DataAssets/Items/Weapons/WeaponData.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Features/BossSystem/Subsystems/SunlightExposureSubsystem.h"
#include "Features/InventorySystem/Core/InventoryComponent.h"
#include "Features/InventorySystem/Interfaces/Pickable.h"
#include "Features/InventorySystem/Items/Weapon.h"
#include "GAS/VelmaraAttributeSet.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Player/CustomPlayerController.h"
#include "Subsystems/BloodMarkSubsystem.h"
#include "Subsystems/SpectralObjectsSubsystem.h"
#include "UI/Style/VelmaraUITheme.h"

UVelmaraGameplayHUDWidget::UVelmaraGameplayHUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedRef<SWidget> UVelmaraGameplayHUDWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildHUDTree();
	}
	return Super::RebuildWidget();
}

void UVelmaraGameplayHUDWidget::BuildHUDTree()
{
	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("GameplayHUDRoot"));
	RootOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = RootOverlay;

	UVerticalBox* VitalsVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HUDVitalsVBox"));
	if (UOverlaySlot* VitalsSlot = RootOverlay->AddChildToOverlay(VitalsVBox))
	{
		VitalsSlot->SetHorizontalAlignment(HAlign_Left);
		VitalsSlot->SetVerticalAlignment(VAlign_Top);
		VitalsSlot->SetPadding(FMargin(54.0f, 44.0f, 0.0f, 0.0f));
	}

	ShieldBarRowSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("HUDShieldRowSize"));
	ShieldBarRowSize->SetWidthOverride(320.0f);
	ShieldBarRowSize->SetHeightOverride(10.0f);
	UOverlay* ShieldOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("HUDShieldOverlay"));
	ShieldBarRowSize->SetContent(ShieldOverlay);

	FSlateBrush EmptyBg;
	EmptyBg.DrawAs = ESlateBrushDrawType::NoDrawType;

	ShieldProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HUDShieldBar"));
	FProgressBarStyle ShieldStyle;
	ShieldStyle.SetBackgroundImage(FVelmaraUITheme::MakeSolidBrush(FVelmaraUITheme::Charcoal(0.75f)));
	ShieldStyle.SetFillImage(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::BarShieldFillMask),
		FVector2D(320.0f, 10.0f),
		FVelmaraUITheme::Shield()));
	ShieldProgressBar->SetWidgetStyle(ShieldStyle);
	ShieldProgressBar->SetFillColorAndOpacity(FVelmaraUITheme::Shield());
	ShieldProgressBar->SetPercent(0.0f);
	if (UOverlaySlot* ShSlot = ShieldOverlay->AddChildToOverlay(ShieldProgressBar))
	{
		ShSlot->SetHorizontalAlignment(HAlign_Fill);
		ShSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UImage* ShieldFrame = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("HUDShieldFrame"));
	ShieldFrame->SetBrush(FVelmaraUITheme::MakeShieldBarFrameBrush(FVelmaraUITheme::BoneMuted(0.85f)));
	if (UOverlaySlot* ShFSlot = ShieldOverlay->AddChildToOverlay(ShieldFrame))
	{
		ShFSlot->SetHorizontalAlignment(HAlign_Fill);
		ShFSlot->SetVerticalAlignment(VAlign_Fill);
	}

	if (UVerticalBoxSlot* ShRowSlot = VitalsVBox->AddChildToVerticalBox(ShieldBarRowSize))
	{
		ShRowSlot->SetHorizontalAlignment(HAlign_Left);
		ShRowSlot->SetPadding(FMargin(28.0f, 0.0f, 0.0f, 5.0f));
	}

	UHorizontalBox* HealthRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HUDHealthRow"));
	if (UVerticalBoxSlot* HRowSlot = VitalsVBox->AddChildToVerticalBox(HealthRow))
	{
		HRowSlot->SetHorizontalAlignment(HAlign_Left);
		HRowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 7.0f));
	}

	USizeBox* LeftFinSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("HUDFinialLSize"));
	LeftFinSize->SetWidthOverride(42.0f);
	LeftFinSize->SetHeightOverride(28.0f);
	UImage* LeftFin = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("HUDFinialL"));
	LeftFin->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::BarFinialSimpleL),
		FVector2D(42.0f, 28.0f),
		FVelmaraUITheme::Bone(0.90f)));
	LeftFinSize->SetContent(LeftFin);
	if (UHorizontalBoxSlot* LFSlot = HealthRow->AddChildToHorizontalBox(LeftFinSize))
	{
		LFSlot->SetVerticalAlignment(VAlign_Center);
		LFSlot->SetPadding(FMargin(0.0f, 0.0f, -14.0f, 0.0f));
	}

	USizeBox* HealthBodySize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("HUDHealthBodySize"));
	HealthBodySize->SetWidthOverride(420.0f);
	HealthBodySize->SetHeightOverride(16.0f);
	UOverlay* HealthOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("HUDHealthOverlay"));
	HealthBodySize->SetContent(HealthOverlay);

	UBorder* HealthTrackBg = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HUDHealthTrackBg"));
	HealthTrackBg->SetBrush(FVelmaraUITheme::MakeSolidBrush(FVelmaraUITheme::Charcoal(0.86f)));
	if (UOverlaySlot* BgSlot = HealthOverlay->AddChildToOverlay(HealthTrackBg))
	{
		BgSlot->SetHorizontalAlignment(HAlign_Fill);
		BgSlot->SetVerticalAlignment(VAlign_Fill);
		BgSlot->SetPadding(FMargin(1.5f));
	}

	UTexture2D* HealthFillTex = FVelmaraUITheme::LoadTexture(VelmaraUIPaths::BarHealthFillMask);

	DelayedHealthBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HUDDelayedHealthBar"));
	FProgressBarStyle DelayedStyle;
	DelayedStyle.SetBackgroundImage(EmptyBg);
	DelayedStyle.SetFillImage(FVelmaraUITheme::MakeImageBrush(HealthFillTex, FVector2D(420.0f, 16.0f), FVelmaraUITheme::BloodDried()));
	DelayedHealthBar->SetWidgetStyle(DelayedStyle);
	DelayedHealthBar->SetFillColorAndOpacity(FVelmaraUITheme::BloodDried());
	DelayedHealthBar->SetPercent(1.0f);
	if (UOverlaySlot* DSlot = HealthOverlay->AddChildToOverlay(DelayedHealthBar))
	{
		DSlot->SetHorizontalAlignment(HAlign_Fill);
		DSlot->SetVerticalAlignment(VAlign_Fill);
		DSlot->SetPadding(FMargin(1.5f));
	}

	HealthProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HUDHealthBar"));
	FProgressBarStyle HealthStyle;
	HealthStyle.SetBackgroundImage(EmptyBg);
	HealthStyle.SetFillImage(FVelmaraUITheme::MakeImageBrush(HealthFillTex, FVector2D(420.0f, 16.0f), FVelmaraUITheme::Blood()));
	HealthProgressBar->SetWidgetStyle(HealthStyle);
	HealthProgressBar->SetFillColorAndOpacity(FVelmaraUITheme::Blood());
	HealthProgressBar->SetPercent(1.0f);
	if (UOverlaySlot* HSlot = HealthOverlay->AddChildToOverlay(HealthProgressBar))
	{
		HSlot->SetHorizontalAlignment(HAlign_Fill);
		HSlot->SetVerticalAlignment(VAlign_Fill);
		HSlot->SetPadding(FMargin(1.5f));
	}

	UImage* HealthFrame = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("HUDHealthFrame"));
	HealthFrame->SetBrush(FVelmaraUITheme::MakeHealthBarFrameBrush(FVelmaraUITheme::Bone(0.90f)));
	if (UOverlaySlot* HFSlot = HealthOverlay->AddChildToOverlay(HealthFrame))
	{
		HFSlot->SetHorizontalAlignment(HAlign_Fill);
		HFSlot->SetVerticalAlignment(VAlign_Fill);
	}

	if (UHorizontalBoxSlot* BodyHSlot = HealthRow->AddChildToHorizontalBox(HealthBodySize))
	{
		BodyHSlot->SetVerticalAlignment(VAlign_Center);
	}

	USizeBox* RightFinSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("HUDFinialRSize"));
	RightFinSize->SetWidthOverride(42.0f);
	RightFinSize->SetHeightOverride(28.0f);
	UImage* RightFin = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("HUDFinialR"));
	RightFin->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::BarFinialSimpleR),
		FVector2D(42.0f, 28.0f),
		FVelmaraUITheme::Bone(0.90f)));
	RightFinSize->SetContent(RightFin);
	if (UHorizontalBoxSlot* RFSlot = HealthRow->AddChildToHorizontalBox(RightFinSize))
	{
		RFSlot->SetVerticalAlignment(VAlign_Center);
		RFSlot->SetPadding(FMargin(-14.0f, 0.0f, 0.0f, 0.0f));
	}

	USizeBox* EnergyBarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("HUDEnergyBarSize"));
	EnergyBarSize->SetWidthOverride(300.0f);
	EnergyBarSize->SetHeightOverride(8.0f);
	EnergyProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HUDEnergyBar"));
	FProgressBarStyle EnergyStyle;
	EnergyStyle.SetBackgroundImage(FVelmaraUITheme::MakeSolidBrush(FVelmaraUITheme::Charcoal(0.78f)));
	EnergyStyle.SetFillImage(FVelmaraUITheme::MakeSolidBrush(FVelmaraUITheme::Spectral()));
	EnergyProgressBar->SetWidgetStyle(EnergyStyle);
	EnergyProgressBar->SetFillColorAndOpacity(FVelmaraUITheme::Spectral());
	EnergyProgressBar->SetPercent(1.0f);
	EnergyBarSize->SetContent(EnergyProgressBar);
	if (UVerticalBoxSlot* EnSlot = VitalsVBox->AddChildToVerticalBox(EnergyBarSize))
	{
		EnSlot->SetHorizontalAlignment(HAlign_Left);
		EnSlot->SetPadding(FMargin(28.0f, 0.0f, 0.0f, 7.0f));
	}

	UHorizontalBox* PipsRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HUDEnergyPipsRow"));
	EnergyPipImages.Reset();
	for (int32 PipIdx = 0; PipIdx < 5; ++PipIdx)
	{
		USizeBox* PipSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		PipSize->SetWidthOverride(16.0f);
		PipSize->SetHeightOverride(16.0f);
		UImage* PipImg = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		PipImg->SetBrush(FVelmaraUITheme::MakeImageBrush(
			FVelmaraUITheme::LoadTexture(VelmaraUIPaths::PipDiamondFilled),
			FVector2D(16.0f, 16.0f),
			FVelmaraUITheme::Spectral()));
		PipSize->SetContent(PipImg);
		if (UHorizontalBoxSlot* PSlot = PipsRow->AddChildToHorizontalBox(PipSize))
		{
			PSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
		}
		EnergyPipImages.Add(PipImg);
	}
	if (UVerticalBoxSlot* PipsVSlot = VitalsVBox->AddChildToVerticalBox(PipsRow))
	{
		PipsVSlot->SetHorizontalAlignment(HAlign_Left);
		PipsVSlot->SetPadding(FMargin(28.0f, 0.0f, 0.0f, 12.0f));
	}

	SunlightBadgeRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HUDSunlightBadge"));
	SunlightBadgeRow->SetVisibility(ESlateVisibility::Collapsed);
	USizeBox* SunIconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	SunIconSize->SetWidthOverride(22.0f);
	SunIconSize->SetHeightOverride(22.0f);
	UImage* SunIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	SunIcon->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::IconStatusSunlight),
		FVector2D(22.0f, 22.0f),
		FVelmaraUITheme::Sunlight()));
	SunIconSize->SetContent(SunIcon);
	if (UHorizontalBoxSlot* SISlot = SunlightBadgeRow->AddChildToHorizontalBox(SunIconSize))
	{
		SISlot->SetVerticalAlignment(VAlign_Center);
		SISlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
	}
	UCommonTextBlock* SunLabel = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass());
	SunLabel->SetText(NSLOCTEXT("VelmaraUI", "HUDSunlightWarn", "EXPOSICIÓN SOLAR"));
	SunLabel->SetFont(FVelmaraUITheme::GetDisplayFont(15, 200));
	SunLabel->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Sunlight()));
	SunlightBadgeRow->AddChildToHorizontalBox(SunLabel)->SetVerticalAlignment(VAlign_Center);
	if (UVerticalBoxSlot* SunVSlot = VitalsVBox->AddChildToVerticalBox(SunlightBadgeRow))
	{
		SunVSlot->SetHorizontalAlignment(HAlign_Left);
		SunVSlot->SetPadding(FMargin(28.0f, 2.0f, 0.0f, 6.0f));
	}

	PossessionBadgeRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HUDPossessionBadge"));
	PossessionBadgeRow->SetVisibility(ESlateVisibility::Collapsed);
	USizeBox* PossIconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	PossIconSize->SetWidthOverride(20.0f);
	PossIconSize->SetHeightOverride(20.0f);
	UImage* PossIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	PossIcon->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::IconStatSpectral),
		FVector2D(20.0f, 20.0f),
		FVelmaraUITheme::Spectral()));
	PossIconSize->SetContent(PossIcon);
	if (UHorizontalBoxSlot* PISlot = PossessionBadgeRow->AddChildToHorizontalBox(PossIconSize))
	{
		PISlot->SetVerticalAlignment(VAlign_Center);
		PISlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
	}
	UCommonTextBlock* PossLabel = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass());
	PossLabel->SetText(NSLOCTEXT("VelmaraUI", "HUDPossessionActive", "POSESIÓN ACTIVA"));
	PossLabel->SetFont(FVelmaraUITheme::GetDisplayFont(14, 190));
	PossLabel->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Spectral()));
	PossessionBadgeRow->AddChildToHorizontalBox(PossLabel)->SetVerticalAlignment(VAlign_Center);
	if (UVerticalBoxSlot* PosVSlot = VitalsVBox->AddChildToVerticalBox(PossessionBadgeRow))
	{
		PosVSlot->SetHorizontalAlignment(HAlign_Left);
		PosVSlot->SetPadding(FMargin(28.0f, 2.0f, 0.0f, 0.0f));
	}

	BloodSenseBadgeRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HUDBloodSenseBadge"));
	BloodSenseBadgeRow->SetVisibility(ESlateVisibility::Collapsed);
	USizeBox* BloodSenseIconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	BloodSenseIconSize->SetWidthOverride(16.0f);
	BloodSenseIconSize->SetHeightOverride(16.0f);
	UImage* BloodSenseIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	BloodSenseIcon->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::IconStatBlood),
		FVector2D(16.0f, 16.0f),
		FVelmaraUITheme::BloodBright()));
	BloodSenseIconSize->SetContent(BloodSenseIcon);
	if (UHorizontalBoxSlot* BSISlot = BloodSenseBadgeRow->AddChildToHorizontalBox(BloodSenseIconSize))
	{
		BSISlot->SetVerticalAlignment(VAlign_Center);
		BSISlot->SetPadding(FMargin(2.0f, 0.0f, 10.0f, 0.0f));
	}
	BloodSenseBadgeLabel = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass());
	BloodSenseBadgeLabel->SetText(NSLOCTEXT("VelmaraUI", "HUDBloodSenseAvailable", "POSESIÓN DISPONIBLE"));
	BloodSenseBadgeLabel->SetFont(FVelmaraUITheme::GetDisplayFont(12, 190));
	BloodSenseBadgeLabel->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BloodBright()));
	BloodSenseBadgeRow->AddChildToHorizontalBox(BloodSenseBadgeLabel)->SetVerticalAlignment(VAlign_Center);
	if (UVerticalBoxSlot* BloodSenseVSlot = VitalsVBox->AddChildToVerticalBox(BloodSenseBadgeRow))
	{
		BloodSenseVSlot->SetHorizontalAlignment(HAlign_Left);
		BloodSenseVSlot->SetPadding(FMargin(28.0f, 2.0f, 0.0f, 0.0f));
	}
	UHorizontalBox* LoadoutRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HUDLoadoutRow"));
	if (UOverlaySlot* LoadSlot = RootOverlay->AddChildToOverlay(LoadoutRow))
	{
		LoadSlot->SetHorizontalAlignment(HAlign_Left);
		LoadSlot->SetVerticalAlignment(VAlign_Bottom);
		LoadSlot->SetPadding(FMargin(54.0f, 0.0f, 0.0f, 44.0f));
	}

	const TCHAR* SkillIcons[4] = {
		VelmaraUIPaths::IconStatSpectral,
		VelmaraUIPaths::IconMenuSkills,
		VelmaraUIPaths::IconStatShield,
		VelmaraUIPaths::IconStatDamage
	};

	SkillHintTexts.Reset();
	for (int32 SkillIdx = 0; SkillIdx < 4; ++SkillIdx)
	{
		UVerticalBox* SkillCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		if (UHorizontalBoxSlot* SColSlot = LoadoutRow->AddChildToHorizontalBox(SkillCol))
		{
			SColSlot->SetVerticalAlignment(VAlign_Bottom);
			SColSlot->SetPadding(FMargin(0.0f, 0.0f, 14.0f, 0.0f));
		}

		USizeBox* DiamondSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		DiamondSize->SetWidthOverride(72.0f);
		DiamondSize->SetHeightOverride(72.0f);
		UOverlay* DiamondOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		DiamondSize->SetContent(DiamondOverlay);

		UImage* DiamondFill = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		DiamondFill->SetBrush(FVelmaraUITheme::MakeImageBrush(
			FVelmaraUITheme::LoadTexture(VelmaraUIPaths::SlotDiamondFillMask),
			FVector2D(72.0f, 72.0f),
			FVelmaraUITheme::Void(0.75f)));
		if (UOverlaySlot* DFSlot = DiamondOverlay->AddChildToOverlay(DiamondFill))
		{
			DFSlot->SetHorizontalAlignment(HAlign_Fill);
			DFSlot->SetVerticalAlignment(VAlign_Fill);
		}

		UImage* DiamondBorder = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		DiamondBorder->SetBrush(FVelmaraUITheme::MakeImageBrush(
			FVelmaraUITheme::LoadTexture(VelmaraUIPaths::SlotDiamondNormal),
			FVector2D(72.0f, 72.0f),
			FVelmaraUITheme::Bone(0.88f)));
		if (UOverlaySlot* DBSlot = DiamondOverlay->AddChildToOverlay(DiamondBorder))
		{
			DBSlot->SetHorizontalAlignment(HAlign_Fill);
			DBSlot->SetVerticalAlignment(VAlign_Fill);
		}

		USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		IconSize->SetWidthOverride(30.0f);
		IconSize->SetHeightOverride(30.0f);
		UImage* SkillImg = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		SkillImg->SetBrush(FVelmaraUITheme::MakeImageBrush(
			FVelmaraUITheme::LoadTexture(SkillIcons[SkillIdx]),
			FVector2D(30.0f, 30.0f),
			FVelmaraUITheme::Bone()));
		IconSize->SetContent(SkillImg);
		if (UOverlaySlot* ISlot = DiamondOverlay->AddChildToOverlay(IconSize))
		{
			ISlot->SetHorizontalAlignment(HAlign_Center);
			ISlot->SetVerticalAlignment(VAlign_Center);
		}

		SkillCol->AddChildToVerticalBox(DiamondSize)->SetHorizontalAlignment(HAlign_Center);

		UCommonTextBlock* KeyHint = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass());
		KeyHint->SetFont(FVelmaraUITheme::GetDisplayFont(13, 120));
		KeyHint->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted()));
		KeyHint->SetJustification(ETextJustify::Center);
		if (UVerticalBoxSlot* KSlot = SkillCol->AddChildToVerticalBox(KeyHint))
		{
			KSlot->SetHorizontalAlignment(HAlign_Center);
			KSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
		}
		SkillHintTexts.Add(KeyHint);
	}

	UVerticalBox* WeaponCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HUDWeaponCol"));
	if (UHorizontalBoxSlot* WColSlot = LoadoutRow->AddChildToHorizontalBox(WeaponCol))
	{
		WColSlot->SetVerticalAlignment(VAlign_Bottom);
		WColSlot->SetPadding(FMargin(10.0f, 0.0f, 0.0f, 0.0f));
	}

	USizeBox* WeaponSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("HUDWeaponSlotSize"));
	WeaponSize->SetWidthOverride(82.0f);
	WeaponSize->SetHeightOverride(82.0f);
	UOverlay* WeaponOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("HUDWeaponOverlay"));
	WeaponSize->SetContent(WeaponOverlay);

	UBorder* WeaponBg = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	WeaponBg->SetBrush(FVelmaraUITheme::MakeSolidBrush(FVelmaraUITheme::Void(0.68f)));
	if (UOverlaySlot* WBgSlot = WeaponOverlay->AddChildToOverlay(WeaponBg))
	{
		WBgSlot->SetHorizontalAlignment(HAlign_Fill);
		WBgSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UImage* WeaponBrackets = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	WeaponBrackets->SetBrush(FVelmaraUITheme::MakeBracketsMinimalBrush(FVelmaraUITheme::Bone(0.90f)));
	if (UOverlaySlot* WFrSlot = WeaponOverlay->AddChildToOverlay(WeaponBrackets))
	{
		WFrSlot->SetHorizontalAlignment(HAlign_Fill);
		WFrSlot->SetVerticalAlignment(VAlign_Fill);
	}

	USizeBox* WepIconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	WepIconSize->SetWidthOverride(52.0f);
	WepIconSize->SetHeightOverride(52.0f);
	ActiveWeaponIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("HUDActiveWeaponIcon"));
	ActiveWeaponIcon->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::IconWeaponLongsword),
		FVector2D(52.0f, 52.0f),
		FVelmaraUITheme::Bone()));
	WepIconSize->SetContent(ActiveWeaponIcon);
	if (UOverlaySlot* WISlot = WeaponOverlay->AddChildToOverlay(WepIconSize))
	{
		WISlot->SetHorizontalAlignment(HAlign_Center);
		WISlot->SetVerticalAlignment(VAlign_Center);
	}

	WeaponCol->AddChildToVerticalBox(WeaponSize)->SetHorizontalAlignment(HAlign_Center);

	UCommonTextBlock* WeaponKeyHint = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass());
	WeaponKeyHint->SetText(FText::FromString(TEXT("TAB")));
	WeaponKeyHint->SetFont(FVelmaraUITheme::GetDisplayFont(13, 120));
	WeaponKeyHint->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted()));
	WeaponCol->AddChildToVerticalBox(WeaponKeyHint)->SetHorizontalAlignment(HAlign_Center);

	UHorizontalBox* BloodCounterRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HUDBloodCounterRow"));
	if (UOverlaySlot* BloodSlot = RootOverlay->AddChildToOverlay(BloodCounterRow))
	{
		BloodSlot->SetHorizontalAlignment(HAlign_Right);
		BloodSlot->SetVerticalAlignment(VAlign_Bottom);
		BloodSlot->SetPadding(FMargin(0.0f, 0.0f, 60.0f, 52.0f));
	}

	USizeBox* BloodDropSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	BloodDropSize->SetWidthOverride(24.0f);
	BloodDropSize->SetHeightOverride(24.0f);
	UImage* BloodDropImg = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	BloodDropImg->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::IconStatBlood),
		FVector2D(24.0f, 24.0f),
		FVelmaraUITheme::Bone()));
	BloodDropSize->SetContent(BloodDropImg);
	if (UHorizontalBoxSlot* DropSlot = BloodCounterRow->AddChildToHorizontalBox(BloodDropSize))
	{
		DropSlot->SetVerticalAlignment(VAlign_Center);
		DropSlot->SetPadding(FMargin(0.0f, 0.0f, 12.0f, 0.0f));
	}

	BloodCounterText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("HUDBloodCounterText"));
	BloodCounterText->SetText(FText::FromString(TEXT("0")));
	BloodCounterText->SetFont(FVelmaraUITheme::GetDisplayFont(26, 160));
	BloodCounterText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
	if (UHorizontalBoxSlot* BTextSlot = BloodCounterRow->AddChildToHorizontalBox(BloodCounterText))
	{
		BTextSlot->SetVerticalAlignment(VAlign_Center);
	}

	RefreshSkillKeyHints();
}

void UVelmaraGameplayHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ULocalPlayer* LP = GetOwningLocalPlayer())
	{
		if (UCommonInputSubsystem* InputSys = UCommonInputSubsystem::Get(LP))
		{
			InputMethodHandle = InputSys->OnInputMethodChangedNative.AddUObject(this, &UVelmaraGameplayHUDWidget::HandleInputMethodChanged);
		}
	}

	if (APlayerController* PC = GetOwningPlayer())
	{
		BindToPlayerPawn(PC->GetPawn());
	}
	RefreshSkillKeyHints();
}

void UVelmaraGameplayHUDWidget::NativeDestruct()
{
	UnbindFromCurrentPawn();

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

void UVelmaraGameplayHUDWidget::UnbindFromCurrentPawn()
{
	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		if (HealthChangedHandle.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(UVelmaraAttributeSet::GetHealthAttribute()).Remove(HealthChangedHandle);
		}
		if (MaxHealthChangedHandle.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(UVelmaraAttributeSet::GetMaxHealthAttribute()).Remove(MaxHealthChangedHandle);
		}
		if (ShieldChangedHandle.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(UVelmaraAttributeSet::GetShieldAttribute()).Remove(ShieldChangedHandle);
		}
		if (MaxShieldChangedHandle.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(UVelmaraAttributeSet::GetMaxShieldAttribute()).Remove(MaxShieldChangedHandle);
		}
		if (EnergyChangedHandle.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(UVelmaraAttributeSet::GetEnergyAttribute()).Remove(EnergyChangedHandle);
		}
		if (MaxEnergyChangedHandle.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(UVelmaraAttributeSet::GetMaxEnergyAttribute()).Remove(MaxEnergyChangedHandle);
		}
	}
	HealthChangedHandle.Reset();
	MaxHealthChangedHandle.Reset();
	ShieldChangedHandle.Reset();
	MaxShieldChangedHandle.Reset();
	EnergyChangedHandle.Reset();
	MaxEnergyChangedHandle.Reset();

	if (UProgressionComponent* Prog = BoundProgression.Get())
	{
		Prog->OnCurrencyChanged.RemoveDynamic(this, &UVelmaraGameplayHUDWidget::HandleCurrencyChanged);
	}
	if (UInventoryComponent* Inv = BoundInventory.Get())
	{
		Inv->OnItemEquipped.RemoveDynamic(this, &UVelmaraGameplayHUDWidget::HandleItemEquipped);
	}

	BoundASC.Reset();
	BoundProgression.Reset();
	BoundInventory.Reset();
	BoundPawn.Reset();
}

void UVelmaraGameplayHUDWidget::BindToPlayerPawn(APawn* PlayerPawn)
{
	if (BoundPawn.Get() == PlayerPawn && PlayerPawn != nullptr)
	{
		return;
	}

	UnbindFromCurrentPawn();
	BoundPawn = PlayerPawn;
	if (!PlayerPawn)
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PlayerPawn))
	{
		BoundASC = ASC;
		HealthChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(UVelmaraAttributeSet::GetHealthAttribute()).AddUObject(this, &UVelmaraGameplayHUDWidget::OnHealthChanged);
		MaxHealthChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(UVelmaraAttributeSet::GetMaxHealthAttribute()).AddUObject(this, &UVelmaraGameplayHUDWidget::OnMaxHealthChanged);
		ShieldChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(UVelmaraAttributeSet::GetShieldAttribute()).AddUObject(this, &UVelmaraGameplayHUDWidget::OnShieldChanged);
		MaxShieldChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(UVelmaraAttributeSet::GetMaxShieldAttribute()).AddUObject(this, &UVelmaraGameplayHUDWidget::OnMaxShieldChanged);
		EnergyChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(UVelmaraAttributeSet::GetEnergyAttribute()).AddUObject(this, &UVelmaraGameplayHUDWidget::OnEnergyChanged);
		MaxEnergyChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(UVelmaraAttributeSet::GetMaxEnergyAttribute()).AddUObject(this, &UVelmaraGameplayHUDWidget::OnMaxEnergyChanged);
	}

	APawn* PrimaryBody = PlayerPawn;
	if (ACustomPlayerController* CustomPC = Cast<ACustomPlayerController>(GetOwningPlayer()))
	{
		if (CustomPC->GetOriginalBody())
		{
			PrimaryBody = CustomPC->GetOriginalBody();
		}
	}

	if (UProgressionComponent* Prog = PrimaryBody->FindComponentByClass<UProgressionComponent>())
	{
		BoundProgression = Prog;
		Prog->OnCurrencyChanged.AddUniqueDynamic(this, &UVelmaraGameplayHUDWidget::HandleCurrencyChanged);
		UpdateBloodCounterImmediate(Prog->GetAvailableCurrency());
	}

	if (UInventoryComponent* Inv = PrimaryBody->FindComponentByClass<UInventoryComponent>())
	{
		BoundInventory = Inv;
		Inv->OnItemEquipped.AddUniqueDynamic(this, &UVelmaraGameplayHUDWidget::HandleItemEquipped);
		UpdateWeaponSlotIcon(Inv->GetCurrentWeapon());
	}

	RefreshAllVitals();
}

void UVelmaraGameplayHUDWidget::RefreshAllVitals()
{
	UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC)
	{
		return;
	}

	const float Health = ASC->GetNumericAttribute(UVelmaraAttributeSet::GetHealthAttribute());
	const float MaxHealth = ASC->GetNumericAttribute(UVelmaraAttributeSet::GetMaxHealthAttribute());
	const float Shield = ASC->GetNumericAttribute(UVelmaraAttributeSet::GetShieldAttribute());
	const float MaxShield = ASC->GetNumericAttribute(UVelmaraAttributeSet::GetMaxShieldAttribute());
	const float Energy = ASC->GetNumericAttribute(UVelmaraAttributeSet::GetEnergyAttribute());
	const float MaxEnergy = ASC->GetNumericAttribute(UVelmaraAttributeSet::GetMaxEnergyAttribute());

	UpdateHealthBar(Health, MaxHealth);
	UpdateShieldBar(Shield, MaxShield);
	UpdateEnergyBar(Energy, MaxEnergy);
}

void UVelmaraGameplayHUDWidget::UpdateHealthBar(float Health, float MaxHealth)
{
	const float NewPercent = MaxHealth > 0.0f ? FMath::Clamp(Health / MaxHealth, 0.0f, 1.0f) : 0.0f;
	if (NewPercent < TargetHealthPercent)
	{
		DelayedHealthHoldTimer = 0.35f;
	}
	else if (NewPercent > DisplayedDelayedHealth)
	{
		DisplayedDelayedHealth = NewPercent;
		if (DelayedHealthBar)
		{
			DelayedHealthBar->SetPercent(DisplayedDelayedHealth);
		}
	}

	TargetHealthPercent = NewPercent;
	if (HealthProgressBar)
	{
		HealthProgressBar->SetPercent(TargetHealthPercent);
	}
}

void UVelmaraGameplayHUDWidget::UpdateShieldBar(float Shield, float MaxShield)
{
	const float Percent = MaxShield > 0.0f ? FMath::Clamp(Shield / MaxShield, 0.0f, 1.0f) : 0.0f;
	if (ShieldProgressBar)
	{
		ShieldProgressBar->SetPercent(Percent);
	}
	if (ShieldBarRowSize)
	{
		ShieldBarRowSize->SetVisibility(MaxShield > 0.0f && Shield > 0.0f ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UVelmaraGameplayHUDWidget::UpdateEnergyBar(float Energy, float MaxEnergy)
{
	const float Percent = MaxEnergy > 0.0f ? FMath::Clamp(Energy / MaxEnergy, 0.0f, 1.0f) : 0.0f;
	if (EnergyProgressBar)
	{
		EnergyProgressBar->SetPercent(Percent);
	}

	const int32 FilledPips = FMath::FloorToInt(Percent * 5.0f + 0.01f);
	UTexture2D* FilledTex = FVelmaraUITheme::LoadTexture(VelmaraUIPaths::PipDiamondFilled);
	UTexture2D* HollowTex = FVelmaraUITheme::LoadTexture(VelmaraUIPaths::PipDiamondHollow);

	for (int32 Index = 0; Index < EnergyPipImages.Num(); ++Index)
	{
		if (UImage* PipImg = EnergyPipImages[Index])
		{
			const bool bFilled = Index < FilledPips;
			PipImg->SetBrush(FVelmaraUITheme::MakeImageBrush(
				bFilled ? FilledTex : HollowTex,
				FVector2D(16.0f, 16.0f),
				bFilled ? FVelmaraUITheme::Spectral() : FVelmaraUITheme::Ash()));
		}
	}
}

void UVelmaraGameplayHUDWidget::UpdateBloodCounterImmediate(int32 Amount)
{
	TargetBloodValue = Amount;
	DisplayedBloodValue = static_cast<float>(Amount);
	if (BloodCounterText)
	{
		BloodCounterText->SetText(FVelmaraUITheme::FormatGroupedNumber(Amount));
	}
}

void UVelmaraGameplayHUDWidget::UpdateWeaponSlotIcon(TScriptInterface<IPickable> WeaponPickable)
{
	if (!ActiveWeaponIcon)
	{
		return;
	}

	UTexture2D* ChosenIcon = FVelmaraUITheme::LoadTexture(VelmaraUIPaths::IconWeaponLongsword);
	if (UObject* Obj = WeaponPickable.GetObject())
	{
		const FString ClassName = Obj->GetClass()->GetName();
		if (ClassName.Contains(TEXT("Dual"), ESearchCase::IgnoreCase))
		{
			ChosenIcon = FVelmaraUITheme::LoadTexture(VelmaraUIPaths::IconWeaponDualBlades);
		}
		else if (ClassName.Contains(TEXT("Spear"), ESearchCase::IgnoreCase) || ClassName.Contains(TEXT("Lanza"), ESearchCase::IgnoreCase))
		{
			ChosenIcon = FVelmaraUITheme::LoadTexture(VelmaraUIPaths::IconWeaponSpear);
		}
		else if (ClassName.Contains(TEXT("Scythe"), ESearchCase::IgnoreCase) || ClassName.Contains(TEXT("Guada"), ESearchCase::IgnoreCase))
		{
			ChosenIcon = FVelmaraUITheme::LoadTexture(VelmaraUIPaths::IconWeaponScythe);
		}
		else if (ClassName.Contains(TEXT("Pistol"), ESearchCase::IgnoreCase) || ClassName.Contains(TEXT("Flintlock"), ESearchCase::IgnoreCase))
		{
			ChosenIcon = FVelmaraUITheme::LoadTexture(VelmaraUIPaths::IconWeaponFlintlock);
		}
	}

	ActiveWeaponIcon->SetBrush(FVelmaraUITheme::MakeImageBrush(ChosenIcon, FVector2D(52.0f, 52.0f), FVelmaraUITheme::Bone()));
}

void UVelmaraGameplayHUDWidget::RefreshSkillKeyHints()
{
	bool bGamepad = false;
	if (ULocalPlayer* LP = GetOwningLocalPlayer())
	{
		if (const UCommonInputSubsystem* InputSys = UCommonInputSubsystem::Get(LP))
		{
			bGamepad = InputSys->GetCurrentInputType() == ECommonInputType::Gamepad;
		}
	}

	const TCHAR* KbmLabels[4] = { TEXT("Q"), TEXT("F"), TEXT("RMB"), TEXT("R") };
	const TCHAR* PadLabels[4] = { TEXT("LB"), TEXT("RB"), TEXT("LT"), TEXT("RT") };

	for (int32 Index = 0; Index < SkillHintTexts.Num() && Index < 4; ++Index)
	{
		if (SkillHintTexts[Index])
		{
			SkillHintTexts[Index]->SetText(FText::FromString(bGamepad ? PadLabels[Index] : KbmLabels[Index]));
		}
	}
}

void UVelmaraGameplayHUDWidget::HandleInputMethodChanged(ECommonInputType NewInputType)
{
	RefreshSkillKeyHints();
}

void UVelmaraGameplayHUDWidget::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		UpdateHealthBar(Data.NewValue, ASC->GetNumericAttribute(UVelmaraAttributeSet::GetMaxHealthAttribute()));
	}
}

void UVelmaraGameplayHUDWidget::OnMaxHealthChanged(const FOnAttributeChangeData& Data)
{
	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		UpdateHealthBar(ASC->GetNumericAttribute(UVelmaraAttributeSet::GetHealthAttribute()), Data.NewValue);
	}
}

void UVelmaraGameplayHUDWidget::OnShieldChanged(const FOnAttributeChangeData& Data)
{
	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		UpdateShieldBar(Data.NewValue, ASC->GetNumericAttribute(UVelmaraAttributeSet::GetMaxShieldAttribute()));
	}
}

void UVelmaraGameplayHUDWidget::OnMaxShieldChanged(const FOnAttributeChangeData& Data)
{
	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		UpdateShieldBar(ASC->GetNumericAttribute(UVelmaraAttributeSet::GetShieldAttribute()), Data.NewValue);
	}
}

void UVelmaraGameplayHUDWidget::OnEnergyChanged(const FOnAttributeChangeData& Data)
{
	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		UpdateEnergyBar(Data.NewValue, ASC->GetNumericAttribute(UVelmaraAttributeSet::GetMaxEnergyAttribute()));
	}
}

void UVelmaraGameplayHUDWidget::OnMaxEnergyChanged(const FOnAttributeChangeData& Data)
{
	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		UpdateEnergyBar(ASC->GetNumericAttribute(UVelmaraAttributeSet::GetEnergyAttribute()), Data.NewValue);
	}
}

void UVelmaraGameplayHUDWidget::HandleCurrencyChanged(int32 NewAmount)
{
	TargetBloodValue = NewAmount;
}

void UVelmaraGameplayHUDWidget::HandleItemEquipped(TScriptInterface<IPickable> NewWeapon)
{
	UpdateWeaponSlotIcon(NewWeapon);
}

void UVelmaraGameplayHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (APlayerController* PC = GetOwningPlayer())
	{
		APawn* CurrentPawn = PC->GetPawn();
		if (CurrentPawn != BoundPawn.Get())
		{
			BindToPlayerPawn(CurrentPawn);
		}

		const ACustomPlayerController* CustomPC = Cast<ACustomPlayerController>(PC);
		const bool bPossessing = CustomPC && CustomPC->IsPossessing();

		if (PossessionBadgeRow)
		{
			PossessionBadgeRow->SetVisibility(bPossessing ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}

		if (BloodSenseBadgeRow && BloodSenseBadgeLabel)
		{
			bool bShowBadge = false;
			if (!bPossessing && GetWorld())
			{
				const UBloodMarkSubsystem* BloodMarks = GetWorld()->GetSubsystem<UBloodMarkSubsystem>();
				const USpectralObjectsSubsystem* SpectralObjects = GetWorld()->GetSubsystem<USpectralObjectsSubsystem>();

				if (BloodMarks && BloodMarks->GetMarkedEnemyCount() > 0)
				{
					bool bEnoughEnergy = true;
					if (const UAbilitySystemComponent* ASC = BoundASC.Get())
					{
						const FGameplayAttribute EnergyAttribute = UVelmaraAttributeSet::GetEnergyAttribute();
						const float MinEnergy = CustomPC ? CustomPC->MinEnergyToStartPossession : 0.f;
						bEnoughEnergy = !ASC->HasAttributeSetForAttribute(EnergyAttribute) || ASC->GetNumericAttribute(EnergyAttribute) >= MinEnergy;
					}

					BloodSenseBadgeLabel->SetText(bEnoughEnergy
						? NSLOCTEXT("VelmaraUI", "HUDBloodSenseAvailable", "POSESIÓN DISPONIBLE")
						: NSLOCTEXT("VelmaraUI", "HUDBloodSenseMarked", "PRESAS MARCADAS"));
					BloodSenseBadgeLabel->SetColorAndOpacity(FSlateColor(bEnoughEnergy ? FVelmaraUITheme::BloodBright() : FVelmaraUITheme::Blood()));
					bShowBadge = true;
				}
				else if (SpectralObjects && SpectralObjects->GetRevealedObjectCount() > 0)
				{
					BloodSenseBadgeLabel->SetText(NSLOCTEXT("VelmaraUI", "HUDSpectralResonance", "VISIÓN ESPECTRAL"));
					BloodSenseBadgeLabel->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Spectral()));
					bShowBadge = true;
				}
			}
			BloodSenseBadgeRow->SetVisibility(bShowBadge ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}

		if (SunlightBadgeRow && CurrentPawn)
		{
			const USunlightExposureSubsystem* SunSys = GetWorld() ? GetWorld()->GetSubsystem<USunlightExposureSubsystem>() : nullptr;
			const bool bExposed = SunSys && SunSys->IsExposed(CurrentPawn);
			SunlightBadgeRow->SetVisibility(bExposed ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
			if (bExposed)
			{
				SunlightPulseTime += InDeltaTime * 4.5f;
				SunlightBadgeRow->SetRenderOpacity(0.75f + 0.25f * FMath::Sin(SunlightPulseTime));
			}
		}
	}

	if (DisplayedDelayedHealth > TargetHealthPercent)
	{
		if (DelayedHealthHoldTimer > 0.0f)
		{
			DelayedHealthHoldTimer -= InDeltaTime;
		}
		else
		{
			DisplayedDelayedHealth = FMath::FInterpTo(DisplayedDelayedHealth, TargetHealthPercent, InDeltaTime, 3.8f);
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

	if (BloodCounterText && !FMath::IsNearlyEqual(DisplayedBloodValue, static_cast<float>(TargetBloodValue), 0.5f))
	{
		DisplayedBloodValue = FMath::FInterpTo(DisplayedBloodValue, static_cast<float>(TargetBloodValue), InDeltaTime, 7.0f);
		BloodCounterText->SetText(FVelmaraUITheme::FormatGroupedNumber(FMath::RoundToInt(DisplayedBloodValue)));
	}
}
