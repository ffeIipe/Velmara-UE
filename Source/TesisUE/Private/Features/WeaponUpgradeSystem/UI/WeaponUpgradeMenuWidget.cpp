#include "Features/WeaponUpgradeSystem/UI/WeaponUpgradeMenuWidget.h"

#include "../../UpgradeSystem/UI/UpgradeUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "DataAssets/Items/Weapons/WeaponData.h"
#include "Features/WeaponUpgradeSystem/Core/WeaponUpgradeComponent.h"
#include "Features/WeaponUpgradeSystem/UI/WeaponComboRowWidget.h"
#include "Features/WeaponUpgradeSystem/UI/WeaponUpgradeTabWidget.h"

#define LOCTEXT_NAMESPACE "WeaponUpgrade"

void UWeaponUpgradeMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!RowWidgetClass)
	{
		RowWidgetClass = UWeaponComboRowWidget::StaticClass();
	}
	if (!TabWidgetClass)
	{
		TabWidgetClass = UWeaponUpgradeTabWidget::StaticClass();
	}

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
}

UWeaponUpgradeComponent* UWeaponUpgradeMenuWidget::GetWeaponUpgrades() const
{
	return Cast<UWeaponUpgradeComponent>(GetUpgradeComponent());
}

void UWeaponUpgradeMenuWidget::BuildLayout()
{
	using namespace UpgradeUIStyle;

	// Fondo oscuro a pantalla completa con el panel centrado.
	UBorder* Dimmer = Make<UBorder>(WidgetTree);
	Dimmer->SetBrush(MakeRoundedBrush(Colors::Overlay(), 0.f));
	Dimmer->SetPadding(FMargin(24.f));
	Dimmer->SetHorizontalAlignment(HAlign_Center);
	Dimmer->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Dimmer;

	USizeBox* PanelSize = Make<USizeBox>(WidgetTree);
	PanelSize->SetWidthOverride(900.f);
	Dimmer->SetContent(PanelSize);

	UBorder* Panel = Make<UBorder>(WidgetTree);
	Panel->SetBrush(MakeRoundedBrush(Colors::BloodPanel(), 14.f, Colors::BloodDim(), 1.5f));
	Panel->SetPadding(FMargin(32.f, 28.f));
	PanelSize->SetContent(Panel);

	UVerticalBox* Main = Make<UVerticalBox>(WidgetTree);
	Panel->SetContent(Main);

	// Encabezado
	Main->AddChildToVerticalBox(MakeText(WidgetTree, LOCTEXT("AltarTitle", "ALTAR DE SANGRE"), 30, Colors::Blood(), true));

	Main->AddChildToVerticalBox(MakeText(WidgetTree, LOCTEXT("AltarSubtitle", "Ofrecé Sangre para despertar nuevos combos en tus armas."), 14, Colors::TextSecondary()))
	    ->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));

	// Moneda
	UBorder* CurrencyBar = Make<UBorder>(WidgetTree);
	CurrencyBar->SetBrush(MakeRoundedBrush(Colors::BloodRow(), 8.f, Colors::BloodDim(), 1.f));
	CurrencyBar->SetPadding(FMargin(16.f, 10.f));
	CurrencyText = MakeText(WidgetTree, FText::GetEmpty(), 17, Colors::TextPrimary(), true);
	CurrencyBar->SetContent(CurrencyText);
	Main->AddChildToVerticalBox(CurrencyBar)->SetPadding(FMargin(0.f, 18.f, 0.f, 16.f));

	// Pestañas de armas
	TabsContainer = Make<UHorizontalBox>(WidgetTree);
	Main->AddChildToVerticalBox(TabsContainer)->SetPadding(FMargin(0.f, 0.f, 0.f, 18.f));

	// Arma seleccionada
	UHorizontalBox* WeaponHeader = Make<UHorizontalBox>(WidgetTree);
	Main->AddChildToVerticalBox(WeaponHeader)->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));

	WeaponNameText = MakeText(WidgetTree, FText::GetEmpty(), 22, Colors::TextPrimary(), true);
	UHorizontalBoxSlot* WeaponNameSlot = WeaponHeader->AddChildToHorizontalBox(WeaponNameText);
	WeaponNameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	WeaponNameSlot->SetVerticalAlignment(VAlign_Center);

	WeaponProgressText = MakeText(WidgetTree, FText::GetEmpty(), 14, Colors::Gold(), true);
	WeaponHeader->AddChildToHorizontalBox(WeaponProgressText)->SetVerticalAlignment(VAlign_Center);

	EmptyText = MakeText(WidgetTree, FText::GetEmpty(), 14, Colors::TextSecondary());
	EmptyText->SetAutoWrapText(true);
	EmptyText->SetVisibility(ESlateVisibility::Collapsed);
	Main->AddChildToVerticalBox(EmptyText)->SetPadding(FMargin(0.f, 4.f, 0.f, 12.f));

	// Combos del arma (con scroll si son muchos)
	USizeBox* RowsSize = Make<USizeBox>(WidgetTree);
	RowsSize->SetMaxDesiredHeight(470.f);
	Main->AddChildToVerticalBox(RowsSize);

	UScrollBox* RowsScroll = Make<UScrollBox>(WidgetTree);
	RowsSize->SetContent(RowsScroll);

	RowsContainer = Make<UVerticalBox>(WidgetTree);
	RowsScroll->AddChild(RowsContainer);

	// Mensaje de resultado del último desbloqueo
	FeedbackText = MakeText(WidgetTree, FText::GetEmpty(), 14, Colors::TextSecondary(), true);
	FeedbackText->SetJustification(ETextJustify::Center);
	UVerticalBoxSlot* FeedbackSlot = Main->AddChildToVerticalBox(FeedbackText);
	FeedbackSlot->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
	FeedbackSlot->SetHorizontalAlignment(HAlign_Center);

	// Pie: ayuda + botón cerrar
	UHorizontalBox* Footer = Make<UHorizontalBox>(WidgetTree);
	Main->AddChildToVerticalBox(Footer)->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));

	UHorizontalBoxSlot* HintSlot = Footer->AddChildToHorizontalBox(MakeText(WidgetTree, LOCTEXT("AltarHint", "Esc para cerrar"), 13, Colors::TextSecondary()));
	HintSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	HintSlot->SetVerticalAlignment(VAlign_Center);

	USizeBox* CloseSize = Make<USizeBox>(WidgetTree);
	CloseSize->SetWidthOverride(140.f);
	CloseSize->SetHeightOverride(42.f);

	UButton* CloseButton = Make<UButton>(WidgetTree);
	CloseButton->SetStyle(MakeButtonStyle(Colors::BloodRow(), Colors::RowHovered(), Colors::Locked(), Colors::BloodDim(), 1.f));
	CloseButton->OnClicked.AddDynamic(this, &UWeaponUpgradeMenuWidget::HandleCloseClicked);
	CloseSize->SetContent(CloseButton);

	if (UButtonSlot* CloseContentSlot = Cast<UButtonSlot>(CloseButton->SetContent(MakeText(WidgetTree, LOCTEXT("AltarClose", "Cerrar"), 15, Colors::TextPrimary(), true))))
	{
		CloseContentSlot->SetHorizontalAlignment(HAlign_Center);
		CloseContentSlot->SetVerticalAlignment(VAlign_Center);
	}

	Footer->AddChildToHorizontalBox(CloseSize)->SetVerticalAlignment(VAlign_Center);
}

void UWeaponUpgradeMenuWidget::OnUpgradeComponentSet()
{
	if (FeedbackText)
	{
		FeedbackText->SetText(FText::GetEmpty());
	}

	// El inventario pudo cambiar desde la última vez: forzamos a reconstruir pestañas y filas.
	Weapons.Reset();
	RowsWeapon = nullptr;

	RefreshMenu();
}

void UWeaponUpgradeMenuWidget::BindToUpgradeComponent()
{
	Super::BindToUpgradeComponent();

	if (UWeaponUpgradeComponent* Upgrades = GetWeaponUpgrades())
	{
		Upgrades->OnCombosChanged.AddUniqueDynamic(this, &UWeaponUpgradeMenuWidget::HandleCombosChanged);
	}
}

void UWeaponUpgradeMenuWidget::UnbindFromUpgradeComponent()
{
	if (UWeaponUpgradeComponent* Upgrades = GetWeaponUpgrades())
	{
		Upgrades->OnCombosChanged.RemoveDynamic(this, &UWeaponUpgradeMenuWidget::HandleCombosChanged);
	}

	Super::UnbindFromUpgradeComponent();
}

void UWeaponUpgradeMenuWidget::RefreshMenu()
{
	UWeaponUpgradeComponent* Upgrades = GetWeaponUpgrades();
	if (!Upgrades || !CurrencyText) return;

	// Moneda
	if (Upgrades->HasCurrencyProvider())
	{
		CurrencyText->SetText(FText::Format(LOCTEXT("AltarCurrency", "{0}: {1}"), Upgrades->GetCurrencyDisplayName(), FText::AsNumber(Upgrades->GetCurrencyAmount())));
	}
	else if (Upgrades->AreUpgradesFree())
	{
		CurrencyText->SetText(FText::Format(LOCTEXT("AltarCurrencyFree", "Sin proveedor de {0}: modo prueba, los desbloqueos son gratis"), Upgrades->GetCurrencyDisplayName()));
	}
	else
	{
		CurrencyText->SetText(FText::Format(LOCTEXT("AltarCurrencyNone", "Sin proveedor de {0} disponible"), Upgrades->GetCurrencyDisplayName()));
	}

	// Pestañas: solo se reconstruyen si cambió la lista de armas (nunca mientras se procesa el click de una pestaña).
	const TArray<UWeaponData*> CurrentWeapons = Upgrades->GetUpgradeableWeapons();
	bool bWeaponsChanged = CurrentWeapons.Num() != Weapons.Num();
	for (int32 Index = 0; !bWeaponsChanged && Index < CurrentWeapons.Num(); ++Index)
	{
		bWeaponsChanged = CurrentWeapons[Index] != Weapons[Index];
	}

	if (bWeaponsChanged)
	{
		Weapons.Reset();
		Weapons.Append(CurrentWeapons);
		RebuildTabs();
	}

	if (!SelectedWeapon || !Weapons.Contains(SelectedWeapon))
	{
		SelectedWeapon = Weapons.IsEmpty() ? nullptr : Weapons[0].Get();
	}

	for (int32 Index = 0; Index < Tabs.Num(); ++Index)
	{
		UWeaponData* TabWeapon = Weapons.IsValidIndex(Index) ? Weapons[Index].Get() : nullptr;
		if (!Tabs[Index] || !TabWeapon) continue;

		Tabs[Index]->SetSelected(TabWeapon == SelectedWeapon);
		Tabs[Index]->SetProgressText(FText::Format(LOCTEXT("TabProgress", "{0} / {1} combos"),
			Upgrades->GetUnlockedComboCount(TabWeapon), Upgrades->GetComboUpgrades(TabWeapon).Num()));
	}

	// Arma seleccionada
	UWeaponData* Weapon = GetSelectedWeapon();
	const int32 ComboCount = Upgrades->GetComboUpgrades(Weapon).Num();

	WeaponNameText->SetText(WeaponUpgradeUtils::GetWeaponDisplayName(Weapon));
	WeaponProgressText->SetText(Weapon && ComboCount > 0
		? FText::Format(LOCTEXT("WeaponProgress", "{0} / {1} combos desbloqueados"), Upgrades->GetUnlockedComboCount(Weapon), ComboCount)
		: FText::GetEmpty());

	if (!Weapon)
	{
		EmptyText->SetText(LOCTEXT("AltarNoWeapons", "No tenés armas para mejorar. Equipá una espada o agregala en \"Upgradeable Weapons\" del componente Weapon Upgrade."));
		EmptyText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else if (ComboCount == 0)
	{
		EmptyText->SetText(LOCTEXT("AltarNoCombos", "Esta arma no tiene combos desbloqueables: asigná un RequiredUnlockTag a los nodos que abren cada rama de su árbol de combos, o completá ComboUpgrades en su Data Asset."));
		EmptyText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else
	{
		EmptyText->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (RowsWeapon != Weapon || Rows.Num() != ComboCount)
	{
		RebuildRows();
		return;
	}

	for (UWeaponComboRowWidget* Row : Rows)
	{
		if (Row)
		{
			Row->Refresh();
		}
	}
}

void UWeaponUpgradeMenuWidget::RebuildTabs()
{
	if (!TabsContainer) return;

	TabsContainer->ClearChildren();
	Tabs.Reset();

	TSubclassOf<UWeaponUpgradeTabWidget> TabClass = TabWidgetClass;
	if (!TabClass)
	{
		TabClass = UWeaponUpgradeTabWidget::StaticClass();
	}

	for (int32 Index = 0; Index < Weapons.Num(); ++Index)
	{
		UWeaponUpgradeTabWidget* Tab = CreateWidget<UWeaponUpgradeTabWidget>(this, TabClass);
		if (!Tab) continue;

		Tab->Setup(this, Index, WeaponUpgradeUtils::GetWeaponDisplayName(Weapons[Index]));
		TabsContainer->AddChildToHorizontalBox(Tab)->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
		Tabs.Add(Tab);
	}
}

void UWeaponUpgradeMenuWidget::RebuildRows()
{
	if (!RowsContainer) return;

	RowsContainer->ClearChildren();
	Rows.Reset();

	UWeaponUpgradeComponent* Upgrades = GetWeaponUpgrades();
	UWeaponData* Weapon = GetSelectedWeapon();
	RowsWeapon = Weapon;

	if (!Upgrades || !Weapon) return;

	TSubclassOf<UWeaponComboRowWidget> RowClass = RowWidgetClass;
	if (!RowClass)
	{
		RowClass = UWeaponComboRowWidget::StaticClass();
	}

	for (const FWeaponComboUpgrade& Upgrade : Upgrades->GetComboUpgrades(Weapon))
	{
		UWeaponComboRowWidget* Row = CreateWidget<UWeaponComboRowWidget>(this, RowClass);
		if (!Row) continue;

		Row->Setup(Upgrades, Weapon, Upgrade.UnlockTag, this);
		RowsContainer->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
		Rows.Add(Row);
	}
}

UWeaponData* UWeaponUpgradeMenuWidget::GetSelectedWeapon() const
{
	return SelectedWeapon;
}

void UWeaponUpgradeMenuWidget::SelectWeapon(const int32 WeaponIndex)
{
	if (!Weapons.IsValidIndex(WeaponIndex) || Weapons[WeaponIndex] == SelectedWeapon) return;

	SelectedWeapon = Weapons[WeaponIndex];

	if (FeedbackText)
	{
		FeedbackText->SetText(FText::GetEmpty());
	}

	RefreshMenu();
	SetFocus();
}

void UWeaponUpgradeMenuWidget::ShowUnlockResult(const EWeaponUpgradeResult Result, const UWeaponData* Weapon, const FGameplayTag& UnlockTag)
{
	using namespace UpgradeUIStyle;

	if (FeedbackText)
	{
		const UWeaponUpgradeComponent* Upgrades = GetWeaponUpgrades();
		const FText CurrencyName = Upgrades ? Upgrades->GetCurrencyDisplayName() : FText::GetEmpty();

		FWeaponComboUpgrade Upgrade;
		if (Upgrades)
		{
			Upgrades->GetComboUpgrade(Weapon, UnlockTag, Upgrade);
		}

		FText Message;
		bool bIsError = true;

		switch (Result)
		{
		case EWeaponUpgradeResult::Success:
			Message = FText::Format(LOCTEXT("AltarResultSuccess", "Combo desbloqueado: {0}"), Upgrade.DisplayName);
			bIsError = false;
			break;
		case EWeaponUpgradeResult::AlreadyUnlocked:
			Message = LOCTEXT("AltarResultAlready", "Ese combo ya está desbloqueado.");
			break;
		case EWeaponUpgradeResult::MissingRequirements:
			Message = LOCTEXT("AltarResultRequirements", "Primero tenés que desbloquear los combos previos.");
			break;
		case EWeaponUpgradeResult::NotEnoughCurrency:
			Message = FText::Format(LOCTEXT("AltarResultNoCurrency", "No tenés suficiente {0} para este combo."), CurrencyName);
			break;
		case EWeaponUpgradeResult::NoCurrencySystem:
			Message = FText::Format(LOCTEXT("AltarResultNoCurrencySystem", "No hay un proveedor de {0} disponible."), CurrencyName);
			break;
		default:
			Message = LOCTEXT("AltarResultInvalid", "El combo no está configurado correctamente.");
			break;
		}

		FeedbackText->SetText(Message);
		FeedbackText->SetColorAndOpacity(FSlateColor(bIsError ? Colors::Error() : Colors::Success()));
	}

	RefreshMenu();

	// Si el botón clickeado quedó deshabilitado, el foco vuelve al menú para que Esc siga funcionando.
	SetFocus();
}

void UWeaponUpgradeMenuWidget::HandleCloseClicked()
{
	RequestClose();
}

void UWeaponUpgradeMenuWidget::HandleCombosChanged()
{
	RefreshMenu();
}

#undef LOCTEXT_NAMESPACE
