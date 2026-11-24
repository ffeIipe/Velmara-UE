#include "Features/AttributeTreeSystem/UI/AttributeTreeRowWidget.h"

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
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Features/AttributeTreeSystem/Core/AttributeTreeComponent.h"
#include "Features/AttributeTreeSystem/UI/AttributeTreeMenuWidget.h"
#include "UI/Style/VelmaraUITheme.h"

#define LOCTEXT_NAMESPACE "AttributeTree"

void UAttributeTreeRowWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
}

void UAttributeTreeRowWidget::Setup(UAttributeTreeComponent* InAttributeTree, const FGameplayTag& InUpgradeTag, UAttributeTreeMenuWidget* InOwnerMenu)
{
	AttributeTree = InAttributeTree;
	UpgradeTag = InUpgradeTag;
	OwnerMenu = InOwnerMenu;

	Refresh();
}

UWidget* UAttributeTreeRowWidget::GetFocusTarget() const
{
	return RowButton;
}

void UAttributeTreeRowWidget::BuildLayout()
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
	ButtonSize->SetHeightOverride(76.f);
	if (UHorizontalBoxSlot* BtnWrapperSlot = OuterRow->AddChildToHorizontalBox(ButtonSize))
	{
		BtnWrapperSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		BtnWrapperSlot->SetVerticalAlignment(VAlign_Center);
	}

	RowButton = Make<UButton>(WidgetTree);
	RowButton->SetStyle(MakeTransparentButtonStyle());
	RowButton->OnClicked.AddDynamic(this, &UAttributeTreeRowWidget::HandleUpgradeClicked);
	RowButton->OnHovered.AddDynamic(this, &UAttributeTreeRowWidget::HandleRowHovered);
	RowButton->OnUnhovered.AddDynamic(this, &UAttributeTreeRowWidget::HandleRowUnhovered);
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
		ColSlot->SetPadding(FMargin(20.f, 0.f, 18.f, 0.f));
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
	LeftDivSize->SetHeightOverride(34.f);
	UBorder* LeftDiv = Make<UBorder>(WidgetTree);
	LeftDiv->SetBrush(FVelmaraUITheme::MakeSolidBrush(FVelmaraUITheme::Ash(0.75f)));
	LeftDivSize->SetContent(LeftDiv);
	if (UHorizontalBoxSlot* LDivSlot = Columns->AddChildToHorizontalBox(LeftDivSize))
	{
		LDivSlot->SetVerticalAlignment(VAlign_Center);
		LDivSlot->SetPadding(FMargin(0.f, 0.f, 18.f, 0.f));
	}

	NameText = MakeDisplayText(WidgetTree, FText::GetEmpty(), 17, FVelmaraUITheme::Bone(), 140);
	if (UHorizontalBoxSlot* NameSlot = Columns->AddChildToHorizontalBox(NameText))
	{
		NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		NameSlot->SetVerticalAlignment(VAlign_Center);
		NameSlot->SetPadding(FMargin(0.f, 0.f, 14.f, 0.f));
	}

	UHorizontalBox* ValuePreviewBox = Make<UHorizontalBox>(WidgetTree);
	CurrentValueText = MakeText(WidgetTree, FText::GetEmpty(), 16, FVelmaraUITheme::BoneMuted(), false);
	if (UHorizontalBoxSlot* CurSlot = ValuePreviewBox->AddChildToHorizontalBox(CurrentValueText))
	{
		CurSlot->SetVerticalAlignment(VAlign_Center);
	}

	ArrowText = MakeText(WidgetTree, FText::FromString(TEXT("  →  ")), 15, FVelmaraUITheme::BoneMuted(0.75f), false);
	if (UHorizontalBoxSlot* ArrSlot = ValuePreviewBox->AddChildToHorizontalBox(ArrowText))
	{
		ArrSlot->SetVerticalAlignment(VAlign_Center);
	}

	NextValueText = MakeText(WidgetTree, FText::GetEmpty(), 16, FVelmaraUITheme::BloodBright(), true);
	if (UHorizontalBoxSlot* NextSlot = ValuePreviewBox->AddChildToHorizontalBox(NextValueText))
	{
		NextSlot->SetVerticalAlignment(VAlign_Center);
	}

	USizeBox* ValueWidthBox = Make<USizeBox>(WidgetTree);
	ValueWidthBox->SetMinDesiredWidth(135.f);
	ValueWidthBox->SetContent(ValuePreviewBox);
	if (UHorizontalBoxSlot* ValSlot = Columns->AddChildToHorizontalBox(ValueWidthBox))
	{
		ValSlot->SetVerticalAlignment(VAlign_Center);
		ValSlot->SetPadding(FMargin(0.f, 0.f, 18.f, 0.f));
	}

	PipsBox = Make<UHorizontalBox>(WidgetTree);
	if (UHorizontalBoxSlot* PipsSlot = Columns->AddChildToHorizontalBox(PipsBox))
	{
		PipsSlot->SetVerticalAlignment(VAlign_Center);
		PipsSlot->SetPadding(FMargin(0.f, 0.f, 18.f, 0.f));
	}

	USizeBox* RightDivSize = Make<USizeBox>(WidgetTree);
	RightDivSize->SetWidthOverride(1.f);
	RightDivSize->SetHeightOverride(34.f);
	UBorder* RightDiv = Make<UBorder>(WidgetTree);
	RightDiv->SetBrush(FVelmaraUITheme::MakeSolidBrush(FVelmaraUITheme::Ash(0.75f)));
	RightDivSize->SetContent(RightDiv);
	if (UHorizontalBoxSlot* RDivSlot = Columns->AddChildToHorizontalBox(RightDivSize))
	{
		RDivSlot->SetVerticalAlignment(VAlign_Center);
		RDivSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
	}

	USizeBox* CostSection = Make<USizeBox>(WidgetTree);
	CostSection->SetMinDesiredWidth(102.f);
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

UTexture2D* UAttributeTreeRowWidget::ResolveFallbackIcon() const
{
	const FString TagString = UpgradeTag.ToString().ToLower();
	if (TagString.Contains(TEXT("health")) || TagString.Contains(TEXT("vida")))
	{
		return FVelmaraUITheme::LoadTexture(VelmaraUIPaths::IconStatBlood);
	}
	if (TagString.Contains(TEXT("armor")) || TagString.Contains(TEXT("shield")) || TagString.Contains(TEXT("escudo")))
	{
		return FVelmaraUITheme::LoadTexture(VelmaraUIPaths::IconStatShield);
	}
	if (TagString.Contains(TEXT("energy")) || TagString.Contains(TEXT("spectral")))
	{
		return FVelmaraUITheme::LoadTexture(VelmaraUIPaths::IconStatSpectral);
	}
	if (TagString.Contains(TEXT("attackspeed")) || TagString.Contains(TEXT("vel")))
	{
		return FVelmaraUITheme::LoadTexture(VelmaraUIPaths::IconStatAttackSpeed);
	}
	if (TagString.Contains(TEXT("speed")))
	{
		return FVelmaraUITheme::LoadTexture(VelmaraUIPaths::IconStatSpeed);
	}
	return FVelmaraUITheme::LoadTexture(VelmaraUIPaths::IconStatDamage);
}

void UAttributeTreeRowWidget::Refresh()
{
	UAttributeTreeComponent* Tree = AttributeTree.Get();
	FAttributeUpgradeDefinition Definition;

	if (!Tree || !NameText || !Tree->GetUpgradeDefinition(UpgradeTag, Definition)) return;

	const int32 CurrentLevel = Tree->GetUpgradeLevel(UpgradeTag);
	const int32 MaxLevel = Definition.GetMaxLevel();
	bIsMaxed = CurrentLevel >= MaxLevel;
	const bool bCanAfford = Tree->CanAffordUpgrade(UpgradeTag);
	bIsRowEnabled = !bIsMaxed && bCanAfford;

	NameText->SetText(Definition.DisplayName.ToUpper());

	UTexture2D* IconTex = Definition.Icon ? Definition.Icon.Get() : ResolveFallbackIcon();
	if (IconTex)
	{
		IconImage->SetBrush(FVelmaraUITheme::MakeImageBrush(IconTex, FVector2D(34.f, 34.f), FVelmaraUITheme::Bone()));
		IconBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else
	{
		IconBox->SetVisibility(ESlateVisibility::Collapsed);
	}

	const float CurrentVal = Tree->GetCurrentAttributeValue(UpgradeTag);
	const float CurBonus = Definition.GetTotalBonusAtLevel(CurrentLevel);
	CurrentValueText->SetText(AttributeTreeUtils::FormatValue(CurrentVal, Definition.DisplayFormat));

	if (bIsMaxed)
	{
		ArrowText->SetVisibility(ESlateVisibility::Collapsed);
		NextValueText->SetText(LOCTEXT("RowMaxSuffix", "  MÁX"));
		NextValueText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted()));
	}
	else
	{
		const float NextBonus = Definition.GetTotalBonusAtLevel(CurrentLevel + 1);
		const float Delta = NextBonus - CurBonus;
		ArrowText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		NextValueText->SetText(AttributeTreeUtils::FormatValue(CurrentVal + Delta, Definition.DisplayFormat));
		NextValueText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BloodBright()));
	}

	if (bIsMaxed)
	{
		CostBloodIcon->SetVisibility(ESlateVisibility::Collapsed);
		CostText->SetText(LOCTEXT("RowMaxLabel", "MÁX"));
		CostText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted(0.65f)));
		ChevronImage->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		CostBloodIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		ChevronImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		if (Tree->AreUpgradesFree())
		{
			CostText->SetText(LOCTEXT("RowFreeShort", "GRATIS"));
			CostText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
		}
		else
		{
			const int32 Cost = Tree->GetNextLevelCost(UpgradeTag);
			CostText->SetText(FVelmaraUITheme::FormatGroupedNumber(Cost));
			CostText->SetColorAndOpacity(FSlateColor(bCanAfford ? FVelmaraUITheme::Bone() : FVelmaraUITheme::BloodBright()));
		}
	}

	RebuildLevelPips(CurrentLevel, MaxLevel, bCanAfford);
	RefreshVisualState();
}

void UAttributeTreeRowWidget::RebuildLevelPips(const int32 CurrentLevel, const int32 MaxLevel, const bool bCanAffordNext)
{
	using namespace UpgradeUIStyle;

	if (!PipsBox) return;
	PipsBox->ClearChildren();

	UTexture2D* FilledTex = FVelmaraUITheme::LoadTexture(VelmaraUIPaths::PipDiamondFilled);
	UTexture2D* HollowTex = FVelmaraUITheme::LoadTexture(VelmaraUIPaths::PipDiamondHollow);

	for (int32 Level = 1; Level <= MaxLevel; ++Level)
	{
		const bool bOwned = Level <= CurrentLevel;
		const bool bNext = Level == CurrentLevel + 1;

		USizeBox* PipSize = Make<USizeBox>(WidgetTree);
		PipSize->SetWidthOverride(18.f);
		PipSize->SetHeightOverride(18.f);

		UImage* PipImg = Make<UImage>(WidgetTree);
		if (bOwned)
		{
			PipImg->SetBrush(FVelmaraUITheme::MakeImageBrush(FilledTex, FVector2D(18.f, 18.f), FVelmaraUITheme::BloodBright()));
		}
		else if (bNext && bCanAffordNext)
		{
			PipImg->SetBrush(FVelmaraUITheme::MakeImageBrush(HollowTex, FVector2D(18.f, 18.f), FVelmaraUITheme::Bone(0.85f)));
		}
		else
		{
			PipImg->SetBrush(FVelmaraUITheme::MakeImageBrush(HollowTex, FVector2D(18.f, 18.f), FVelmaraUITheme::Ash()));
		}

		PipSize->SetContent(PipImg);
		if (UHorizontalBoxSlot* PSlot = PipsBox->AddChildToHorizontalBox(PipSize))
		{
			PSlot->SetVerticalAlignment(VAlign_Center);
			PSlot->SetPadding(FMargin(0.f, 0.f, Level < MaxLevel ? 6.f : 0.f, 0.f));
		}
	}
}

void UAttributeTreeRowWidget::RefreshVisualState()
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
		FrameBorderImage->SetBrush(FVelmaraUITheme::MakeOptionRowBrush(
			bActive ? FVelmaraUITheme::BloodBright() : FVelmaraUITheme::Ash()));
	}

	if (FillBorder)
	{
		FillBorder->SetBrush(FVelmaraUITheme::MakeSolidBrush(
			bActive ? FVelmaraUITheme::BloodDried(0.45f) : FVelmaraUITheme::Charcoal(0.74f)));
	}

	if (NameText)
	{
		NameText->SetColorAndOpacity(FSlateColor(
			bActive ? FVelmaraUITheme::Bone() : (bIsMaxed ? FVelmaraUITheme::BoneMuted(0.75f) : FVelmaraUITheme::Bone(0.90f))));
	}

	if (ChevronImage)
	{
		ChevronImage->SetColorAndOpacity(bActive ? FVelmaraUITheme::BloodBright() : FVelmaraUITheme::BoneMuted());
	}
}

void UAttributeTreeRowWidget::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnAddedToFocusPath(InFocusEvent);
	bIsRowFocused = true;
	RefreshVisualState();
	if (UAttributeTreeMenuWidget* Menu = OwnerMenu.Get())
	{
		Menu->NotifyRowSelected(UpgradeTag);
	}
}

void UAttributeTreeRowWidget::NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnRemovedFromFocusPath(InFocusEvent);
	bIsRowFocused = false;
	RefreshVisualState();
}

void UAttributeTreeRowWidget::HandleRowHovered()
{
	bIsRowHovered = true;
	if (RowButton)
	{
		RowButton->SetKeyboardFocus();
	}
	RefreshVisualState();
	if (UAttributeTreeMenuWidget* Menu = OwnerMenu.Get())
	{
		Menu->NotifyRowSelected(UpgradeTag);
	}
}

void UAttributeTreeRowWidget::HandleRowUnhovered()
{
	bIsRowHovered = false;
	RefreshVisualState();
}

void UAttributeTreeRowWidget::HandleUpgradeClicked()
{
	UAttributeTreeComponent* Tree = AttributeTree.Get();
	if (!Tree) return;

	const EAttributeUpgradeResult Result = Tree->TryUpgrade(UpgradeTag);

	if (UAttributeTreeMenuWidget* Menu = OwnerMenu.Get())
	{
		Menu->ShowUpgradeResult(Result, UpgradeTag);
	}
}

#undef LOCTEXT_NAMESPACE
