#include "Features/AttributeTreeSystem/UI/AttributeTreeMenuWidget.h"

#include "../../UpgradeSystem/UI/UpgradeUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Features/AttributeTreeSystem/Core/AttributeTreeComponent.h"
#include "Features/AttributeTreeSystem/UI/AttributeTreeRowWidget.h"

#define LOCTEXT_NAMESPACE "AttributeTree"

void UAttributeTreeMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!RowWidgetClass)
	{
		RowWidgetClass = UAttributeTreeRowWidget::StaticClass();
	}

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
}

UAttributeTreeComponent* UAttributeTreeMenuWidget::GetAttributeTree() const
{
	return Cast<UAttributeTreeComponent>(GetUpgradeComponent());
}

void UAttributeTreeMenuWidget::BuildLayout()
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
	PanelSize->SetWidthOverride(820.f);
	Dimmer->SetContent(PanelSize);

	UBorder* Panel = Make<UBorder>(WidgetTree);
	Panel->SetBrush(MakeRoundedBrush(Colors::Panel(), 14.f, Colors::PanelOutline(), 1.5f));
	Panel->SetPadding(FMargin(32.f, 28.f));
	PanelSize->SetContent(Panel);

	UVerticalBox* Main = Make<UVerticalBox>(WidgetTree);
	Panel->SetContent(Main);

	// Encabezado
	Main->AddChildToVerticalBox(MakeText(WidgetTree, LOCTEXT("MenuTitle", "ÁRBOL DE ATRIBUTOS"), 30, Colors::Gold(), true));

	Main->AddChildToVerticalBox(MakeText(WidgetTree, LOCTEXT("MenuSubtitle", "Mejorá cada atributo del Nivel I al Nivel V."), 14, Colors::TextSecondary()))
	    ->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));

	// Moneda
	UBorder* CurrencyBar = Make<UBorder>(WidgetTree);
	CurrencyBar->SetBrush(MakeRoundedBrush(Colors::Row(), 8.f, Colors::RowOutline(), 1.f));
	CurrencyBar->SetPadding(FMargin(16.f, 10.f));
	CurrencyText = MakeText(WidgetTree, FText::GetEmpty(), 17, Colors::TextPrimary(), true);
	CurrencyBar->SetContent(CurrencyText);
	Main->AddChildToVerticalBox(CurrencyBar)->SetPadding(FMargin(0.f, 18.f, 0.f, 16.f));

	// Ramas
	RowsContainer = Make<UVerticalBox>(WidgetTree);
	Main->AddChildToVerticalBox(RowsContainer);

	// Mensaje de resultado de la última compra
	FeedbackText = MakeText(WidgetTree, FText::GetEmpty(), 14, Colors::TextSecondary(), true);
	FeedbackText->SetJustification(ETextJustify::Center);
	UVerticalBoxSlot* FeedbackSlot = Main->AddChildToVerticalBox(FeedbackText);
	FeedbackSlot->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
	FeedbackSlot->SetHorizontalAlignment(HAlign_Center);

	// Pie: ayuda + botón cerrar
	UHorizontalBox* Footer = Make<UHorizontalBox>(WidgetTree);
	Main->AddChildToVerticalBox(Footer)->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));

	UHorizontalBoxSlot* HintSlot = Footer->AddChildToHorizontalBox(MakeText(WidgetTree, LOCTEXT("MenuHint", "Esc para cerrar"), 13, Colors::TextSecondary()));
	HintSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	HintSlot->SetVerticalAlignment(VAlign_Center);

	USizeBox* CloseSize = Make<USizeBox>(WidgetTree);
	CloseSize->SetWidthOverride(140.f);
	CloseSize->SetHeightOverride(42.f);

	UButton* CloseButton = Make<UButton>(WidgetTree);
	CloseButton->SetStyle(MakeButtonStyle(Colors::Row(), Colors::RowHovered(), Colors::Locked(), Colors::GoldDim(), 1.f));
	CloseButton->OnClicked.AddDynamic(this, &UAttributeTreeMenuWidget::HandleCloseClicked);
	CloseSize->SetContent(CloseButton);

	if (UButtonSlot* CloseContentSlot = Cast<UButtonSlot>(CloseButton->SetContent(MakeText(WidgetTree, LOCTEXT("MenuClose", "Cerrar"), 15, Colors::TextPrimary(), true))))
	{
		CloseContentSlot->SetHorizontalAlignment(HAlign_Center);
		CloseContentSlot->SetVerticalAlignment(VAlign_Center);
	}

	Footer->AddChildToHorizontalBox(CloseSize)->SetVerticalAlignment(VAlign_Center);
}

void UAttributeTreeMenuWidget::OnUpgradeComponentSet()
{
	if (FeedbackText)
	{
		FeedbackText->SetText(FText::GetEmpty());
	}

	RebuildRows();
	RefreshMenu();
}

void UAttributeTreeMenuWidget::BindToUpgradeComponent()
{
	Super::BindToUpgradeComponent();

	if (UAttributeTreeComponent* Tree = GetAttributeTree())
	{
		Tree->OnAttributeUpgraded.AddUniqueDynamic(this, &UAttributeTreeMenuWidget::HandleAttributeUpgraded);
	}
}

void UAttributeTreeMenuWidget::UnbindFromUpgradeComponent()
{
	if (UAttributeTreeComponent* Tree = GetAttributeTree())
	{
		Tree->OnAttributeUpgraded.RemoveDynamic(this, &UAttributeTreeMenuWidget::HandleAttributeUpgraded);
	}

	Super::UnbindFromUpgradeComponent();
}

void UAttributeTreeMenuWidget::RebuildRows()
{
	if (!RowsContainer) return;

	RowsContainer->ClearChildren();
	Rows.Reset();

	UAttributeTreeComponent* Tree = GetAttributeTree();
	if (!Tree) return;

	TSubclassOf<UAttributeTreeRowWidget> RowClass = RowWidgetClass;
	if (!RowClass)
	{
		RowClass = UAttributeTreeRowWidget::StaticClass();
	}

	for (const FAttributeUpgradeDefinition& Definition : Tree->GetUpgrades())
	{
		UAttributeTreeRowWidget* Row = CreateWidget<UAttributeTreeRowWidget>(this, RowClass);
		if (!Row) continue;

		Row->Setup(Tree, Definition.UpgradeTag, this);
		RowsContainer->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
		Rows.Add(Row);
	}
}

void UAttributeTreeMenuWidget::RefreshMenu()
{
	UAttributeTreeComponent* Tree = GetAttributeTree();
	if (!Tree) return;

	if (CurrencyText)
	{
		if (Tree->HasCurrencyProvider())
		{
			CurrencyText->SetText(FText::Format(LOCTEXT("MenuCurrency", "{0} disponible: {1}"),
				Tree->GetCurrencyDisplayName(), FText::AsNumber(Tree->GetCurrencyAmount())));
		}
		else if (Tree->AreUpgradesFree())
		{
			CurrencyText->SetText(FText::Format(LOCTEXT("MenuCurrencyFree", "Sin proveedor de {0}: modo prueba, las mejoras son gratis"),
				Tree->GetCurrencyDisplayName()));
		}
		else
		{
			CurrencyText->SetText(FText::Format(LOCTEXT("MenuCurrencyNone", "Sin proveedor de {0} disponible"), Tree->GetCurrencyDisplayName()));
		}
	}

	if (Rows.Num() != Tree->GetUpgrades().Num())
	{
		RebuildRows();
		return;
	}

	for (UAttributeTreeRowWidget* Row : Rows)
	{
		if (Row)
		{
			Row->Refresh();
		}
	}
}

void UAttributeTreeMenuWidget::ShowUpgradeResult(const EAttributeUpgradeResult Result, const FGameplayTag& UpgradeTag)
{
	using namespace UpgradeUIStyle;

	if (FeedbackText)
	{
		const UAttributeTreeComponent* Tree = GetAttributeTree();
		FAttributeUpgradeDefinition Definition;
		if (Tree)
		{
			Tree->GetUpgradeDefinition(UpgradeTag, Definition);
		}

		const FText CurrencyName = Tree ? Tree->GetCurrencyDisplayName() : FText::GetEmpty();
		FText Message;
		bool bIsError = true;

		switch (Result)
		{
		case EAttributeUpgradeResult::Success:
			Message = FText::Format(LOCTEXT("MenuResultSuccess", "Mejora aplicada: {0} » Nivel {1}"), Definition.DisplayName,
				FText::FromString(AttributeTreeUtils::ToRoman(Tree ? Tree->GetUpgradeLevel(UpgradeTag) : 0)));
			bIsError = false;
			break;
		case EAttributeUpgradeResult::NotEnoughCurrency:
			Message = FText::Format(LOCTEXT("MenuResultNoCurrency", "No tenés suficiente {0} para esta mejora."), CurrencyName);
			break;
		case EAttributeUpgradeResult::MaxLevelReached:
			Message = LOCTEXT("MenuResultMax", "Esta rama ya está en el nivel máximo.");
			break;
		case EAttributeUpgradeResult::NoCurrencySystem:
			Message = FText::Format(LOCTEXT("MenuResultNoCurrencySystem", "No hay un proveedor de {0} disponible."), CurrencyName);
			break;
		case EAttributeUpgradeResult::NoAbilitySystem:
			Message = LOCTEXT("MenuResultNoASC", "El personaje no tiene un AbilitySystemComponent.");
			break;
		default:
			Message = LOCTEXT("MenuResultInvalid", "La mejora no está configurada correctamente.");
			break;
		}

		FeedbackText->SetText(Message);
		FeedbackText->SetColorAndOpacity(FSlateColor(bIsError ? Colors::Error() : Colors::Success()));
	}

	RefreshMenu();

	// Si el botón clickeado quedó deshabilitado (nivel máximo), el foco vuelve al menú para que Esc siga funcionando.
	SetFocus();
}

void UAttributeTreeMenuWidget::HandleCloseClicked()
{
	RequestClose();
}

void UAttributeTreeMenuWidget::HandleAttributeUpgraded(FGameplayTag UpgradeTag, int32 NewLevel)
{
	RefreshMenu();
}

#undef LOCTEXT_NAMESPACE
