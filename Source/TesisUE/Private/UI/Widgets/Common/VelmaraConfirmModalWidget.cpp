#include "UI/Widgets/Common/VelmaraConfirmModalWidget.h"
#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/BackgroundBlur.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UI/Layout/VelmaraPrimaryGameLayout.h"
#include "UI/Style/VelmaraUITheme.h"
#include "UI/Widgets/Common/VelmaraBracketPanel.h"
#include "UI/Widgets/Common/VelmaraButtonBase.h"
#include "UI/Widgets/Common/VelmaraFooterBar.h"

UVelmaraConfirmModalWidget::UVelmaraConfirmModalWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = true;
	bIsModal = true;
	bSupportsActivationFocus = true;
	bAutoActivate = false;
}

UVelmaraConfirmModalWidget* UVelmaraConfirmModalWidget::ShowConfirmModal(
	UObject* WorldContext,
	const FText& Title,
	const FText& Message,
	const FText& ConfirmLabel,
	const FText& CancelLabel,
	TFunction<void()> OnConfirm,
	TFunction<void()> OnCancel)
{
	if (!WorldContext)
	{
		return nullptr;
	}

	if (UVelmaraPrimaryGameLayout* Layout = UVelmaraPrimaryGameLayout::Get(WorldContext))
	{
		return Layout->PushModalWidget<UVelmaraConfirmModalWidget>(
			StaticClass(),
			[&](UVelmaraConfirmModalWidget& Modal)
			{
				Modal.ConfigureModal(Title, Message, ConfirmLabel, CancelLabel, MoveTemp(OnConfirm), MoveTemp(OnCancel));
			});
	}

	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	APlayerController* PC = World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
	if (!PC)
	{
		return nullptr;
	}

	UVelmaraConfirmModalWidget* Modal = CreateWidget<UVelmaraConfirmModalWidget>(PC, StaticClass());
	if (Modal)
	{
		Modal->bAddedDirectlyToViewport = true;
		Modal->ConfigureModal(Title, Message, ConfirmLabel, CancelLabel, MoveTemp(OnConfirm), MoveTemp(OnCancel));
		Modal->AddToViewport(50);
		Modal->ActivateWidget();
	}
	return Modal;
}

void UVelmaraConfirmModalWidget::ConfigureModal(
	const FText& Title,
	const FText& Message,
	const FText& ConfirmLabel,
	const FText& CancelLabel,
	TFunction<void()> InOnConfirm,
	TFunction<void()> InOnCancel)
{
	PendingTitle = Title;
	PendingMessage = Message;
	PendingConfirmLabel = ConfirmLabel.IsEmpty() ? NSLOCTEXT("VelmaraUI", "ModalConfirm", "CONFIRMAR") : ConfirmLabel;
	PendingCancelLabel = CancelLabel.IsEmpty() ? NSLOCTEXT("VelmaraUI", "ModalCancel", "CANCELAR") : CancelLabel;
	ConfirmCallback = MoveTemp(InOnConfirm);
	CancelCallback = MoveTemp(InOnCancel);
	bClosed = false;

	if (TitleText)
	{
		TitleText->SetText(PendingTitle.ToUpper());
	}
	if (MessageText)
	{
		MessageText->SetText(PendingMessage);
	}
	if (CancelButton)
	{
		CancelButton->SetButtonText(PendingCancelLabel);
	}
	if (ConfirmButton)
	{
		ConfirmButton->SetButtonText(PendingConfirmLabel);
	}
}

TOptional<FUIInputConfig> UVelmaraConfirmModalWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture, false);
}

TSharedRef<SWidget> UVelmaraConfirmModalWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildModalTree();
	}
	return Super::RebuildWidget();
}

void UVelmaraConfirmModalWidget::BuildModalTree()
{
	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("ConfirmModalRoot"));
	RootOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = RootOverlay;

	UBackgroundBlur* Blur = WidgetTree->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass(), TEXT("ModalBlur"));
	Blur->SetBlurStrength(10.0f);
	Blur->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UOverlaySlot* BlurSlot = RootOverlay->AddChildToOverlay(Blur))
	{
		BlurSlot->SetHorizontalAlignment(HAlign_Fill);
		BlurSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UBorder* DimOverlay = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ModalDim"));
	DimOverlay->SetBrush(FVelmaraUITheme::MakeSolidBrush(FVelmaraUITheme::Void(0.75f)));
	if (UOverlaySlot* DimSlot = RootOverlay->AddChildToOverlay(DimOverlay))
	{
		DimSlot->SetHorizontalAlignment(HAlign_Fill);
		DimSlot->SetVerticalAlignment(VAlign_Fill);
	}

	USizeBox* DialogSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ModalDialogSize"));
	DialogSize->SetWidthOverride(760.0f);
	DialogSize->SetMinDesiredHeight(310.0f);
	if (UOverlaySlot* DialogSlot = RootOverlay->AddChildToOverlay(DialogSize))
	{
		DialogSlot->SetHorizontalAlignment(HAlign_Center);
		DialogSlot->SetVerticalAlignment(VAlign_Center);
	}

	DialogPanel = WidgetTree->ConstructWidget<UVelmaraBracketPanel>(UVelmaraBracketPanel::StaticClass(), TEXT("ModalBracketPanel"));
	DialogPanel->SetUseMinimalBrackets(true);
	DialogPanel->SetBackdropAlpha(0.92f);
	DialogPanel->SetShowSignature(false);
	DialogPanel->SetShowTopStar(true);
	DialogPanel->SetContentPadding(FMargin(40.0f, 34.0f, 40.0f, 32.0f));
	DialogSize->SetContent(DialogPanel);

	UVerticalBox* ContentBox = DialogPanel->GetContentBox();

	TitleText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("ModalTitleText"));
	TitleText->SetText(PendingTitle.ToUpper());
	TitleText->SetFont(FVelmaraUITheme::GetDisplayFont(26, 220));
	TitleText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
	TitleText->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* TitleSlot = ContentBox->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
		TitleSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 12.0f));
	}

	USizeBox* SepSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ModalSepSize"));
	SepSize->SetWidthOverride(380.0f);
	SepSize->SetHeightOverride(18.0f);
	UImage* SepImg = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ModalSepImg"));
	SepImg->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnSepDiamond),
		FVector2D(380.0f, 18.0f),
		FVelmaraUITheme::BloodBright()));
	SepSize->SetContent(SepImg);
	if (UVerticalBoxSlot* SepSlot = ContentBox->AddChildToVerticalBox(SepSize))
	{
		SepSlot->SetHorizontalAlignment(HAlign_Center);
		SepSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
	}

	MessageText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("ModalMessageText"));
	MessageText->SetText(PendingMessage);
	MessageText->SetFont(FVelmaraUITheme::GetBodyFont(19, 0));
	MessageText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted()));
	MessageText->SetJustification(ETextJustify::Center);
	MessageText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* MsgSlot = ContentBox->AddChildToVerticalBox(MessageText))
	{
		MsgSlot->SetHorizontalAlignment(HAlign_Fill);
		MsgSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		MsgSlot->SetPadding(FMargin(8.0f, 0.0f, 8.0f, 26.0f));
	}

	UHorizontalBox* ButtonsRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ModalButtonsRow"));
	if (UVerticalBoxSlot* BtnsSlot = ContentBox->AddChildToVerticalBox(ButtonsRow))
	{
		BtnsSlot->SetHorizontalAlignment(HAlign_Fill);
		BtnsSlot->SetVerticalAlignment(VAlign_Bottom);
	}

	CancelButton = WidgetTree->ConstructWidget<UVelmaraButtonBase>(UVelmaraButtonBase::StaticClass(), TEXT("ModalCancelBtn"));
	CancelButton->SetVariant(EVelmaraButtonVariant::HexAction);
	CancelButton->SetButtonMinSize(0.0f, 52.0f);
	CancelButton->SetFontSize(17);
	CancelButton->SetButtonText(PendingCancelLabel.IsEmpty() ? NSLOCTEXT("VelmaraUI", "ModalCancel", "CANCELAR") : PendingCancelLabel);
	CancelButton->OnClicked().AddUObject(this, &UVelmaraConfirmModalWidget::HandleCancelClicked);
	if (UHorizontalBoxSlot* CancelSlot = ButtonsRow->AddChildToHorizontalBox(CancelButton))
	{
		CancelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		CancelSlot->SetHorizontalAlignment(HAlign_Fill);
		CancelSlot->SetPadding(FMargin(0.0f, 0.0f, 20.0f, 0.0f));
	}

	ConfirmButton = WidgetTree->ConstructWidget<UVelmaraButtonBase>(UVelmaraButtonBase::StaticClass(), TEXT("ModalConfirmBtn"));
	ConfirmButton->SetVariant(EVelmaraButtonVariant::HexAction);
	ConfirmButton->SetButtonMinSize(0.0f, 52.0f);
	ConfirmButton->SetFontSize(17);
	ConfirmButton->SetButtonText(PendingConfirmLabel.IsEmpty() ? NSLOCTEXT("VelmaraUI", "ModalConfirm", "CONFIRMAR") : PendingConfirmLabel);
	ConfirmButton->OnClicked().AddUObject(this, &UVelmaraConfirmModalWidget::HandleConfirmClicked);
	if (UHorizontalBoxSlot* ConfirmSlot = ButtonsRow->AddChildToHorizontalBox(ConfirmButton))
	{
		ConfirmSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ConfirmSlot->SetHorizontalAlignment(HAlign_Fill);
	}

	FooterBar = WidgetTree->ConstructWidget<UVelmaraFooterBar>(UVelmaraFooterBar::StaticClass(), TEXT("ModalFooterBar"));
	FooterBar->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	FooterBar->SetShowMotto(false);
	FooterBar->SetActions({
		FVelmaraFooterAction(TEXT("Confirm"), NSLOCTEXT("VelmaraUI", "FooterAccept", "ACEPTAR")),
		FVelmaraFooterAction(TEXT("Back"), NSLOCTEXT("VelmaraUI", "FooterCancel", "CANCELAR"))
	});
	if (UOverlaySlot* FooterSlot = RootOverlay->AddChildToOverlay(FooterBar))
	{
		FooterSlot->SetHorizontalAlignment(HAlign_Fill);
		FooterSlot->SetVerticalAlignment(VAlign_Bottom);
		FooterSlot->SetPadding(FMargin(64.0f, 0.0f, 64.0f, 36.0f));
	}
}

UWidget* UVelmaraConfirmModalWidget::NativeGetDesiredFocusTarget() const
{
	return CancelButton;
}

void UVelmaraConfirmModalWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	bClosed = false;
	if (CancelButton)
	{
		CancelButton->SetKeyboardFocus();
	}
}

bool UVelmaraConfirmModalWidget::NativeOnHandleBackAction()
{
	CloseModal(false);
	return true;
}

FReply UVelmaraConfirmModalWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
	{
		CloseModal(false);
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UVelmaraConfirmModalWidget::HandleConfirmClicked()
{
	CloseModal(true);
}

void UVelmaraConfirmModalWidget::HandleCancelClicked()
{
	CloseModal(false);
}

void UVelmaraConfirmModalWidget::CloseModal(bool bConfirmed)
{
	if (bClosed)
	{
		return;
	}
	bClosed = true;

	TFunction<void()> CallbackToRun = bConfirmed ? MoveTemp(ConfirmCallback) : MoveTemp(CancelCallback);
	ConfirmCallback = nullptr;
	CancelCallback = nullptr;

	DeactivateWidget();
	if (bAddedDirectlyToViewport)
	{
		RemoveFromParent();
	}

	if (CallbackToRun)
	{
		CallbackToRun();
	}
}
