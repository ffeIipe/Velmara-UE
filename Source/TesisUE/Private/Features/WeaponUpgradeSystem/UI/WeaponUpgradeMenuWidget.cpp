#include "Features/WeaponUpgradeSystem/UI/WeaponUpgradeMenuWidget.h"

#include "../../UpgradeSystem/UI/UpgradeUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "DataAssets/Items/Weapons/WeaponData.h"
#include "Features/WeaponUpgradeSystem/Core/WeaponUpgradeComponent.h"
#include "Features/WeaponUpgradeSystem/UI/WeaponComboRowWidget.h"
#include "Features/WeaponUpgradeSystem/UI/WeaponUpgradeTabWidget.h"
#include "UI/Style/VelmaraUITheme.h"

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

UWidget* UWeaponUpgradeMenuWidget::GetFirstFocusableWidget() const
{
	for (const UWeaponComboRowWidget* Row : Rows)
	{
		if (Row && Row->GetFocusTarget())
		{
			return Row->GetFocusTarget();
		}
	}
	return nullptr;
}

void UWeaponUpgradeMenuWidget::BuildLayout()
{
	using namespace UpgradeUIStyle;

	RootDimmer = Make<UBorder>(WidgetTree);
	RootDimmer->SetBrush(MakeSharpBrush(Colors::Overlay()));
	RootDimmer->SetPadding(FMargin(24.f));
	RootDimmer->SetHorizontalAlignment(HAlign_Center);
	RootDimmer->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = RootDimmer;

	PanelSizeBox = Make<USizeBox>(WidgetTree);
	PanelSizeBox->SetWidthOverride(960.f);
	RootDimmer->SetContent(PanelSizeBox);

	PanelBorder = Make<UBorder>(WidgetTree);
	PanelBorder->SetBrush(MakeSharpBrush(Colors::BloodPanel(), Colors::PanelOutline(), 1.f));
	PanelBorder->SetPadding(FMargin(32.f, 28.f));
	PanelSizeBox->SetContent(PanelBorder);

	UVerticalBox* Main = Make<UVerticalBox>(WidgetTree);
	PanelBorder->SetContent(Main);

	StandaloneHeaderBox = Make<UVerticalBox>(WidgetTree);
	Main->AddChildToVerticalBox(StandaloneHeaderBox);

	StandaloneHeaderBox->AddChildToVerticalBox(MakeDisplayText(WidgetTree, LOCTEXT("AltarTitle", "MEJORAS DE ARMAS"), 30, FVelmaraUITheme::Bone(), 200));

	USizeBox* HeaderSepSize = Make<USizeBox>(WidgetTree);
	HeaderSepSize->SetHeightOverride(18.f);
	UImage* HeaderSep = Make<UImage>(WidgetTree);
	HeaderSep->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnSepStar),
		FVector2D(420.f, 18.f),
		FVelmaraUITheme::Bone(0.8f)));
	HeaderSepSize->SetContent(HeaderSep);
	StandaloneHeaderBox->AddChildToVerticalBox(HeaderSepSize)->SetPadding(FMargin(0.f, 6.f, 0.f, 10.f));

	StandaloneHeaderBox->AddChildToVerticalBox(MakeText(WidgetTree, LOCTEXT("AltarSubtitle", "Ofrece Sangre para despertar nuevos combos en tus armas."), 15, Colors::TextSecondary()));

	UBorder* CurrencyBar = Make<UBorder>(WidgetTree);
	CurrencyBar->SetBrush(FVelmaraUITheme::MakeOptionRowBrush(FVelmaraUITheme::Ash()));
	CurrencyBar->SetPadding(FMargin(16.f, 10.f));
	CurrencyText = MakeDisplayText(WidgetTree, FText::GetEmpty(), 16, Colors::TextPrimary(), 120);
	CurrencyBar->SetContent(CurrencyText);
	StandaloneHeaderBox->AddChildToVerticalBox(CurrencyBar)->SetPadding(FMargin(0.f, 14.f, 0.f, 16.f));

	TabsContainer = Make<UHorizontalBox>(WidgetTree);
	Main->AddChildToVerticalBox(TabsContainer)->SetPadding(FMargin(24.f, 0.f, 0.f, 12.f));

	UHorizontalBox* WeaponHeader = Make<UHorizontalBox>(WidgetTree);
	Main->AddChildToVerticalBox(WeaponHeader)->SetPadding(FMargin(30.f, 0.f, 0.f, 12.f));

	WeaponNameText = MakeDisplayText(WidgetTree, FText::GetEmpty(), 18, FVelmaraUITheme::Bone(), 160);
	if (UHorizontalBoxSlot* WeaponNameSlot = WeaponHeader->AddChildToHorizontalBox(WeaponNameText))
	{
		WeaponNameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		WeaponNameSlot->SetVerticalAlignment(VAlign_Center);
	}

	WeaponProgressText = MakeDisplayText(WidgetTree, FText::GetEmpty(), 13, FVelmaraUITheme::BloodBright(), 120);
	WeaponHeader->AddChildToHorizontalBox(WeaponProgressText)->SetVerticalAlignment(VAlign_Center);

	EmptyText = MakeText(WidgetTree, FText::GetEmpty(), 15, Colors::TextSecondary());
	EmptyText->SetAutoWrapText(true);
	EmptyText->SetVisibility(ESlateVisibility::Collapsed);
	Main->AddChildToVerticalBox(EmptyText)->SetPadding(FMargin(30.f, 8.f, 0.f, 12.f));

	UScrollBox* RowsScroll = Make<UScrollBox>(WidgetTree);
	RowsScroll->SetConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible);
	if (UVerticalBoxSlot* ScrollSlot = Main->AddChildToVerticalBox(RowsScroll))
	{
		ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	RowsContainer = Make<UVerticalBox>(WidgetTree);
	RowsScroll->AddChild(RowsContainer);

	FeedbackText = MakeText(WidgetTree, FText::GetEmpty(), 14, Colors::TextSecondary(), true);
	FeedbackText->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* FeedbackSlot = Main->AddChildToVerticalBox(FeedbackText))
	{
		FeedbackSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
		FeedbackSlot->SetHorizontalAlignment(HAlign_Center);
	}

	UHorizontalBox* Footer = Make<UHorizontalBox>(WidgetTree);
	StandaloneFooterBox = Footer;
	Main->AddChildToVerticalBox(Footer)->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));

	UHorizontalBoxSlot* HintSlot = Footer->AddChildToHorizontalBox(MakeText(WidgetTree, LOCTEXT("AltarHint", "Esc / B para cerrar"), 14, Colors::TextSecondary()));
	HintSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	HintSlot->SetVerticalAlignment(VAlign_Center);

	USizeBox* CloseSize = Make<USizeBox>(WidgetTree);
	CloseSize->SetWidthOverride(150.f);
	CloseSize->SetHeightOverride(42.f);

	UButton* CloseButton = Make<UButton>(WidgetTree);
	CloseButton->SetStyle(MakeButtonStyle(Colors::BloodRow(), Colors::RowHovered(), Colors::Locked(), FVelmaraUITheme::Ash(), 1.f));
	CloseButton->OnClicked.AddDynamic(this, &UWeaponUpgradeMenuWidget::HandleCloseClicked);
	CloseSize->SetContent(CloseButton);

	if (UButtonSlot* CloseContentSlot = Cast<UButtonSlot>(CloseButton->SetContent(MakeDisplayText(WidgetTree, LOCTEXT("AltarClose", "CERRAR"), 15, Colors::TextPrimary(), 140))))
	{
		CloseContentSlot->SetHorizontalAlignment(HAlign_Center);
		CloseContentSlot->SetVerticalAlignment(VAlign_Center);
	}

	Footer->AddChildToHorizontalBox(CloseSize)->SetVerticalAlignment(VAlign_Center);

	UpdateStandaloneChromeVisibility();
}

void UWeaponUpgradeMenuWidget::OnEmbeddedModeChanged()
{
	UpdateStandaloneChromeVisibility();
}

void UWeaponUpgradeMenuWidget::UpdateStandaloneChromeVisibility()
{
	if (!RootDimmer || !PanelBorder || !PanelSizeBox) return;

	if (bEmbeddedMode)
	{
		RootDimmer->SetBrush(UpgradeUIStyle::MakeTransparentBrush());
		RootDimmer->SetPadding(FMargin(0.f));
		RootDimmer->SetHorizontalAlignment(HAlign_Fill);
		RootDimmer->SetVerticalAlignment(VAlign_Fill);
		PanelSizeBox->ClearWidthOverride();
		PanelBorder->SetBrush(UpgradeUIStyle::MakeTransparentBrush());
		PanelBorder->SetPadding(FMargin(0.f));
		if (StandaloneHeaderBox) StandaloneHeaderBox->SetVisibility(ESlateVisibility::Collapsed);
		if (StandaloneFooterBox) StandaloneFooterBox->SetVisibility(ESlateVisibility::Collapsed);
		if (FeedbackText) FeedbackText->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		RootDimmer->SetBrush(UpgradeUIStyle::MakeSharpBrush(UpgradeUIStyle::Colors::Overlay()));
		RootDimmer->SetPadding(FMargin(24.f));
		RootDimmer->SetHorizontalAlignment(HAlign_Center);
		RootDimmer->SetVerticalAlignment(VAlign_Center);
		PanelSizeBox->SetWidthOverride(960.f);
		PanelBorder->SetBrush(UpgradeUIStyle::MakeSharpBrush(UpgradeUIStyle::Colors::BloodPanel(), UpgradeUIStyle::Colors::PanelOutline(), 1.f));
		PanelBorder->SetPadding(FMargin(32.f, 28.f));
		if (StandaloneHeaderBox) StandaloneHeaderBox->SetVisibility(ESlateVisibility::Visible);
		if (StandaloneFooterBox) StandaloneFooterBox->SetVisibility(ESlateVisibility::Visible);
		if (FeedbackText) FeedbackText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void UWeaponUpgradeMenuWidget::OnUpgradeComponentSet()
{
	if (FeedbackText)
	{
		FeedbackText->SetText(FText::GetEmpty());
	}

	Weapons.Reset();
	RowsWeapon = nullptr;

	RefreshMenu();

	if (Rows.Num() > 0 && Rows[0])
	{
		NotifyComboSelected(Rows[0]->GetWeapon(), Rows[0]->GetUnlockTag());
	}
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
	if (!Upgrades || !WeaponNameText) return;

	if (CurrencyText)
	{
		if (Upgrades->HasCurrencyProvider())
		{
			CurrencyText->SetText(FText::Format(LOCTEXT("AltarCurrency", "{0} DISPONIBLE: {1}"),
				Upgrades->GetCurrencyDisplayName().ToUpper(), FVelmaraUITheme::FormatGroupedNumber(Upgrades->GetCurrencyAmount())));
		}
		else if (Upgrades->AreUpgradesFree())
		{
			CurrencyText->SetText(FText::Format(LOCTEXT("AltarCurrencyFree", "MODO PRUEBA: DESBLOQUEOS DE {0} GRATUITOS"),
				Upgrades->GetCurrencyDisplayName().ToUpper()));
		}
		else
		{
			CurrencyText->SetText(FText::Format(LOCTEXT("AltarCurrencyNone", "SIN PROVEEDOR DE {0} DISPONIBLE"),
				Upgrades->GetCurrencyDisplayName().ToUpper()));
		}
	}

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
		Tabs[Index]->SetProgressText(FText::Format(LOCTEXT("TabProgress", "{0} / {1}"),
			Upgrades->GetUnlockedComboCount(TabWeapon), Upgrades->GetComboUpgrades(TabWeapon).Num()));
	}

	UWeaponData* Weapon = GetSelectedWeapon();
	const int32 ComboCount = Upgrades->GetComboUpgrades(Weapon).Num();

	WeaponNameText->SetText(WeaponUpgradeUtils::GetWeaponDisplayName(Weapon).ToUpper());
	WeaponProgressText->SetText(Weapon && ComboCount > 0
		? FText::Format(LOCTEXT("WeaponProgress", "{0} / {1} COMBOS"), Upgrades->GetUnlockedComboCount(Weapon), ComboCount)
		: FText::GetEmpty());

	if (!Weapon)
	{
		EmptyText->SetText(LOCTEXT("AltarNoWeapons", "No tienes armas disponibles para consagrar. Equipa un arma o añádela al componente Weapon Upgrade."));
		EmptyText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else if (ComboCount == 0)
	{
		EmptyText->SetText(LOCTEXT("AltarNoCombos", "Esta arma aún no posee combos desbloqueables configurados."));
		EmptyText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else
	{
		EmptyText->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (RowsWeapon != Weapon || Rows.Num() != ComboCount)
	{
		RebuildRows();
		if (Rows.Num() > 0 && Rows[0])
		{
			NotifyComboSelected(Rows[0]->GetWeapon(), Rows[0]->GetUnlockTag());
		}
		else
		{
			NotifyComboSelected(Weapon, FGameplayTag());
		}
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
	using namespace UpgradeUIStyle;

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
		if (Index > 0)
		{
			USizeBox* DivSize = Make<USizeBox>(WidgetTree);
			DivSize->SetWidthOverride(1.f);
			DivSize->SetHeightOverride(18.f);
			UBorder* DivLine = Make<UBorder>(WidgetTree);
			DivLine->SetBrush(FVelmaraUITheme::MakeSolidBrush(FVelmaraUITheme::Ash(0.75f)));
			DivSize->SetContent(DivLine);
			if (UHorizontalBoxSlot* DSlot = TabsContainer->AddChildToHorizontalBox(DivSize))
			{
				DSlot->SetVerticalAlignment(VAlign_Center);
				DSlot->SetPadding(FMargin(4.f, 0.f, 4.f, 6.f));
			}
		}

		UWeaponUpgradeTabWidget* Tab = CreateWidget<UWeaponUpgradeTabWidget>(this, TabClass);
		if (!Tab) continue;

		Tab->Setup(this, Index, WeaponUpgradeUtils::GetWeaponDisplayName(Weapons[Index]));
		TabsContainer->AddChildToHorizontalBox(Tab)->SetVerticalAlignment(VAlign_Center);
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

	if (UWidget* FirstFocus = GetFirstFocusableWidget())
	{
		FirstFocus->SetKeyboardFocus();
	}
}

void UWeaponUpgradeMenuWidget::CycleWeapon(const int32 Delta)
{
	if (Weapons.Num() <= 1) return;

	int32 CurrentIdx = Weapons.IndexOfByKey(SelectedWeapon);
	if (CurrentIdx == INDEX_NONE) CurrentIdx = 0;

	const int32 NextIdx = (CurrentIdx + Delta + Weapons.Num()) % Weapons.Num();
	SelectWeapon(NextIdx);
}

void UWeaponUpgradeMenuWidget::NotifyComboSelected(UWeaponData* InWeapon, const FGameplayTag& InUnlockTag)
{
	OnComboSelected.Broadcast(InWeapon, InUnlockTag);
}

void UWeaponUpgradeMenuWidget::ShowUnlockResult(const EWeaponUpgradeResult Result, const UWeaponData* Weapon, const FGameplayTag& UnlockTag)
{
	using namespace UpgradeUIStyle;

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
		Message = FText::Format(LOCTEXT("AltarResultSuccess", "Combo despertado: {0}"), Upgrade.DisplayName);
		bIsError = false;
		break;
	case EWeaponUpgradeResult::AlreadyUnlocked:
		Message = LOCTEXT("AltarResultAlready", "Este combo ya fue despertado.");
		break;
	case EWeaponUpgradeResult::MissingRequirements:
		Message = LOCTEXT("AltarResultRequirements", "Debes despertar primero los combos previos.");
		break;
	case EWeaponUpgradeResult::NotEnoughCurrency:
		Message = FText::Format(LOCTEXT("AltarResultNoCurrency", "No tienes suficiente {0} para este combo."), CurrencyName);
		break;
	case EWeaponUpgradeResult::NoCurrencySystem:
		Message = FText::Format(LOCTEXT("AltarResultNoCurrencySystem", "No hay un proveedor de {0} disponible."), CurrencyName);
		break;
	default:
		Message = LOCTEXT("AltarResultInvalid", "El combo no está configurado correctamente.");
		break;
	}

	if (FeedbackText)
	{
		FeedbackText->SetText(Message);
		FeedbackText->SetColorAndOpacity(FSlateColor(bIsError ? Colors::Error() : Colors::Success()));
	}

	OnFeedbackMessage.Broadcast(Message, bIsError);
	OnComboSelected.Broadcast(const_cast<UWeaponData*>(Weapon), UnlockTag);
	RefreshMenu();
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
