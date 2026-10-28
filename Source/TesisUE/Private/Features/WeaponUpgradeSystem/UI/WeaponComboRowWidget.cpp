#include "Features/WeaponUpgradeSystem/UI/WeaponComboRowWidget.h"

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
#include "DataAssets/Items/Weapons/WeaponData.h"
#include "Engine/Texture2D.h"
#include "Features/WeaponUpgradeSystem/Core/WeaponUpgradeComponent.h"
#include "Features/WeaponUpgradeSystem/UI/WeaponUpgradeMenuWidget.h"

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

void UWeaponComboRowWidget::BuildLayout()
{
	using namespace UpgradeUIStyle;

	Background = Make<UBorder>(WidgetTree);
	Background->SetPadding(FMargin(18.f, 14.f));
	WidgetTree->RootWidget = Background;

	UHorizontalBox* Columns = Make<UHorizontalBox>(WidgetTree);
	Background->SetContent(Columns);

	// Ícono (opcional)
	IconBox = Make<USizeBox>(WidgetTree);
	IconBox->SetWidthOverride(56.f);
	IconBox->SetHeightOverride(56.f);
	IconImage = Make<UImage>(WidgetTree);
	IconBox->SetContent(IconImage);
	IconBox->SetVisibility(ESlateVisibility::Collapsed);

	UHorizontalBoxSlot* IconSlot = Columns->AddChildToHorizontalBox(IconBox);
	IconSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
	IconSlot->SetVerticalAlignment(VAlign_Center);

	// Información del combo
	UVerticalBox* Info = Make<UVerticalBox>(WidgetTree);
	UHorizontalBoxSlot* InfoSlot = Columns->AddChildToHorizontalBox(Info);
	InfoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	InfoSlot->SetVerticalAlignment(VAlign_Center);

	UHorizontalBox* Header = Make<UHorizontalBox>(WidgetTree);
	Info->AddChildToVerticalBox(Header);

	NameText = MakeText(WidgetTree, FText::GetEmpty(), 19, Colors::TextPrimary(), true);
	UHorizontalBoxSlot* NameSlot = Header->AddChildToHorizontalBox(NameText);
	NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	NameSlot->SetVerticalAlignment(VAlign_Center);

	StateText = MakeText(WidgetTree, FText::GetEmpty(), 12, Colors::TextSecondary(), true);
	Header->AddChildToHorizontalBox(StateText)->SetVerticalAlignment(VAlign_Center);

	InputText = MakeText(WidgetTree, FText::GetEmpty(), 13, Colors::Gold(), true);
	Info->AddChildToVerticalBox(InputText)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));

	DescriptionText = MakeText(WidgetTree, FText::GetEmpty(), 13, Colors::TextSecondary());
	DescriptionText->SetAutoWrapText(true);
	Info->AddChildToVerticalBox(DescriptionText)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));

	RequirementsText = MakeText(WidgetTree, FText::GetEmpty(), 12, Colors::Error(), true);
	RequirementsText->SetAutoWrapText(true);
	Info->AddChildToVerticalBox(RequirementsText)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));

	// Botón de desbloqueo
	USizeBox* ButtonSize = Make<USizeBox>(WidgetTree);
	ButtonSize->SetWidthOverride(190.f);
	ButtonSize->SetHeightOverride(64.f);

	UnlockButton = Make<UButton>(WidgetTree);
	UnlockButton->SetStyle(MakeButtonStyle(Colors::Blood(), Colors::BloodHovered(), Colors::BloodPressed()));
	UnlockButton->OnClicked.AddDynamic(this, &UWeaponComboRowWidget::HandleUnlockClicked);
	ButtonSize->SetContent(UnlockButton);

	UVerticalBox* ButtonContent = Make<UVerticalBox>(WidgetTree);
	ButtonLabelText = MakeText(WidgetTree, FText::GetEmpty(), 15, Colors::TextOnBlood(), true);
	ButtonLabelText->SetJustification(ETextJustify::Center);
	ButtonContent->AddChildToVerticalBox(ButtonLabelText)->SetHorizontalAlignment(HAlign_Center);

	ButtonCostText = MakeText(WidgetTree, FText::GetEmpty(), 12, Colors::TextOnBlood());
	ButtonCostText->SetJustification(ETextJustify::Center);
	ButtonContent->AddChildToVerticalBox(ButtonCostText)->SetHorizontalAlignment(HAlign_Center);

	if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(UnlockButton->SetContent(ButtonContent)))
	{
		ContentSlot->SetHorizontalAlignment(HAlign_Center);
		ContentSlot->SetVerticalAlignment(VAlign_Center);
	}

	UHorizontalBoxSlot* ButtonSlot = Columns->AddChildToHorizontalBox(ButtonSize);
	ButtonSlot->SetPadding(FMargin(18.f, 0.f, 0.f, 0.f));
	ButtonSlot->SetVerticalAlignment(VAlign_Center);
}

void UWeaponComboRowWidget::Refresh()
{
	using namespace UpgradeUIStyle;

	UWeaponUpgradeComponent* Upgrades = WeaponUpgrades.Get();
	const UWeaponData* WeaponData = Weapon.Get();
	FWeaponComboUpgrade Upgrade;

	if (!Upgrades || !WeaponData || !NameText || !Upgrades->GetComboUpgrade(WeaponData, UnlockTag, Upgrade)) return;

	const EWeaponComboState State = Upgrades->GetComboState(WeaponData, UnlockTag);
	const bool bCanAfford = Upgrades->CanAffordCombo(WeaponData, UnlockTag);

	NameText->SetText(Upgrade.DisplayName);
	DescriptionText->SetText(Upgrade.Description);
	DescriptionText->SetVisibility(Upgrade.Description.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);

	if (Upgrade.Icon)
	{
		IconImage->SetBrushFromTexture(Upgrade.Icon);
		IconBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else
	{
		IconBox->SetVisibility(ESlateVisibility::Collapsed);
	}

	// Cómo se ejecuta: "Suelo: Pulsar · Pulsar · Mantener   ·   4 golpes"
	const FComboInputPath Path = WeaponUpgradeUtils::FindComboPath(WeaponData, UnlockTag);
	const FText InputDescription = WeaponUpgradeUtils::DescribeComboInput(Path);
	InputText->SetText(Path.bFound && Path.HitCount > 0
		? FText::Format(LOCTEXT("ComboInputWithHits", "{0}   ·   {1} {1}|plural(one=golpe,other=golpes)"), InputDescription, Path.HitCount)
		: InputDescription);
	InputText->SetColorAndOpacity(FSlateColor(Path.bFound ? Colors::Gold() : Colors::Error()));

	// Requisitos faltantes, con el nombre del combo si es de esta misma arma.
	const FGameplayTagContainer MissingRequirements = Upgrades->GetMissingRequirements(WeaponData, UnlockTag);
	if (MissingRequirements.IsEmpty())
	{
		RequirementsText->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		TArray<FText> RequirementNames;
		for (const FGameplayTag& RequiredTag : MissingRequirements)
		{
			FWeaponComboUpgrade RequiredUpgrade;
			RequirementNames.Add(Upgrades->GetComboUpgrade(WeaponData, RequiredTag, RequiredUpgrade)
				? RequiredUpgrade.DisplayName
				: FText::FromName(RequiredTag.GetTagName()));
		}

		RequirementsText->SetText(FText::Format(LOCTEXT("ComboRequires", "Requiere: {0}"), FText::Join(FText::FromString(TEXT(", ")), RequirementNames)));
		RequirementsText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	// Estado visual
	switch (State)
	{
	case EWeaponComboState::Unlocked:
		Background->SetBrush(MakeRoundedBrush(Colors::BloodRow(), 10.f, Colors::GoldDim(), 1.f));
		StateText->SetText(LOCTEXT("ComboStateUnlocked", "DESBLOQUEADO"));
		StateText->SetColorAndOpacity(FSlateColor(Colors::Gold()));
		break;
	case EWeaponComboState::Available:
		Background->SetBrush(MakeRoundedBrush(Colors::BloodRow(), 10.f, Colors::BloodDim(), 1.f));
		StateText->SetText(LOCTEXT("ComboStateAvailable", "DISPONIBLE"));
		StateText->SetColorAndOpacity(FSlateColor(Colors::Blood()));
		break;
	default:
		Background->SetBrush(MakeRoundedBrush(Colors::Row(), 10.f, Colors::RowOutline(), 1.f));
		StateText->SetText(LOCTEXT("ComboStateLocked", "BLOQUEADO"));
		StateText->SetColorAndOpacity(FSlateColor(Colors::LockedText()));
		break;
	}

	// Botón
	const bool bButtonEnabled = State == EWeaponComboState::Available && bCanAfford;
	UnlockButton->SetIsEnabled(bButtonEnabled);

	const FLinearColor ButtonTextColor = bButtonEnabled ? Colors::TextOnBlood() : Colors::LockedText();
	ButtonLabelText->SetColorAndOpacity(FSlateColor(ButtonTextColor));
	ButtonCostText->SetColorAndOpacity(FSlateColor(ButtonTextColor));

	if (State == EWeaponComboState::Unlocked)
	{
		ButtonLabelText->SetText(LOCTEXT("ComboButtonUnlocked", "Desbloqueado"));
		ButtonCostText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	ButtonLabelText->SetText(State == EWeaponComboState::Available
		? LOCTEXT("ComboButtonUnlock", "Desbloquear")
		: LOCTEXT("ComboButtonLocked", "Bloqueado"));

	ButtonCostText->SetText(Upgrades->AreUpgradesFree()
		? LOCTEXT("ComboButtonFree", "Gratis (modo prueba)")
		: FText::Format(LOCTEXT("ComboButtonCost", "{0} {1}"), FText::AsNumber(Upgrade.Cost), Upgrades->GetCurrencyDisplayName()));
	ButtonCostText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
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
