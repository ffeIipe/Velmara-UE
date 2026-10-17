#include "Features/WeaponUpgradeSystem/UI/WeaponUpgradeTabWidget.h"

#include "../../UpgradeSystem/UI/UpgradeUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Features/WeaponUpgradeSystem/UI/WeaponUpgradeMenuWidget.h"

void UWeaponUpgradeTabWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
}

void UWeaponUpgradeTabWidget::BuildLayout()
{
	using namespace UpgradeUIStyle;

	USizeBox* TabSize = Make<USizeBox>(WidgetTree);
	TabSize->SetMinDesiredWidth(190.f);
	TabSize->SetHeightOverride(58.f);
	WidgetTree->RootWidget = TabSize;

	TabButton = Make<UButton>(WidgetTree);
	TabButton->OnClicked.AddDynamic(this, &UWeaponUpgradeTabWidget::HandleClicked);
	TabSize->SetContent(TabButton);

	UVerticalBox* Content = Make<UVerticalBox>(WidgetTree);

	LabelText = MakeText(WidgetTree, FText::GetEmpty(), 16, Colors::TextPrimary(), true);
	LabelText->SetJustification(ETextJustify::Center);
	Content->AddChildToVerticalBox(LabelText)->SetHorizontalAlignment(HAlign_Center);

	ProgressText = MakeText(WidgetTree, FText::GetEmpty(), 12, Colors::TextSecondary());
	ProgressText->SetJustification(ETextJustify::Center);
	Content->AddChildToVerticalBox(ProgressText)->SetHorizontalAlignment(HAlign_Center);

	if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(TabButton->SetContent(Content)))
	{
		ContentSlot->SetHorizontalAlignment(HAlign_Center);
		ContentSlot->SetVerticalAlignment(VAlign_Center);
		ContentSlot->SetPadding(FMargin(16.f, 4.f));
	}

	SetSelected(false);
}

void UWeaponUpgradeTabWidget::Setup(UWeaponUpgradeMenuWidget* InOwnerMenu, const int32 InWeaponIndex, const FText& Label)
{
	OwnerMenu = InOwnerMenu;
	WeaponIndex = InWeaponIndex;

	if (LabelText)
	{
		LabelText->SetText(Label);
	}
}

void UWeaponUpgradeTabWidget::SetProgressText(const FText& Progress)
{
	if (ProgressText)
	{
		ProgressText->SetText(Progress);
	}
}

void UWeaponUpgradeTabWidget::SetSelected(const bool bInSelected)
{
	using namespace UpgradeUIStyle;

	bSelected = bInSelected;
	if (!TabButton) return;

	if (bSelected)
	{
		TabButton->SetStyle(MakeButtonStyle(Colors::Blood(), Colors::BloodHovered(), Colors::BloodPressed()));
		LabelText->SetColorAndOpacity(FSlateColor(Colors::TextOnBlood()));
		ProgressText->SetColorAndOpacity(FSlateColor(Colors::TextOnBlood()));
	}
	else
	{
		TabButton->SetStyle(MakeButtonStyle(Colors::BloodRow(), Colors::RowHovered(), Colors::BloodPressed(), Colors::BloodDim(), 1.f));
		LabelText->SetColorAndOpacity(FSlateColor(Colors::TextPrimary()));
		ProgressText->SetColorAndOpacity(FSlateColor(Colors::TextSecondary()));
	}
}

void UWeaponUpgradeTabWidget::HandleClicked()
{
	if (UWeaponUpgradeMenuWidget* Menu = OwnerMenu.Get())
	{
		Menu->SelectWeapon(WeaponIndex);
	}
}
