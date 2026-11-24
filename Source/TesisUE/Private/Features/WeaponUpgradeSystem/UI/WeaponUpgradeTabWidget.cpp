#include "Features/WeaponUpgradeSystem/UI/WeaponUpgradeTabWidget.h"

#include "../../UpgradeSystem/UI/UpgradeUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Features/WeaponUpgradeSystem/UI/WeaponUpgradeMenuWidget.h"
#include "UI/Style/VelmaraUITheme.h"

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
	TabSize->SetMinDesiredWidth(148.f);
	TabSize->SetHeightOverride(52.f);
	WidgetTree->RootWidget = TabSize;

	TabButton = Make<UButton>(WidgetTree);
	TabButton->SetStyle(MakeTransparentButtonStyle());
	TabButton->OnClicked.AddDynamic(this, &UWeaponUpgradeTabWidget::HandleClicked);
	TabButton->OnHovered.AddDynamic(this, &UWeaponUpgradeTabWidget::HandleHovered);
	TabButton->OnUnhovered.AddDynamic(this, &UWeaponUpgradeTabWidget::HandleUnhovered);
	TabSize->SetContent(TabButton);

	UVerticalBox* Content = Make<UVerticalBox>(WidgetTree);

	LabelText = MakeDisplayText(WidgetTree, FText::GetEmpty(), 14, FVelmaraUITheme::BoneMuted(), 160);
	LabelText->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* LSlot = Content->AddChildToVerticalBox(LabelText))
	{
		LSlot->SetHorizontalAlignment(HAlign_Center);
	}

	ProgressText = MakeText(WidgetTree, FText::GetEmpty(), 11, FVelmaraUITheme::BoneMuted(0.65f));
	ProgressText->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* PSlot = Content->AddChildToVerticalBox(ProgressText))
	{
		PSlot->SetHorizontalAlignment(HAlign_Center);
		PSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 4.f));
	}

	USizeBox* LineSize = Make<USizeBox>(WidgetTree);
	LineSize->SetWidthOverride(120.f);
	LineSize->SetHeightOverride(10.f);
	ActiveLineImage = Make<UImage>(WidgetTree);
	ActiveLineImage->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnTabActiveLine),
		FVector2D(120.f, 10.f),
		FVelmaraUITheme::BloodBright()));
	ActiveLineImage->SetVisibility(ESlateVisibility::Hidden);
	LineSize->SetContent(ActiveLineImage);
	if (UVerticalBoxSlot* LineSlot = Content->AddChildToVerticalBox(LineSize))
	{
		LineSlot->SetHorizontalAlignment(HAlign_Center);
	}

	if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(TabButton->SetContent(Content)))
	{
		ContentSlot->SetHorizontalAlignment(HAlign_Center);
		ContentSlot->SetVerticalAlignment(VAlign_Center);
		ContentSlot->SetPadding(FMargin(14.f, 4.f));
	}

	SetSelected(false);
}

void UWeaponUpgradeTabWidget::Setup(UWeaponUpgradeMenuWidget* InOwnerMenu, const int32 InWeaponIndex, const FText& Label)
{
	OwnerMenu = InOwnerMenu;
	WeaponIndex = InWeaponIndex;

	if (LabelText)
	{
		LabelText->SetText(Label.ToUpper());
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
	bSelected = bInSelected;
	RefreshVisualState();
}

void UWeaponUpgradeTabWidget::RefreshVisualState()
{
	if (!LabelText || !ProgressText) return;

	if (bSelected)
	{
		LabelText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BloodBright()));
		ProgressText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone(0.85f)));
		if (ActiveLineImage)
		{
			ActiveLineImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
	}
	else if (bHovered)
	{
		LabelText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
		ProgressText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted()));
		if (ActiveLineImage)
		{
			ActiveLineImage->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	else
	{
		LabelText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted(0.68f)));
		ProgressText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted(0.45f)));
		if (ActiveLineImage)
		{
			ActiveLineImage->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void UWeaponUpgradeTabWidget::HandleClicked()
{
	if (UWeaponUpgradeMenuWidget* Menu = OwnerMenu.Get())
	{
		Menu->SelectWeapon(WeaponIndex);
	}
}

void UWeaponUpgradeTabWidget::HandleHovered()
{
	bHovered = true;
	RefreshVisualState();
}

void UWeaponUpgradeTabWidget::HandleUnhovered()
{
	bHovered = false;
	RefreshVisualState();
}
