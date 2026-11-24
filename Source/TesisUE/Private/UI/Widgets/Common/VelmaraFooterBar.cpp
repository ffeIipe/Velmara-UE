#include "UI/Widgets/Common/VelmaraFooterBar.h"
#include "Blueprint/WidgetTree.h"
#include "CommonActionWidget.h"
#include "CommonInputSubsystem.h"
#include "CommonTextBlock.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Engine/DataTable.h"
#include "Engine/LocalPlayer.h"
#include "UI/Style/VelmaraUITheme.h"

UVelmaraFooterBar::UVelmaraFooterBar(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedRef<SWidget> UVelmaraFooterBar::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildFooterTree();
	}
	return Super::RebuildWidget();
}

void UVelmaraFooterBar::NativeConstruct()
{
	Super::NativeConstruct();

	if (ULocalPlayer* LP = GetOwningLocalPlayer())
	{
		if (UCommonInputSubsystem* InputSys = UCommonInputSubsystem::Get(LP))
		{
			InputMethodChangedHandle = InputSys->OnInputMethodChangedNative.AddUObject(this, &UVelmaraFooterBar::HandleInputMethodChanged);
		}
	}

	RebuildActionPrompts();
}

void UVelmaraFooterBar::NativeDestruct()
{
	if (InputMethodChangedHandle.IsValid())
	{
		if (ULocalPlayer* LP = GetOwningLocalPlayer())
		{
			if (UCommonInputSubsystem* InputSys = UCommonInputSubsystem::Get(LP))
			{
				InputSys->OnInputMethodChangedNative.Remove(InputMethodChangedHandle);
			}
		}
		InputMethodChangedHandle.Reset();
	}

	Super::NativeDestruct();
}

void UVelmaraFooterBar::BuildFooterTree()
{
	UHorizontalBox* RootBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("FooterRootBox"));
	WidgetTree->RootWidget = RootBox;

	MottoBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("FooterMottoBox"));
	if (UHorizontalBoxSlot* MottoSlot = RootBox->AddChildToHorizontalBox(MottoBox))
	{
		MottoSlot->SetHorizontalAlignment(HAlign_Left);
		MottoSlot->SetVerticalAlignment(VAlign_Center);
	}

	UCommonTextBlock* LeftMotto = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("LeftMottoText"));
	LeftMotto->SetText(NSLOCTEXT("VelmaraUI", "MottoLeft", "ALGUNAS COSAS"));
	LeftMotto->SetFont(FVelmaraUITheme::GetDisplayFont(14, 220));
	LeftMotto->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted(0.80f)));
	if (UHorizontalBoxSlot* LeftSlot = MottoBox->AddChildToHorizontalBox(LeftMotto))
	{
		LeftSlot->SetVerticalAlignment(VAlign_Center);
	}

	USizeBox* StarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MottoStarSize"));
	StarSize->SetWidthOverride(16.0f);
	StarSize->SetHeightOverride(16.0f);
	UImage* StarImg = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MottoStarImg"));
	StarImg->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnRowStar),
		FVector2D(16.0f, 16.0f),
		FVelmaraUITheme::BoneMuted(0.85f)));
	StarSize->SetContent(StarImg);
	if (UHorizontalBoxSlot* StarSlot = MottoBox->AddChildToHorizontalBox(StarSize))
	{
		StarSlot->SetVerticalAlignment(VAlign_Center);
		StarSlot->SetPadding(FMargin(14.0f, 0.0f));
	}

	UCommonTextBlock* RightMotto = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("RightMottoText"));
	RightMotto->SetText(NSLOCTEXT("VelmaraUI", "MottoRight", "NUNCA MUEREN"));
	RightMotto->SetFont(FVelmaraUITheme::GetDisplayFont(14, 220));
	RightMotto->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted(0.80f)));
	if (UHorizontalBoxSlot* RightSlot = MottoBox->AddChildToHorizontalBox(RightMotto))
	{
		RightSlot->SetVerticalAlignment(VAlign_Center);
	}

	USpacer* MidSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("FooterMidSpacer"));
	if (UHorizontalBoxSlot* SpacerSlot = RootBox->AddChildToHorizontalBox(MidSpacer))
	{
		SpacerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	ActionsBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("FooterActionsBox"));
	if (UHorizontalBoxSlot* ActionsSlot = RootBox->AddChildToHorizontalBox(ActionsBox))
	{
		ActionsSlot->SetHorizontalAlignment(HAlign_Right);
		ActionsSlot->SetVerticalAlignment(VAlign_Center);
	}

	MottoBox->SetVisibility(bShowMotto ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}

void UVelmaraFooterBar::SetActions(const TArray<FVelmaraFooterAction>& InActions)
{
	CurrentActions = InActions;
	RebuildActionPrompts();
}

void UVelmaraFooterBar::SetShowMotto(bool bShow)
{
	bShowMotto = bShow;
	if (MottoBox)
	{
		MottoBox->SetVisibility(bShowMotto ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UVelmaraFooterBar::HandleInputMethodChanged(ECommonInputType NewInputType)
{
	RebuildActionPrompts();
}

void UVelmaraFooterBar::RebuildActionPrompts()
{
	if (!ActionsBox || !WidgetTree)
	{
		return;
	}

	ActionsBox->ClearChildren();
	UDataTable* ActionsTable = FVelmaraUITheme::LoadInputActionsTable();

	for (int32 Index = 0; Index < CurrentActions.Num(); ++Index)
	{
		const FVelmaraFooterAction& Action = CurrentActions[Index];

		UHorizontalBox* EntryRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		if (UHorizontalBoxSlot* EntrySlot = ActionsBox->AddChildToHorizontalBox(EntryRow))
		{
			EntrySlot->SetVerticalAlignment(VAlign_Center);
			EntrySlot->SetPadding(FMargin(Index > 0 ? 28.0f : 0.0f, 0.0f, 0.0f, 0.0f));
		}

		if (ActionsTable && !Action.ActionRowName.IsNone())
		{
			USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			IconSize->SetWidthOverride(28.0f);
			IconSize->SetHeightOverride(28.0f);

			UCommonActionWidget* ActionIcon = WidgetTree->ConstructWidget<UCommonActionWidget>(UCommonActionWidget::StaticClass());
			FDataTableRowHandle Handle;
			Handle.DataTable = ActionsTable;
			Handle.RowName = Action.ActionRowName;
			ActionIcon->SetInputAction(Handle);
			IconSize->SetContent(ActionIcon);

			if (UHorizontalBoxSlot* IconSlot = EntryRow->AddChildToHorizontalBox(IconSize))
			{
				IconSlot->SetVerticalAlignment(VAlign_Center);
				IconSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
			}
		}

		UCommonTextBlock* ActionLabel = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass());
		ActionLabel->SetText(Action.Label.ToUpper());
		ActionLabel->SetFont(FVelmaraUITheme::GetDisplayFont(15, 180));

		FLinearColor TextColor = FVelmaraUITheme::Bone();
		if (!Action.bEnabled)
		{
			TextColor = FVelmaraUITheme::Ash(0.55f);
		}
		else if (Action.bHighlighted)
		{
			TextColor = FVelmaraUITheme::BloodBright();
		}
		ActionLabel->SetColorAndOpacity(FSlateColor(TextColor));

		if (UHorizontalBoxSlot* LabelSlot = EntryRow->AddChildToHorizontalBox(ActionLabel))
		{
			LabelSlot->SetVerticalAlignment(VAlign_Center);
		}
	}
}
