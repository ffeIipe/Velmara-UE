#include "Features/WeaponUpgradeSystem/UI/WeaponComboRowWidget.h"

#include "../../UpgradeSystem/UI/UpgradeUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "DataAssets/Items/Weapons/WeaponData.h"
#include "Engine/Texture2D.h"
#include "Features/WeaponUpgradeSystem/Core/WeaponUpgradeComponent.h"
#include "Features/WeaponUpgradeSystem/UI/WeaponUpgradeMenuWidget.h"
#include "UI/Style/VelmaraUITheme.h"

#define LOCTEXT_NAMESPACE "WeaponUpgrade"

void UWeaponComboRowWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
}

void UWeaponComboRowWidget::Setup(UWeaponUpgradeComponent* InWeaponUpgrades, UWeaponData* InWeapon, const FGameplayTag& InUnlockTag, UWeaponUpgradeMenuWidget* InOwnerMenu)
{
	WeaponUpgrades = InWeaponUpgrades;
	Weapon = InWeapon;
	UnlockTag = InUnlockTag;
	OwnerMenu = InOwnerMenu;

	Refresh();
}

UWidget* UWeaponComboRowWidget::GetFocusTarget() const
{
	return RowButton;
}

void UWeaponComboRowWidget::BuildLayout()
{
	using namespace UpgradeUIStyle;

	UHorizontalBox* OuterRow = Make<UHorizontalBox>(WidgetTree);
	WidgetTree->RootWidget = OuterRow;

	USizeBox* StarBox = Make<USizeBox>(WidgetTree);
	StarBox->SetWidthOverride(22.f);
	StarBox->SetHeightOverride(22.f);
	FocusStarImage = Make<UImage>(WidgetTree);
	FocusStarImage->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnRowStar),
		FVector2D(20.f, 20.f),
		FVelmaraUITheme::Bone()));
	FocusStarImage->SetVisibility(ESlateVisibility::Hidden);
	StarBox->SetContent(FocusStarImage);
	if (UHorizontalBoxSlot* StarSlot = OuterRow->AddChildToHorizontalBox(StarBox))
	{
		StarSlot->SetVerticalAlignment(VAlign_Center);
		StarSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
	}

	USizeBox* ButtonSize = Make<USizeBox>(WidgetTree);
	ButtonSize->SetMinDesiredHeight(82.f);
	if (UHorizontalBoxSlot* BtnWrapperSlot = OuterRow->AddChildToHorizontalBox(ButtonSize))
	{
		BtnWrapperSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		BtnWrapperSlot->SetVerticalAlignment(VAlign_Center);
	}

	RowButton = Make<UButton>(WidgetTree);
	RowButton->SetStyle(MakeTransparentButtonStyle());
	RowButton->OnClicked.AddDynamic(this, &UWeaponComboRowWidget::HandleUnlockClicked);
	RowButton->OnHovered.AddDynamic(this, &UWeaponComboRowWidget::HandleRowHovered);
	RowButton->OnUnhovered.AddDynamic(this, &UWeaponComboRowWidget::HandleRowUnhovered);
	ButtonSize->SetContent(RowButton);

	UOverlay* CardOverlay = Make<UOverlay>(WidgetTree);
	if (UButtonSlot* RootBtnSlot = Cast<UButtonSlot>(RowButton->SetContent(CardOverlay)))
	{
		RootBtnSlot->SetHorizontalAlignment(HAlign_Fill);
		RootBtnSlot->SetVerticalAlignment(VAlign_Fill);
		RootBtnSlot->SetPadding(FMargin(0.f));
	}

	FillBorder = Make<UBorder>(WidgetTree);
	FillBorder->SetBrush(FVelmaraUITheme::MakeSolidBrush(FVelmaraUITheme::Charcoal(0.74f)));
	if (UOverlaySlot* FillSlot = CardOverlay->AddChildToOverlay(FillBorder))
	{
		FillSlot->SetHorizontalAlignment(HAlign_Fill);
		FillSlot->SetVerticalAlignment(VAlign_Fill);
	}

	HoverWashImage = Make<UImage>(WidgetTree);
	HoverWashImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	HoverWashImage->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::GradHFadeLeftToRight),
		FVector2D(64.f, 64.f),
		FVelmaraUITheme::Blood(0.55f)));
	HoverWashImage->SetRenderOpacity(0.f);
	if (UOverlaySlot* WashSlot = CardOverlay->AddChildToOverlay(HoverWashImage))
	{
		WashSlot->SetHorizontalAlignment(HAlign_Fill);
		WashSlot->SetVerticalAlignment(VAlign_Fill);
	}

	FrameBorderImage = Make<UImage>(WidgetTree);
	FrameBorderImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	FrameBorderImage->SetBrush(FVelmaraUITheme::MakeOptionRowBrush(FVelmaraUITheme::Ash()));
	if (UOverlaySlot* FrameSlot = CardOverlay->AddChildToOverlay(FrameBorderImage))
	{
		FrameSlot->SetHorizontalAlignment(HAlign_Fill);
		FrameSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UHorizontalBox* Columns = Make<UHorizontalBox>(WidgetTree);
	if (UOverlaySlot* ColSlot = CardOverlay->AddChildToOverlay(Columns))
	{
		ColSlot->SetHorizontalAlignment(HAlign_Fill);
		ColSlot->SetVerticalAlignment(VAlign_Center);
		ColSlot->SetPadding(FMargin(20.f, 12.f, 18.f, 12.f));
	}

	IconBox = Make<USizeBox>(WidgetTree);
	IconBox->SetWidthOverride(36.f);
	IconBox->SetHeightOverride(36.f);
	IconImage = Make<UImage>(WidgetTree);
	IconBox->SetContent(IconImage);
	if (UHorizontalBoxSlot* IconSlot = Columns->AddChildToHorizontalBox(IconBox))
	{
		IconSlot->SetVerticalAlignment(VAlign_Center);
		IconSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
	}

	USizeBox* LeftDivSize = Make<USizeBox>(WidgetTree);
	LeftDivSize->SetWidthOverride(1.f);
	LeftDivSize->SetHeightOverride(36.f);
	UBorder* LeftDiv = Make<UBorder>(WidgetTree);
	LeftDiv->SetBrush(FVelmaraUITheme::MakeSolidBrush(FVelmaraUITheme::Ash(0.75f)));
	LeftDivSize->SetContent(LeftDiv);
	if (UHorizontalBoxSlot* LDivSlot = Columns->AddChildToHorizontalBox(LeftDivSize))
	{
		LDivSlot->SetVerticalAlignment(VAlign_Center);
		LDivSlot->SetPadding(FMargin(0.f, 0.f, 18.f, 0.f));
	}

	UVerticalBox* InfoCol = Make<UVerticalBox>(WidgetTree);
	if (UHorizontalBoxSlot* InfoSlot = Columns->AddChildToHorizontalBox(InfoCol))
	{
		InfoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		InfoSlot->SetVerticalAlignment(VAlign_Center);
		InfoSlot->SetPadding(FMargin(0.f, 0.f, 14.f, 0.f));
	}

	NameText = MakeDisplayText(WidgetTree, FText::GetEmpty(), 17, FVelmaraUITheme::Bone(), 140);
	InfoCol->AddChildToVerticalBox(NameText);

	InputText = MakeText(WidgetTree, FText::GetEmpty(), 13, FVelmaraUITheme::Spectral(), true);
	if (UVerticalBoxSlot* InpSlot = InfoCol->AddChildToVerticalBox(InputText))
	{
		InpSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	}

	UHorizontalBox* StateBox = Make<UHorizontalBox>(WidgetTree);
	USizeBox* DiaSize = Make<USizeBox>(WidgetTree);
	DiaSize->SetWidthOverride(14.f);
	DiaSize->SetHeightOverride(14.f);
	StateDiamondImage = Make<UImage>(WidgetTree);
	DiaSize->SetContent(StateDiamondImage);
	if (UHorizontalBoxSlot* DiaSlot = StateBox->AddChildToHorizontalBox(DiaSize))
	{
		DiaSlot->SetVerticalAlignment(VAlign_Center);
		DiaSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
	}

	StateText = MakeDisplayText(WidgetTree, FText::GetEmpty(), 12, FVelmaraUITheme::BoneMuted(), 140);
	if (UHorizontalBoxSlot* StTxtSlot = StateBox->AddChildToHorizontalBox(StateText))
	{
		StTxtSlot->SetVerticalAlignment(VAlign_Center);
	}

	USizeBox* StateWidthBox = Make<USizeBox>(WidgetTree);
	StateWidthBox->SetMinDesiredWidth(138.f);
	StateWidthBox->SetContent(StateBox);
	if (UHorizontalBoxSlot* StOuterSlot = Columns->AddChildToHorizontalBox(StateWidthBox))
	{
		StOuterSlot->SetVerticalAlignment(VAlign_Center);
		StOuterSlot->SetPadding(FMargin(0.f, 0.f, 14.f, 0.f));
	}

	USizeBox* RightDivSize = Make<USizeBox>(WidgetTree);
	RightDivSize->SetWidthOverride(1.f);
	RightDivSize->SetHeightOverride(36.f);
	UBorder* RightDiv = Make<UBorder>(WidgetTree);
	RightDiv->SetBrush(FVelmaraUITheme::MakeSolidBrush(FVelmaraUITheme::Ash(0.75f)));
	RightDivSize->SetContent(RightDiv);
	if (UHorizontalBoxSlot* RDivSlot = Columns->AddChildToHorizontalBox(RightDivSize))
	{
		RDivSlot->SetVerticalAlignment(VAlign_Center);
		RDivSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
	}

	USizeBox* CostSection = Make<USizeBox>(WidgetTree);
	CostSection->SetMinDesiredWidth(104.f);
	UHorizontalBox* CostBox = Make<UHorizontalBox>(WidgetTree);
	CostSection->SetContent(CostBox);

	USizeBox* BloodIconSize = Make<USizeBox>(WidgetTree);
	BloodIconSize->SetWidthOverride(16.f);
	BloodIconSize->SetHeightOverride(16.f);
	CostBloodIcon = Make<UImage>(WidgetTree);
	CostBloodIcon->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::IconStatBlood),
		FVector2D(16.f, 16.f),
		FVelmaraUITheme::Bone()));
	BloodIconSize->SetContent(CostBloodIcon);
	if (UHorizontalBoxSlot* BloodSlot = CostBox->AddChildToHorizontalBox(BloodIconSize))
	{
		BloodSlot->SetVerticalAlignment(VAlign_Center);
		BloodSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
	}

	CostText = MakeDisplayText(WidgetTree, FText::GetEmpty(), 16, FVelmaraUITheme::Bone(), 80);
	if (UHorizontalBoxSlot* CostTxtSlot = CostBox->AddChildToHorizontalBox(CostText))
	{
		CostTxtSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		CostTxtSlot->SetVerticalAlignment(VAlign_Center);
	}

	USizeBox* ChevSize = Make<USizeBox>(WidgetTree);
	ChevSize->SetWidthOverride(12.f);
	ChevSize->SetHeightOverride(16.f);
	ChevronImage = Make<UImage>(WidgetTree);
	ChevronImage->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnChevronR),
		FVector2D(12.f, 16.f),
		FVelmaraUITheme::BoneMuted()));
	ChevSize->SetContent(ChevronImage);
	if (UHorizontalBoxSlot* ChevSlot = CostBox->AddChildToHorizontalBox(ChevSize))
	{
		ChevSlot->SetVerticalAlignment(VAlign_Center);
		ChevSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
	}

	if (UHorizontalBoxSlot* CostOuterSlot = Columns->AddChildToHorizontalBox(CostSection))
	{
		CostOuterSlot->SetVerticalAlignment(VAlign_Center);
	}
}

UTexture2D* UWeaponComboRowWidget::ResolveFallbackIcon() const
{
	const UWeaponData* WeaponData = Weapon.Get();
	if (WeaponData && WeaponData->Icon)
	{
		return WeaponData->Icon.Get();
	}

	const FString WeaponName = WeaponData ? WeaponUpgradeUtils::GetWeaponDisplayName(WeaponData).ToString().ToLower() : FString();
	if (WeaponName.Contains(TEXT("dual")) || WeaponName.Contains(TEXT("doble")))
	{
		return FVelmaraUITheme::LoadTexture(VelmaraUIPaths::IconWeaponDualBlades);
	}
	if (WeaponName.Contains(TEXT("pistol")) || WeaponName.Contains(TEXT("arma de fuego")))
	{
		return FVelmaraUITheme::LoadTexture(VelmaraUIPaths::IconWeaponFlintlock);
	}
	if (WeaponName.Contains(TEXT("lanza")) || WeaponName.Contains(TEXT("spear")))
	{
		return FVelmaraUITheme::LoadTexture(VelmaraUIPaths::IconWeaponSpear);
	}
	if (WeaponName.Contains(TEXT("guada")) || WeaponName.Contains(TEXT("scythe")))
	{
		return FVelmaraUITheme::LoadTexture(VelmaraUIPaths::IconWeaponScythe);
	}
	return FVelmaraUITheme::LoadTexture(VelmaraUIPaths::IconWeaponLongsword);
}

void UWeaponComboRowWidget::Refresh()
{
	UWeaponUpgradeComponent* Upgrades = WeaponUpgrades.Get();
	const UWeaponData* WeaponData = Weapon.Get();
	FWeaponComboUpgrade Upgrade;

	if (!Upgrades || !WeaponData || !NameText || !Upgrades->GetComboUpgrade(WeaponData, UnlockTag, Upgrade)) return;

	const EWeaponComboState State = Upgrades->GetComboState(WeaponData, UnlockTag);
	const bool bCanAfford = Upgrades->CanAffordCombo(WeaponData, UnlockTag);

	bIsUnlocked = State == EWeaponComboState::Unlocked;
	bIsAvailable = State == EWeaponComboState::Available;

	NameText->SetText(Upgrade.DisplayName.ToUpper());

	UTexture2D* IconTex = Upgrade.Icon ? Upgrade.Icon.Get() : ResolveFallbackIcon();
	if (IconTex)
	{
		IconImage->SetBrush(FVelmaraUITheme::MakeImageBrush(IconTex, FVector2D(34.f, 34.f), FVelmaraUITheme::Bone()));
		IconBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else
	{
		IconBox->SetVisibility(ESlateVisibility::Collapsed);
	}

	const FComboInputPath Path = WeaponUpgradeUtils::FindComboPath(WeaponData, UnlockTag);
	const FText InputDescription = WeaponUpgradeUtils::DescribeComboInput(Path);
	InputText->SetText(Path.bFound && Path.HitCount > 0
		? FText::Format(LOCTEXT("ComboInputWithHits", "{0}  ·  {1} {1}|plural(one=golpe,other=golpes)"), InputDescription, Path.HitCount)
		: InputDescription);
	InputText->SetColorAndOpacity(FSlateColor(Path.bFound ? FVelmaraUITheme::Spectral() : FVelmaraUITheme::BoneMuted()));

	UTexture2D* FilledDia = FVelmaraUITheme::LoadTexture(VelmaraUIPaths::PipDiamondFilled);
	UTexture2D* HollowDia = FVelmaraUITheme::LoadTexture(VelmaraUIPaths::PipDiamondHollow);

	switch (State)
	{
	case EWeaponComboState::Unlocked:
		StateDiamondImage->SetBrush(FVelmaraUITheme::MakeImageBrush(FilledDia, FVector2D(14.f, 14.f), FVelmaraUITheme::Bone()));
		StateText->SetText(LOCTEXT("ComboStateUnlocked", "DESBLOQUEADO"));
		StateText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
		break;
	case EWeaponComboState::Available:
		StateDiamondImage->SetBrush(FVelmaraUITheme::MakeImageBrush(FilledDia, FVector2D(14.f, 14.f), FVelmaraUITheme::BloodBright()));
		StateText->SetText(LOCTEXT("ComboStateAvailable", "DISPONIBLE"));
		StateText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BloodBright()));
		break;
	default:
		StateDiamondImage->SetBrush(FVelmaraUITheme::MakeImageBrush(HollowDia, FVector2D(14.f, 14.f), FVelmaraUITheme::Ash()));
		StateText->SetText(LOCTEXT("ComboStateLocked", "BLOQUEADO"));
		StateText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted(0.55f)));
		break;
	}

	if (bIsUnlocked)
	{
		CostBloodIcon->SetVisibility(ESlateVisibility::Collapsed);
		CostText->SetText(LOCTEXT("ComboOwnedShort", "ACTIVO"));
		CostText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted(0.75f)));
		ChevronImage->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		CostBloodIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		ChevronImage->SetVisibility(bIsAvailable ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		if (Upgrades->AreUpgradesFree())
		{
			CostText->SetText(LOCTEXT("ComboFreeShort", "GRATIS"));
			CostText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
		}
		else
		{
			CostText->SetText(FVelmaraUITheme::FormatGroupedNumber(Upgrade.Cost));
			CostText->SetColorAndOpacity(FSlateColor(
				(bIsAvailable && bCanAfford) ? FVelmaraUITheme::Bone() : FVelmaraUITheme::BloodBright()));
		}
	}

	RefreshVisualState();
}

void UWeaponComboRowWidget::RefreshVisualState()
{
	const bool bActive = bIsRowHovered || bIsRowFocused;

	if (FocusStarImage)
	{
		FocusStarImage->SetVisibility(bActive ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	}

	if (HoverWashImage)
	{
		HoverWashImage->SetRenderOpacity(bActive ? 1.f : 0.f);
	}

	if (FrameBorderImage)
	{
		FLinearColor FrameTint = FVelmaraUITheme::Ash();
		if (bActive)
		{
			FrameTint = FVelmaraUITheme::BloodBright();
		}
		else if (bIsUnlocked)
		{
			FrameTint = FVelmaraUITheme::Bone(0.55f);
		}
		FrameBorderImage->SetBrush(FVelmaraUITheme::MakeOptionRowBrush(FrameTint));
	}

	if (FillBorder)
	{
		FillBorder->SetBrush(FVelmaraUITheme::MakeSolidBrush(
			bActive ? FVelmaraUITheme::BloodDried(0.45f) : FVelmaraUITheme::Charcoal(0.74f)));
	}

	if (NameText)
	{
		NameText->SetColorAndOpacity(FSlateColor(
			bActive ? FVelmaraUITheme::Bone() : (bIsAvailable || bIsUnlocked ? FVelmaraUITheme::Bone(0.92f) : FVelmaraUITheme::BoneMuted(0.65f))));
	}

	if (ChevronImage)
	{
		ChevronImage->SetColorAndOpacity(bActive ? FVelmaraUITheme::BloodBright() : FVelmaraUITheme::BoneMuted());
	}
}

void UWeaponComboRowWidget::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnAddedToFocusPath(InFocusEvent);
	bIsRowFocused = true;
	RefreshVisualState();
	if (UWeaponUpgradeMenuWidget* Menu = OwnerMenu.Get())
	{
		Menu->NotifyComboSelected(Weapon.Get(), UnlockTag);
	}
}

void UWeaponComboRowWidget::NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnRemovedFromFocusPath(InFocusEvent);
	bIsRowFocused = false;
	RefreshVisualState();
}

void UWeaponComboRowWidget::HandleRowHovered()
{
	bIsRowHovered = true;
	if (RowButton)
	{
		RowButton->SetKeyboardFocus();
	}
	RefreshVisualState();
	if (UWeaponUpgradeMenuWidget* Menu = OwnerMenu.Get())
	{
		Menu->NotifyComboSelected(Weapon.Get(), UnlockTag);
	}
}

void UWeaponComboRowWidget::HandleRowUnhovered()
{
	bIsRowHovered = false;
	RefreshVisualState();
}

void UWeaponComboRowWidget::HandleUnlockClicked()
{
	UWeaponUpgradeComponent* Upgrades = WeaponUpgrades.Get();
	UWeaponData* WeaponData = Weapon.Get();
	if (!Upgrades || !WeaponData) return;

	const EWeaponUpgradeResult Result = Upgrades->TryUnlockCombo(WeaponData, UnlockTag);

	if (UWeaponUpgradeMenuWidget* Menu = OwnerMenu.Get())
	{
		Menu->ShowUnlockResult(Result, WeaponData, UnlockTag);
	}
}

#undef LOCTEXT_NAMESPACE
