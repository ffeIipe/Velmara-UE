#include "Features/AttributeTreeSystem/UI/AttributeTreeMenuWidget.h"

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
#include "Features/AttributeTreeSystem/Core/AttributeTreeComponent.h"
#include "Features/AttributeTreeSystem/UI/AttributeTreeRowWidget.h"
#include "UI/Style/VelmaraUITheme.h"

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

UWidget* UAttributeTreeMenuWidget::GetFirstFocusableWidget() const
{
	for (const UAttributeTreeRowWidget* Row : Rows)
	{
		if (Row && Row->GetFocusTarget())
		{
			return Row->GetFocusTarget();
		}
	}
	return nullptr;
}

void UAttributeTreeMenuWidget::BuildLayout()
{
	using namespace UpgradeUIStyle;

	RootDimmer = Make<UBorder>(WidgetTree);
	RootDimmer->SetBrush(MakeSharpBrush(Colors::Overlay()));
	RootDimmer->SetPadding(FMargin(24.f));
	RootDimmer->SetHorizontalAlignment(HAlign_Center);
	RootDimmer->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = RootDimmer;

	PanelSizeBox = Make<USizeBox>(WidgetTree);
	PanelSizeBox->SetWidthOverride(920.f);
	RootDimmer->SetContent(PanelSizeBox);

	PanelBorder = Make<UBorder>(WidgetTree);
	PanelBorder->SetBrush(MakeSharpBrush(Colors::Panel(), Colors::PanelOutline(), 1.f));
	PanelBorder->SetPadding(FMargin(32.f, 28.f));
	PanelSizeBox->SetContent(PanelBorder);

	UVerticalBox* Main = Make<UVerticalBox>(WidgetTree);
	PanelBorder->SetContent(Main);

	StandaloneHeaderBox = Make<UVerticalBox>(WidgetTree);
	Main->AddChildToVerticalBox(StandaloneHeaderBox);

	StandaloneHeaderBox->AddChildToVerticalBox(MakeDisplayText(WidgetTree, LOCTEXT("MenuTitle", "ÁRBOL DE ATRIBUTOS"), 30, FVelmaraUITheme::Bone(), 200));

	USizeBox* HeaderSepSize = Make<USizeBox>(WidgetTree);
	HeaderSepSize->SetHeightOverride(18.f);
	UImage* HeaderSep = Make<UImage>(WidgetTree);
	HeaderSep->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnSepStar),
		FVector2D(420.f, 18.f),
		FVelmaraUITheme::Bone(0.8f)));
	HeaderSepSize->SetContent(HeaderSep);
	StandaloneHeaderBox->AddChildToVerticalBox(HeaderSepSize)->SetPadding(FMargin(0.f, 6.f, 0.f, 10.f));

	StandaloneHeaderBox->AddChildToVerticalBox(MakeText(WidgetTree, LOCTEXT("MenuSubtitle", "Fortalece tu sangre del Nivel I al Nivel V."), 15, Colors::TextSecondary()));

	UBorder* CurrencyBar = Make<UBorder>(WidgetTree);
	CurrencyBar->SetBrush(FVelmaraUITheme::MakeOptionRowBrush(FVelmaraUITheme::Ash()));
	CurrencyBar->SetPadding(FMargin(16.f, 10.f));
	CurrencyText = MakeDisplayText(WidgetTree, FText::GetEmpty(), 16, Colors::TextPrimary(), 120);
	CurrencyBar->SetContent(CurrencyText);
	StandaloneHeaderBox->AddChildToVerticalBox(CurrencyBar)->SetPadding(FMargin(0.f, 14.f, 0.f, 18.f));

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

	UHorizontalBoxSlot* HintSlot = Footer->AddChildToHorizontalBox(MakeText(WidgetTree, LOCTEXT("MenuHint", "Esc / B para cerrar"), 14, Colors::TextSecondary()));
	HintSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	HintSlot->SetVerticalAlignment(VAlign_Center);

	USizeBox* CloseSize = Make<USizeBox>(WidgetTree);
	CloseSize->SetWidthOverride(150.f);
	CloseSize->SetHeightOverride(42.f);

	UButton* CloseButton = Make<UButton>(WidgetTree);
	CloseButton->SetStyle(MakeButtonStyle(Colors::Row(), Colors::RowHovered(), Colors::Locked(), FVelmaraUITheme::Ash(), 1.f));
	CloseButton->OnClicked.AddDynamic(this, &UAttributeTreeMenuWidget::HandleCloseClicked);
	CloseSize->SetContent(CloseButton);

	if (UButtonSlot* CloseContentSlot = Cast<UButtonSlot>(CloseButton->SetContent(MakeDisplayText(WidgetTree, LOCTEXT("MenuClose", "CERRAR"), 15, Colors::TextPrimary(), 140))))
	{
		CloseContentSlot->SetHorizontalAlignment(HAlign_Center);
		CloseContentSlot->SetVerticalAlignment(VAlign_Center);
	}

	Footer->AddChildToHorizontalBox(CloseSize)->SetVerticalAlignment(VAlign_Center);

	UpdateStandaloneChromeVisibility();
}

void UAttributeTreeMenuWidget::OnEmbeddedModeChanged()
{
	UpdateStandaloneChromeVisibility();
}

void UAttributeTreeMenuWidget::UpdateStandaloneChromeVisibility()
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
		PanelSizeBox->SetWidthOverride(920.f);
		PanelBorder->SetBrush(UpgradeUIStyle::MakeSharpBrush(UpgradeUIStyle::Colors::Panel(), UpgradeUIStyle::Colors::PanelOutline(), 1.f));
		PanelBorder->SetPadding(FMargin(32.f, 28.f));
		if (StandaloneHeaderBox) StandaloneHeaderBox->SetVisibility(ESlateVisibility::Visible);
		if (StandaloneFooterBox) StandaloneFooterBox->SetVisibility(ESlateVisibility::Visible);
		if (FeedbackText) FeedbackText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void UAttributeTreeMenuWidget::OnUpgradeComponentSet()
{
	if (FeedbackText)
	{
		FeedbackText->SetText(FText::GetEmpty());
	}

	RebuildRows();
	RefreshMenu();

	if (Rows.Num() > 0 && Rows[0])
	{
		NotifyRowSelected(Rows[0]->GetUpgradeTag());
	}
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
			CurrencyText->SetText(FText::Format(LOCTEXT("MenuCurrency", "{0} DISPONIBLE: {1}"),
				Tree->GetCurrencyDisplayName().ToUpper(), FVelmaraUITheme::FormatGroupedNumber(Tree->GetCurrencyAmount())));
		}
		else if (Tree->AreUpgradesFree())
		{
			CurrencyText->SetText(FText::Format(LOCTEXT("MenuCurrencyFree", "MODO PRUEBA: MEJORAS DE {0} GRATUITAS"),
				Tree->GetCurrencyDisplayName().ToUpper()));
		}
		else
		{
			CurrencyText->SetText(FText::Format(LOCTEXT("MenuCurrencyNone", "SIN PROVEEDOR DE {0} DISPONIBLE"), Tree->GetCurrencyDisplayName().ToUpper()));
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

void UAttributeTreeMenuWidget::NotifyRowSelected(const FGameplayTag& UpgradeTag)
{
	OnRowSelected.Broadcast(UpgradeTag);
}

void UAttributeTreeMenuWidget::ShowUpgradeResult(const EAttributeUpgradeResult Result, const FGameplayTag& UpgradeTag)
{
	using namespace UpgradeUIStyle;

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
		Message = FText::Format(LOCTEXT("MenuResultSuccess", "Mejora consagrada: {0}  »  Nivel {1}"), Definition.DisplayName,
			FText::FromString(AttributeTreeUtils::ToRoman(Tree ? Tree->GetUpgradeLevel(UpgradeTag) : 0)));
		bIsError = false;
		break;
	case EAttributeUpgradeResult::NotEnoughCurrency:
		Message = FText::Format(LOCTEXT("MenuResultNoCurrency", "No tienes suficiente {0} para esta mejora."), CurrencyName);
		break;
	case EAttributeUpgradeResult::MaxLevelReached:
		Message = LOCTEXT("MenuResultMax", "Este atributo ya alcanzó su nivel máximo.");
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

	if (FeedbackText)
	{
		FeedbackText->SetText(Message);
		FeedbackText->SetColorAndOpacity(FSlateColor(bIsError ? Colors::Error() : Colors::Success()));
	}

	OnFeedbackMessage.Broadcast(Message, bIsError);
	OnRowSelected.Broadcast(UpgradeTag);
	RefreshMenu();
}

void UAttributeTreeMenuWidget::HandleCloseClicked()
{
	RequestClose();
}

void UAttributeTreeMenuWidget::HandleAttributeUpgraded(FGameplayTag UpgradeTag, int32 NewLevel)
{
	RefreshMenu();
	OnRowSelected.Broadcast(UpgradeTag);
}

#undef LOCTEXT_NAMESPACE
