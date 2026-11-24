#include "UI/Widgets/Menus/VelmaraPauseMenuWidget.h"
#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/BackgroundBlur.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressionComponent.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Features/InventorySystem/Core/InventoryComponent.h"
#include "Features/SaveSystem/Subsystems/SaveGameSubsystem.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Player/CustomPlayerController.h"
#include "SceneEvents/VelmaraGameInstance.h"
#include "UI/Layout/VelmaraPrimaryGameLayout.h"
#include "UI/Style/VelmaraUITheme.h"
#include "UI/Widgets/Common/VelmaraBracketPanel.h"
#include "UI/Widgets/Common/VelmaraButtonBase.h"
#include "UI/Widgets/Common/VelmaraConfirmModalWidget.h"
#include "UI/Widgets/Common/VelmaraFooterBar.h"
#include "UI/Widgets/Menus/VelmaraSaveSlotsWidget.h"
#include "UI/Widgets/Menus/VelmaraSettingsMenuWidget.h"

UVelmaraPauseMenuWidget::UVelmaraPauseMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = true;
	bSupportsActivationFocus = true;
	bAutoActivate = false;
}

TOptional<FUIInputConfig> UVelmaraPauseMenuWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture, false);
}

TSharedRef<SWidget> UVelmaraPauseMenuWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildScreenTree();
	}
	return Super::RebuildWidget();
}

void UVelmaraPauseMenuWidget::BuildScreenTree()
{
	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("PauseRootOverlay"));
	WidgetTree->RootWidget = RootOverlay;

	UBackgroundBlur* Blur = WidgetTree->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass(), TEXT("PauseBlur"));
	Blur->SetBlurStrength(14.0f);
	if (UOverlaySlot* BlurSlot = RootOverlay->AddChildToOverlay(Blur))
	{
		BlurSlot->SetHorizontalAlignment(HAlign_Fill);
		BlurSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UBorder* DimBg = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PauseDimBg"));
	DimBg->SetBrush(FVelmaraUITheme::MakeSolidBrush(FVelmaraUITheme::Void(0.70f)));
	if (UOverlaySlot* DimSlot = RootOverlay->AddChildToOverlay(DimBg))
	{
		DimSlot->SetHorizontalAlignment(HAlign_Fill);
		DimSlot->SetVerticalAlignment(VAlign_Fill);
	}

	USizeBox* LeftSpineSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PauseLeftSpineSize"));
	LeftSpineSize->SetWidthOverride(20.0f);
	UImage* LeftSpine = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PauseLeftSpine"));
	LeftSpine->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	LeftSpine->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnSidebarLineV),
		FVector2D(20.0f, 760.0f),
		FVelmaraUITheme::Bone(0.75f)));
	LeftSpineSize->SetContent(LeftSpine);
	if (UOverlaySlot* SpineSlot = RootOverlay->AddChildToOverlay(LeftSpineSize))
	{
		SpineSlot->SetHorizontalAlignment(HAlign_Left);
		SpineSlot->SetVerticalAlignment(VAlign_Fill);
		SpineSlot->SetPadding(FMargin(44.0f, 76.0f, 0.0f, 96.0f));
	}

	USizeBox* TopCrestSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PauseCrestSize"));
	TopCrestSize->SetWidthOverride(38.0f);
	TopCrestSize->SetHeightOverride(108.0f);
	UImage* TopCrest = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PauseCrest"));
	TopCrest->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	TopCrest->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnCrestVelmara),
		FVector2D(38.0f, 108.0f),
		FVelmaraUITheme::BloodBright(0.92f)));
	TopCrestSize->SetContent(TopCrest);
	if (UOverlaySlot* CrestSlot = RootOverlay->AddChildToOverlay(TopCrestSize))
	{
		CrestSlot->SetHorizontalAlignment(HAlign_Right);
		CrestSlot->SetVerticalAlignment(VAlign_Top);
		CrestSlot->SetPadding(FMargin(0.0f, 44.0f, 60.0f, 0.0f));
	}

	UVerticalBox* LeftCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PauseLeftCol"));
	if (UOverlaySlot* LeftSlot = RootOverlay->AddChildToOverlay(LeftCol))
	{
		LeftSlot->SetHorizontalAlignment(HAlign_Left);
		LeftSlot->SetVerticalAlignment(VAlign_Center);
		LeftSlot->SetPadding(FMargin(96.0f, 0.0f, 0.0f, 32.0f));
	}

	UCommonTextBlock* TitleText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("PauseTitleText"));
	TitleText->SetText(NSLOCTEXT("VelmaraUI", "PauseHeader", "P A U S A"));
	TitleText->SetFont(FVelmaraUITheme::GetDisplayFont(42, 260));
	TitleText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
	if (UVerticalBoxSlot* TSlot = LeftCol->AddChildToVerticalBox(TitleText))
	{
		TSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	USizeBox* SepSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PauseSepSize"));
	SepSize->SetWidthOverride(440.0f);
	SepSize->SetHeightOverride(24.0f);
	UImage* SepImg = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PauseSepImg"));
	SepImg->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnSepStar),
		FVector2D(440.0f, 24.0f),
		FVelmaraUITheme::Bone(0.85f)));
	SepSize->SetContent(SepImg);
	if (UVerticalBoxSlot* SepSlot = LeftCol->AddChildToVerticalBox(SepSize))
	{
		SepSlot->SetHorizontalAlignment(HAlign_Left);
		SepSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 26.0f));
	}

	auto AddPauseBtn = [&](const FText& Label, void (UVelmaraPauseMenuWidget::*Handler)()) -> UVelmaraButtonBase*
	{
		UVelmaraButtonBase* Btn = WidgetTree->ConstructWidget<UVelmaraButtonBase>(UVelmaraButtonBase::StaticClass());
		Btn->SetVariant(EVelmaraButtonVariant::FramedRow);
		Btn->SetFontSize(20);
		Btn->SetButtonMinSize(440.0f, 54.0f);
		Btn->SetButtonText(Label);
		Btn->OnClicked().AddUObject(this, Handler);
		if (UVerticalBoxSlot* BSlot = LeftCol->AddChildToVerticalBox(Btn))
		{
			BSlot->SetHorizontalAlignment(HAlign_Fill);
			BSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
		}
		return Btn;
	};

	ResumeButton = AddPauseBtn(NSLOCTEXT("VelmaraUI", "PauseResume", "REANUDAR"), &UVelmaraPauseMenuWidget::HandleResumeClicked);
	InventoryButton = AddPauseBtn(NSLOCTEXT("VelmaraUI", "PauseInventory", "INVENTARIO"), &UVelmaraPauseMenuWidget::HandleInventoryClicked);
	SettingsButton = AddPauseBtn(NSLOCTEXT("VelmaraUI", "PauseSettings", "AJUSTES"), &UVelmaraPauseMenuWidget::HandleSettingsClicked);
	SaveGameButton = AddPauseBtn(NSLOCTEXT("VelmaraUI", "PauseSave", "GUARDAR PARTIDA"), &UVelmaraPauseMenuWidget::HandleSaveGameClicked);
	LoadGameButton = AddPauseBtn(NSLOCTEXT("VelmaraUI", "PauseLoad", "CARGAR PARTIDA"), &UVelmaraPauseMenuWidget::HandleLoadGameClicked);
	MainMenuButton = AddPauseBtn(NSLOCTEXT("VelmaraUI", "PauseMainMenu", "MENÚ PRINCIPAL"), &UVelmaraPauseMenuWidget::HandleMainMenuClicked);
	QuitDesktopButton = AddPauseBtn(NSLOCTEXT("VelmaraUI", "PauseQuit", "SALIR AL ESCRITORIO"), &UVelmaraPauseMenuWidget::HandleQuitDesktopClicked);

	USizeBox* RightPanelSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PauseStatusSize"));
	RightPanelSize->SetWidthOverride(420.0f);
	RightPanelSize->SetMinDesiredHeight(360.0f);
	StatusPanel = WidgetTree->ConstructWidget<UVelmaraBracketPanel>(UVelmaraBracketPanel::StaticClass(), TEXT("PauseStatusPanel"));
	StatusPanel->SetUseMinimalBrackets(true);
	StatusPanel->SetBackdropAlpha(0.70f);
	StatusPanel->SetShowSignature(true);
	RightPanelSize->SetContent(StatusPanel);
	if (UOverlaySlot* RightSlot = RootOverlay->AddChildToOverlay(RightPanelSize))
	{
		RightSlot->SetHorizontalAlignment(HAlign_Right);
		RightSlot->SetVerticalAlignment(VAlign_Center);
		RightSlot->SetPadding(FMargin(0.0f, 0.0f, 96.0f, 32.0f));
	}

	UVerticalBox* StatusContent = StatusPanel->GetContentBox();

	ZoneTitleText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("PauseZoneTitle"));
	ZoneTitleText->SetFont(FVelmaraUITheme::GetDisplayFont(22, 200));
	ZoneTitleText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
	ZoneTitleText->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* ZSlot = StatusContent->AddChildToVerticalBox(ZoneTitleText))
	{
		ZSlot->SetHorizontalAlignment(HAlign_Center);
		ZSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	USizeBox* StatSepSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PauseStatSepSize"));
	StatSepSize->SetHeightOverride(16.0f);
	UImage* StatSep = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PauseStatSep"));
	StatSep->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnSepDiamond),
		FVector2D(320.0f, 16.0f),
		FVelmaraUITheme::BoneMuted(0.75f)));
	StatSepSize->SetContent(StatSep);
	if (UVerticalBoxSlot* SS = StatusContent->AddChildToVerticalBox(StatSepSize))
	{
		SS->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f));
	}

	StatusBodyText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("PauseStatusBody"));
	StatusBodyText->SetFont(FVelmaraUITheme::GetBodyFont(18, 0));
	StatusBodyText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted()));
	StatusBodyText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* BodyS = StatusContent->AddChildToVerticalBox(StatusBodyText))
	{
		BodyS->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 22.0f));
	}

	UHorizontalBox* BloodRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PauseBloodRow"));
	if (UVerticalBoxSlot* BRSlot = StatusContent->AddChildToVerticalBox(BloodRow))
	{
		BRSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 24.0f));
	}

	UCommonTextBlock* BloodLabel = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("PauseBloodLabel"));
	BloodLabel->SetText(NSLOCTEXT("VelmaraUI", "PauseBloodLabel", "SANGRE: "));
	BloodLabel->SetFont(FVelmaraUITheme::GetDisplayFont(17, 170));
	BloodLabel->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted()));
	if (UHorizontalBoxSlot* BLSlot = BloodRow->AddChildToHorizontalBox(BloodLabel))
	{
		BLSlot->SetVerticalAlignment(VAlign_Center);
		BLSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
	}

	USizeBox* BloodIconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PauseBloodIconSize"));
	BloodIconSize->SetWidthOverride(18.0f);
	BloodIconSize->SetHeightOverride(18.0f);
	UImage* BloodIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PauseBloodIcon"));
	BloodIcon->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::IconStatBlood),
		FVector2D(18.0f, 18.0f),
		FVelmaraUITheme::BloodBright()));
	BloodIconSize->SetContent(BloodIcon);
	if (UHorizontalBoxSlot* BISlot = BloodRow->AddChildToHorizontalBox(BloodIconSize))
	{
		BISlot->SetVerticalAlignment(VAlign_Center);
		BISlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
	}

	BloodValueText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("PauseBloodValue"));
	BloodValueText->SetFont(FVelmaraUITheme::GetDisplayFont(20, 140));
	BloodValueText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
	if (UHorizontalBoxSlot* BVSlot = BloodRow->AddChildToHorizontalBox(BloodValueText))
	{
		BVSlot->SetVerticalAlignment(VAlign_Center);
	}

	USpacer* StatusSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("PauseStatusSpacer"));
	if (UVerticalBoxSlot* SpSlot = StatusContent->AddChildToVerticalBox(StatusSpacer))
	{
		SpSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UCommonTextBlock* QuoteText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("PauseQuoteText"));
	QuoteText->SetText(NSLOCTEXT("VelmaraUI", "PauseQuote", "\"Aún en la penumbra, la verdad observa.\""));
	QuoteText->SetFont(FVelmaraUITheme::GetItalicFont(17, 10));
	QuoteText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted(0.75f)));
	QuoteText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* QSlot = StatusContent->AddChildToVerticalBox(QuoteText))
	{
		QSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 28.0f));
	}

	FooterBar = WidgetTree->ConstructWidget<UVelmaraFooterBar>(UVelmaraFooterBar::StaticClass(), TEXT("PauseFooterBar"));
	FooterBar->SetActions({
		FVelmaraFooterAction(TEXT("Confirm"), NSLOCTEXT("VelmaraUI", "FooterSelect", "SELECCIONAR")),
		FVelmaraFooterAction(TEXT("Back"), NSLOCTEXT("VelmaraUI", "FooterResume", "REANUDAR"))
	});
	if (UOverlaySlot* FootSlot = RootOverlay->AddChildToOverlay(FooterBar))
	{
		FootSlot->SetHorizontalAlignment(HAlign_Fill);
		FootSlot->SetVerticalAlignment(VAlign_Bottom);
		FootSlot->SetPadding(FMargin(64.0f, 0.0f, 64.0f, 36.0f));
	}

	RefreshStatusPanel();
}

void UVelmaraPauseMenuWidget::RefreshStatusPanel()
{
	const FString LevelName = UGameplayStatics::GetCurrentLevelName(this, true);
	if (ZoneTitleText)
	{
		ZoneTitleText->SetText(FVelmaraUITheme::FormatLevelDisplayName(LevelName));
	}

	int32 BloodAmount = 0;
	if (APlayerController* PC = GetOwningPlayer())
	{
		APawn* TargetPawn = PC->GetPawn();
		if (ACustomPlayerController* CustomPC = Cast<ACustomPlayerController>(PC))
		{
			if (CustomPC->GetOriginalBody())
			{
				TargetPawn = CustomPC->GetOriginalBody();
			}
		}
		if (TargetPawn)
		{
			if (const UProgressionComponent* Progression = TargetPawn->FindComponentByClass<UProgressionComponent>())
			{
				BloodAmount = Progression->GetAvailableCurrency();
			}
		}
	}

	if (BloodValueText)
	{
		BloodValueText->SetText(FVelmaraUITheme::FormatGroupedNumber(BloodAmount));
	}

	if (StatusBodyText)
	{
		int32 ActiveSlot = 0;
		if (const UVelmaraGameInstance* GI = Cast<UVelmaraGameInstance>(GetGameInstance()))
		{
			ActiveSlot = FMath::Clamp(GI->ActiveSaveSlotIndex, 0, 2);
		}
		StatusBodyText->SetText(FText::Format(
			NSLOCTEXT("VelmaraUI", "PauseStatusFmt", "Ranura activa: {0}\n\nExplora la catedral, evita la luz solar directa y fortalece tu sangre en el Altar."),
			FText::FromString(FVelmaraUITheme::FormatSlotRomanNumeral(ActiveSlot))));
	}
}

void UVelmaraPauseMenuWidget::ApplyPauseState(bool bPaused)
{
	UGameplayStatics::SetGamePaused(GetWorld(), bPaused);
	if (UVelmaraGameInstance* GI = Cast<UVelmaraGameInstance>(GetGameInstance()))
	{
		GI->SetPauseAudioMixActive(bPaused);
	}
	if (ACustomPlayerController* CustomPC = Cast<ACustomPlayerController>(GetOwningPlayer()))
	{
		CustomPC->SetPauseAudioMixState(bPaused);
	}
}

void UVelmaraPauseMenuWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	ApplyPauseState(true);
	RefreshStatusPanel();
	if (ResumeButton)
	{
		ResumeButton->SetKeyboardFocus();
	}
}

void UVelmaraPauseMenuWidget::NativeOnDeactivated()
{
	Super::NativeOnDeactivated();

	UVelmaraPrimaryGameLayout* Layout = UVelmaraPrimaryGameLayout::Get(this);
	const bool bStillHasMenu = Layout && Layout->HasActiveMenuOrModal();
	if (!bStillHasMenu)
	{
		ApplyPauseState(false);
	}
}

UWidget* UVelmaraPauseMenuWidget::NativeGetDesiredFocusTarget() const
{
	return ResumeButton;
}

bool UVelmaraPauseMenuWidget::NativeOnHandleBackAction()
{
	DeactivateWidget();
	return true;
}

FReply UVelmaraPauseMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::Gamepad_Special_Right)
	{
		DeactivateWidget();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UVelmaraPauseMenuWidget::HandleResumeClicked()
{
	DeactivateWidget();
}

void UVelmaraPauseMenuWidget::HandleInventoryClicked()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}
	APawn* TargetPawn = PC->GetPawn();
	if (ACustomPlayerController* CustomPC = Cast<ACustomPlayerController>(PC))
	{
		if (CustomPC->GetOriginalBody())
		{
			TargetPawn = CustomPC->GetOriginalBody();
		}
	}
	if (TargetPawn)
	{
		if (UInventoryComponent* InvComp = TargetPawn->FindComponentByClass<UInventoryComponent>())
		{
			DeactivateWidget();
			InvComp->ShowInventory();
		}
	}
}

void UVelmaraPauseMenuWidget::HandleSettingsClicked()
{
	if (UVelmaraPrimaryGameLayout* Layout = UVelmaraPrimaryGameLayout::Get(this))
	{
		Layout->PushMenuWidget<UVelmaraSettingsMenuWidget>(UVelmaraSettingsMenuWidget::StaticClass());
	}
}

void UVelmaraPauseMenuWidget::HandleSaveGameClicked()
{
	if (UVelmaraGameInstance* GI = Cast<UVelmaraGameInstance>(GetGameInstance()))
	{
		const int32 SlotToSave = FMath::Clamp(GI->ActiveSaveSlotIndex, 0, 2);
		GI->SaveGame(SlotToSave);
		if (UVelmaraPrimaryGameLayout* Layout = UVelmaraPrimaryGameLayout::Get(this))
		{
			Layout->ShowTransientNotification(NSLOCTEXT("VelmaraUI", "NotifGameSaved", "PARTIDA GUARDADA"));
		}
		RefreshStatusPanel();
	}
}

void UVelmaraPauseMenuWidget::HandleLoadGameClicked()
{
	if (UVelmaraPrimaryGameLayout* Layout = UVelmaraPrimaryGameLayout::Get(this))
	{
		Layout->PushMenuWidget<UVelmaraSaveSlotsWidget>(
			UVelmaraSaveSlotsWidget::StaticClass(),
			[](UVelmaraSaveSlotsWidget& SlotsWidget)
			{
				SlotsWidget.SetSlotsMode(ESaveSlotsMode::LoadGame);
			});
	}
}

void UVelmaraPauseMenuWidget::HandleMainMenuClicked()
{
	TWeakObjectPtr<UVelmaraPauseMenuWidget> WeakThis(this);
	UVelmaraConfirmModalWidget::ShowConfirmModal(
		this,
		NSLOCTEXT("VelmaraUI", "PauseMainMenuTitle", "¿VOLVER AL MENÚ PRINCIPAL?"),
		NSLOCTEXT("VelmaraUI", "PauseMainMenuBody", "Se perderá todo el progreso que no haya sido guardado."),
		NSLOCTEXT("VelmaraUI", "PauseMainMenuConfirm", "VOLVER AL MENÚ"),
		NSLOCTEXT("VelmaraUI", "ModalCancel", "CANCELAR"),
		[WeakThis]()
		{
			if (!WeakThis.IsValid())
			{
				return;
			}
			WeakThis->ApplyPauseState(false);
			if (UVelmaraGameInstance* GI = Cast<UVelmaraGameInstance>(WeakThis->GetGameInstance()))
			{
				GI->ShowLoadingScreen();
			}
			UGameplayStatics::OpenLevel(WeakThis.Get(), FName(TEXT("MainMenu")));
		});
}

void UVelmaraPauseMenuWidget::HandleQuitDesktopClicked()
{
	TWeakObjectPtr<UVelmaraPauseMenuWidget> WeakThis(this);
	UVelmaraConfirmModalWidget::ShowConfirmModal(
		this,
		NSLOCTEXT("VelmaraUI", "PauseQuitTitle", "¿SALIR AL ESCRITORIO?"),
		NSLOCTEXT("VelmaraUI", "PauseQuitBody", "Se cerrará Velmara y se perderá el progreso no guardado."),
		NSLOCTEXT("VelmaraUI", "PauseQuitConfirm", "SALIR"),
		NSLOCTEXT("VelmaraUI", "ModalCancel", "CANCELAR"),
		[WeakThis]()
		{
			if (WeakThis.IsValid())
			{
				UKismetSystemLibrary::QuitGame(WeakThis.Get(), WeakThis->GetOwningPlayer(), EQuitPreference::Quit, false);
			}
		});
}
