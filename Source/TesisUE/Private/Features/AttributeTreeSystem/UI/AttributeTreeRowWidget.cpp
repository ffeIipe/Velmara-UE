#include "Features/AttributeTreeSystem/UI/AttributeTreeRowWidget.h"

#include "../../UpgradeSystem/UI/UpgradeUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Features/AttributeTreeSystem/Core/AttributeTreeComponent.h"
#include "Features/AttributeTreeSystem/UI/AttributeTreeMenuWidget.h"

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

void UAttributeTreeRowWidget::BuildLayout()
{
	using namespace UpgradeUIStyle;

	UBorder* Background = Make<UBorder>(WidgetTree);
	Background->SetBrush(MakeRoundedBrush(Colors::Row(), 10.f, Colors::RowOutline(), 1.f));
	Background->SetPadding(FMargin(18.f, 14.f));
	WidgetTree->RootWidget = Background;

	UHorizontalBox* Columns = Make<UHorizontalBox>(WidgetTree);
	Background->SetContent(Columns);

	// Ícono (opcional, se oculta si la rama no tiene uno)
	IconBox = Make<USizeBox>(WidgetTree);
	IconBox->SetWidthOverride(56.f);
	IconBox->SetHeightOverride(56.f);
	IconImage = Make<UImage>(WidgetTree);
	IconBox->SetContent(IconImage);
	IconBox->SetVisibility(ESlateVisibility::Collapsed);

	UHorizontalBoxSlot* IconSlot = Columns->AddChildToHorizontalBox(IconBox);
	IconSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
	IconSlot->SetVerticalAlignment(VAlign_Center);

	// Información de la rama
	UVerticalBox* Info = Make<UVerticalBox>(WidgetTree);
	UHorizontalBoxSlot* InfoSlot = Columns->AddChildToHorizontalBox(Info);
	InfoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	InfoSlot->SetVerticalAlignment(VAlign_Center);

	UHorizontalBox* Header = Make<UHorizontalBox>(WidgetTree);
	Info->AddChildToVerticalBox(Header);

	NameText = MakeText(WidgetTree, FText::GetEmpty(), 20, Colors::TextPrimary(), true);
	UHorizontalBoxSlot* NameSlot = Header->AddChildToHorizontalBox(NameText);
	NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	NameSlot->SetVerticalAlignment(VAlign_Center);

	LevelText = MakeText(WidgetTree, FText::GetEmpty(), 15, Colors::Gold(), true);
	Header->AddChildToHorizontalBox(LevelText)->SetVerticalAlignment(VAlign_Center);

	DescriptionText = MakeText(WidgetTree, FText::GetEmpty(), 13, Colors::TextSecondary());
	DescriptionText->SetAutoWrapText(true);
	Info->AddChildToVerticalBox(DescriptionText)->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));

	PipsBox = Make<UHorizontalBox>(WidgetTree);
	UVerticalBoxSlot* PipsSlot = Info->AddChildToVerticalBox(PipsBox);
	PipsSlot->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
	PipsSlot->SetHorizontalAlignment(HAlign_Left);

	StatsText = MakeText(WidgetTree, FText::GetEmpty(), 13, Colors::TextPrimary());
	Info->AddChildToVerticalBox(StatsText)->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));

	// Botón de mejora
	USizeBox* ButtonSize = Make<USizeBox>(WidgetTree);
	ButtonSize->SetWidthOverride(180.f);
	ButtonSize->SetHeightOverride(64.f);

	UpgradeButton = Make<UButton>(WidgetTree);
	UpgradeButton->SetStyle(MakeButtonStyle(Colors::ButtonNormal(), Colors::ButtonHovered(), Colors::ButtonPressed()));
	UpgradeButton->OnClicked.AddDynamic(this, &UAttributeTreeRowWidget::HandleUpgradeClicked);
	ButtonSize->SetContent(UpgradeButton);

	UVerticalBox* ButtonContent = Make<UVerticalBox>(WidgetTree);
	ButtonLabelText = MakeText(WidgetTree, FText::GetEmpty(), 15, Colors::TextOnGold(), true);
	ButtonLabelText->SetJustification(ETextJustify::Center);
	ButtonContent->AddChildToVerticalBox(ButtonLabelText)->SetHorizontalAlignment(HAlign_Center);

	ButtonCostText = MakeText(WidgetTree, FText::GetEmpty(), 12, Colors::TextOnGold());
	ButtonCostText->SetJustification(ETextJustify::Center);
	ButtonContent->AddChildToVerticalBox(ButtonCostText)->SetHorizontalAlignment(HAlign_Center);

	if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(UpgradeButton->SetContent(ButtonContent)))
	{
		ContentSlot->SetHorizontalAlignment(HAlign_Center);
		ContentSlot->SetVerticalAlignment(VAlign_Center);
	}

	UHorizontalBoxSlot* ButtonSlot = Columns->AddChildToHorizontalBox(ButtonSize);
	ButtonSlot->SetPadding(FMargin(18.f, 0.f, 0.f, 0.f));
	ButtonSlot->SetVerticalAlignment(VAlign_Center);
}

void UAttributeTreeRowWidget::Refresh()
{
	using namespace UpgradeUIStyle;

	UAttributeTreeComponent* Tree = AttributeTree.Get();
	FAttributeUpgradeDefinition Definition;

	if (!Tree || !NameText || !Tree->GetUpgradeDefinition(UpgradeTag, Definition)) return;

	const int32 CurrentLevel = Tree->GetUpgradeLevel(UpgradeTag);
	const int32 MaxLevel = Definition.GetMaxLevel();
	const bool bIsMaxLevel = CurrentLevel >= MaxLevel;
	const bool bCanAfford = Tree->CanAffordUpgrade(UpgradeTag);

	NameText->SetText(Definition.DisplayName);
	LevelText->SetText(FText::Format(LOCTEXT("RowLevel", "Nivel {0} / {1}"),
		FText::FromString(AttributeTreeUtils::ToRoman(CurrentLevel)),
		FText::FromString(AttributeTreeUtils::ToRoman(MaxLevel))));
	DescriptionText->SetText(Definition.Description);

	if (Definition.Icon)
	{
		IconImage->SetBrushFromTexture(Definition.Icon);
		IconBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else
	{
		IconBox->SetVisibility(ESlateVisibility::Collapsed);
	}

	const FText CurrentValue = AttributeTreeUtils::FormatValue(Tree->GetCurrentAttributeValue(UpgradeTag), Definition.DisplayFormat);
	const FText CurrentBonus = AttributeTreeUtils::FormatBonus(Definition.GetTotalBonusAtLevel(CurrentLevel), Definition.DisplayFormat);

	if (bIsMaxLevel)
	{
		StatsText->SetText(FText::Format(LOCTEXT("RowStatsMax", "Valor: {0}   ·   Bonus: {1}  (máximo)"), CurrentValue, CurrentBonus));
	}
	else
	{
		const FText NextBonus = AttributeTreeUtils::FormatBonus(Definition.GetTotalBonusAtLevel(CurrentLevel + 1), Definition.DisplayFormat);
		StatsText->SetText(FText::Format(LOCTEXT("RowStats", "Valor: {0}   ·   Bonus: {1}  »  {2}"), CurrentValue, CurrentBonus, NextBonus));
	}

	// Botón
	const bool bButtonEnabled = !bIsMaxLevel && bCanAfford;
	UpgradeButton->SetIsEnabled(bButtonEnabled);

	const FLinearColor ButtonTextColor = bButtonEnabled ? Colors::TextOnGold() : Colors::LockedText();
	ButtonLabelText->SetColorAndOpacity(FSlateColor(ButtonTextColor));
	ButtonCostText->SetColorAndOpacity(FSlateColor(ButtonTextColor));

	if (bIsMaxLevel)
	{
		ButtonLabelText->SetText(LOCTEXT("RowMaxed", "MÁXIMO"));
		ButtonCostText->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		ButtonLabelText->SetText(FText::Format(LOCTEXT("RowUpgrade", "Mejorar a {0}"),
			FText::FromString(AttributeTreeUtils::ToRoman(CurrentLevel + 1))));

		ButtonCostText->SetText(Tree->AreUpgradesFree()
			? LOCTEXT("RowFree", "Gratis (modo prueba)")
			: FText::Format(LOCTEXT("RowCost", "{0} {1}"), FText::AsNumber(Tree->GetNextLevelCost(UpgradeTag)), Tree->GetCurrencyDisplayName()));
		ButtonCostText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	RebuildLevelPips(CurrentLevel, MaxLevel, bCanAfford);
}

void UAttributeTreeRowWidget::RebuildLevelPips(const int32 CurrentLevel, const int32 MaxLevel, const bool bCanAffordNext)
{
	using namespace UpgradeUIStyle;

	PipsBox->ClearChildren();

	for (int32 Level = 1; Level <= MaxLevel; ++Level)
	{
		const bool bOwned = Level <= CurrentLevel;
		const bool bNext = Level == CurrentLevel + 1;

		// Conector entre niveles: dorado si ya se alcanzó el nivel de la derecha.
		if (Level > 1)
		{
			USizeBox* ConnectorSize = Make<USizeBox>(WidgetTree);
			ConnectorSize->SetWidthOverride(24.f);
			ConnectorSize->SetHeightOverride(3.f);

			UBorder* Connector = Make<UBorder>(WidgetTree);
			Connector->SetBrush(MakeRoundedBrush(bOwned ? Colors::Gold() : Colors::Locked(), 1.5f));
			ConnectorSize->SetContent(Connector);

			PipsBox->AddChildToHorizontalBox(ConnectorSize)->SetVerticalAlignment(VAlign_Center);
		}

		USizeBox* PipSize = Make<USizeBox>(WidgetTree);
		PipSize->SetWidthOverride(34.f);
		PipSize->SetHeightOverride(34.f);

		UBorder* Pip = Make<UBorder>(WidgetTree);
		Pip->SetPadding(FMargin(0.f));
		Pip->SetHorizontalAlignment(HAlign_Center);
		Pip->SetVerticalAlignment(VAlign_Center);

		FLinearColor TextColor;
		if (bOwned)
		{
			Pip->SetBrush(MakeRoundedBrush(Colors::Gold(), 17.f));
			TextColor = Colors::TextOnGold();
		}
		else if (bNext)
		{
			const FLinearColor Accent = bCanAffordNext ? Colors::Gold() : Colors::GoldDim();
			Pip->SetBrush(MakeRoundedBrush(Colors::Row(), 17.f, Accent, 2.f));
			TextColor = Accent;
		}
		else
		{
			Pip->SetBrush(MakeRoundedBrush(Colors::Locked(), 17.f, Colors::RowOutline(), 1.f));
			TextColor = Colors::LockedText();
		}

		Pip->SetContent(MakeText(WidgetTree, FText::FromString(AttributeTreeUtils::ToRoman(Level)), 11, TextColor, true));
		PipSize->SetContent(Pip);

		PipsBox->AddChildToHorizontalBox(PipSize)->SetVerticalAlignment(VAlign_Center);
	}
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
