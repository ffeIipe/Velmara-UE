#include "UI/Widgets/Menus/VelmaraMainMenuWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Features/SaveSystem/Subsystems/SaveGameSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "SceneEvents/VelmaraGameInstance.h"
#include "UI/Layout/VelmaraPrimaryGameLayout.h"
#include "UI/Style/VelmaraUITheme.h"
#include "UI/Widgets/Common/VelmaraBracketPanel.h"
#include "UI/Widgets/Common/VelmaraButtonBase.h"
#include "UI/Widgets/Common/VelmaraConfirmModalWidget.h"
#include "UI/Widgets/Common/VelmaraFooterBar.h"
#include "UI/Widgets/Menus/VelmaraSaveSlotsWidget.h"
#include "UI/Widgets/Menus/VelmaraSettingsMenuWidget.h"

UVelmaraMainMenuWidget::UVelmaraMainMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = true;
	bSupportsActivationFocus = true;
	bAutoActivate = false;
}

TOptional<FUIInputConfig> UVelmaraMainMenuWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture, false);
}

TSharedRef<SWidget> UVelmaraMainMenuWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildScreenTree();
	}
	return Super::RebuildWidget();
}

void UVelmaraMainMenuWidget::BuildScreenTree()
{
	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("MainMenuRootOverlay"));
	WidgetTree->RootWidget = RootOverlay;

	UScaleBox* BgScale = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("MainMenuBgScale"));
	BgScale->SetStretch(EStretch::ScaleToFill);
	UImage* BgImg = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MainMenuBgImg"));
	BgImg->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::BgMainMenu),
		FVector2D(1920.0f, 1080.0f),
		FLinearColor::White));
	BgScale->SetContent(BgImg);
	if (UOverlaySlot* BgSlot = RootOverlay->AddChildToOverlay(BgScale))
	{
		BgSlot->SetHorizontalAlignment(HAlign_Fill);
		BgSlot->SetVerticalAlignment(VAlign_Fill);
	}

	USizeBox* LeftFadeSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MainMenuLeftFadeSize"));
	LeftFadeSize->SetWidthOverride(860.0f);
	UImage* LeftFade = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MainMenuLeftFade"));
	LeftFade->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	LeftFade->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::GradHFadeLeftToRight),
		FVector2D(860.0f, 1080.0f),
		FVelmaraUITheme::Void(0.94f)));
	LeftFadeSize->SetContent(LeftFade);
	if (UOverlaySlot* FadeSlot = RootOverlay->AddChildToOverlay(LeftFadeSize))
	{
		FadeSlot->SetHorizontalAlignment(HAlign_Left);
		FadeSlot->SetVerticalAlignment(VAlign_Fill);
	}

	USizeBox* LeftSpineSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MainMenuSpineSize"));
	LeftSpineSize->SetWidthOverride(20.0f);
	UImage* LeftSpine = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MainMenuSpine"));
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
		SpineSlot->SetPadding(FMargin(28.0f, 72.0f, 0.0f, 96.0f));
	}

	USizeBox* TopCrestSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MainMenuCrestSize"));
	TopCrestSize->SetWidthOverride(42.0f);
	TopCrestSize->SetHeightOverride(118.0f);
	UImage* TopCrest = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MainMenuCrest"));
	TopCrest->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	TopCrest->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnCrestVelmara),
		FVector2D(42.0f, 118.0f),
		FVelmaraUITheme::BloodBright(0.95f)));
	TopCrestSize->SetContent(TopCrest);
	if (UOverlaySlot* CrestSlot = RootOverlay->AddChildToOverlay(TopCrestSize))
	{
		CrestSlot->SetHorizontalAlignment(HAlign_Right);
		CrestSlot->SetVerticalAlignment(VAlign_Top);
		CrestSlot->SetPadding(FMargin(0.0f, 44.0f, 56.0f, 0.0f));
	}

	UVerticalBox* LeftColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainMenuLeftCol"));
	if (UOverlaySlot* LeftSlot = RootOverlay->AddChildToOverlay(LeftColumn))
	{
		LeftSlot->SetHorizontalAlignment(HAlign_Left);
		LeftSlot->SetVerticalAlignment(VAlign_Center);
		LeftSlot->SetPadding(FMargin(92.0f, 40.0f, 0.0f, 72.0f));
	}

	USizeBox* LogoSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MainMenuLogoSize"));
	LogoSize->SetWidthOverride(440.0f);
	LogoSize->SetHeightOverride(168.0f);
	UScaleBox* LogoScale = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("MainMenuLogoScale"));
	LogoScale->SetStretch(EStretch::ScaleToFit);
	LogoImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MainMenuLogoImg"));
	if (UTexture2D* LogoTex = FVelmaraUITheme::LoadVelmaraLogo())
	{
		LogoImage->SetBrush(FVelmaraUITheme::MakeImageBrush(LogoTex, FVector2D(LogoTex->GetSizeX(), LogoTex->GetSizeY()), FLinearColor::White));
	}
	LogoScale->SetContent(LogoImage);
	LogoSize->SetContent(LogoScale);
	if (UVerticalBoxSlot* LogoSlot = LeftColumn->AddChildToVerticalBox(LogoSize))
	{
		LogoSlot->SetHorizontalAlignment(HAlign_Left);
		LogoSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	USizeBox* SepSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MainMenuSepSize"));
	SepSize->SetWidthOverride(420.0f);
	SepSize->SetHeightOverride(28.0f);
	UImage* SepImg = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MainMenuSepImg"));
	SepImg->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnSepStar),
		FVector2D(420.0f, 28.0f),
		FVelmaraUITheme::Bone(0.85f)));
	SepSize->SetContent(SepImg);
	if (UVerticalBoxSlot* SepSlot = LeftColumn->AddChildToVerticalBox(SepSize))
	{
		SepSlot->SetHorizontalAlignment(HAlign_Left);
		SepSlot->SetPadding(FMargin(10.0f, 0.0f, 0.0f, 28.0f));
	}

	USizeBox* NavSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MainMenuNavSize"));
	NavSize->SetWidthOverride(440.0f);
	NavPanel = WidgetTree->ConstructWidget<UVelmaraBracketPanel>(UVelmaraBracketPanel::StaticClass(), TEXT("MainMenuNavPanel"));
	NavPanel->SetUseMinimalBrackets(true);
	NavPanel->SetBackdropAlpha(0.18f);
	NavPanel->SetShowSignature(false);
	NavPanel->SetContentPadding(FMargin(24.0f, 28.0f, 24.0f, 28.0f));
	NavSize->SetContent(NavPanel);
	if (UVerticalBoxSlot* NavSlot = LeftColumn->AddChildToVerticalBox(NavSize))
	{
		NavSlot->SetHorizontalAlignment(HAlign_Left);
	}

	UVerticalBox* NavList = NavPanel->GetContentBox();

	auto CreateMenuBtn = [&](const FText& Label, void (UVelmaraMainMenuWidget::*Handler)()) -> UVelmaraButtonBase*
	{
		UVelmaraButtonBase* Btn = WidgetTree->ConstructWidget<UVelmaraButtonBase>(UVelmaraButtonBase::StaticClass());
		Btn->SetVariant(EVelmaraButtonVariant::MinimalList);
		Btn->SetFontSize(22);
		Btn->SetButtonMinSize(380.0f, 52.0f);
		Btn->SetButtonText(Label);
		Btn->OnClicked().AddUObject(this, Handler);
		if (UVerticalBoxSlot* BSlot = NavList->AddChildToVerticalBox(Btn))
		{
			BSlot->SetHorizontalAlignment(HAlign_Fill);
			BSlot->SetPadding(FMargin(0.0f, 4.0f));
		}
		return Btn;
	};

	ContinueButton = CreateMenuBtn(NSLOCTEXT("VelmaraUI", "MainContinue", "CONTINUAR"), &UVelmaraMainMenuWidget::HandleContinueClicked);
	NewGameButton = CreateMenuBtn(NSLOCTEXT("VelmaraUI", "MainNewGame", "NUEVA PARTIDA"), &UVelmaraMainMenuWidget::HandleNewGameClicked);
	LoadGameButton = CreateMenuBtn(NSLOCTEXT("VelmaraUI", "MainLoadGame", "CARGAR PARTIDA"), &UVelmaraMainMenuWidget::HandleLoadGameClicked);
	PracticeZoneButton = CreateMenuBtn(NSLOCTEXT("VelmaraUI", "MainPractice", "ZONA DE PRÁCTICA"), &UVelmaraMainMenuWidget::HandlePracticeZoneClicked);
	SettingsButton = CreateMenuBtn(NSLOCTEXT("VelmaraUI", "MainSettings", "AJUSTES"), &UVelmaraMainMenuWidget::HandleSettingsClicked);
	QuitButton = CreateMenuBtn(NSLOCTEXT("VelmaraUI", "MainQuit", "SALIR"), &UVelmaraMainMenuWidget::HandleQuitClicked);

	FooterBar = WidgetTree->ConstructWidget<UVelmaraFooterBar>(UVelmaraFooterBar::StaticClass(), TEXT("MainMenuFooterBar"));
	FooterBar->SetActions({
		FVelmaraFooterAction(TEXT("Confirm"), NSLOCTEXT("VelmaraUI", "FooterSelect", "SELECCIONAR")),
		FVelmaraFooterAction(TEXT("Back"), NSLOCTEXT("VelmaraUI", "FooterQuit", "SALIR"))
	});
	if (UOverlaySlot* FootSlot = RootOverlay->AddChildToOverlay(FooterBar))
	{
		FootSlot->SetHorizontalAlignment(HAlign_Fill);
		FootSlot->SetVerticalAlignment(VAlign_Bottom);
		FootSlot->SetPadding(FMargin(64.0f, 0.0f, 64.0f, 36.0f));
	}

	RefreshContinueVisibility();
}

void UVelmaraMainMenuWidget::RefreshContinueVisibility()
{
	MostRecentSlotIndex = INDEX_NONE;
	if (UGameInstance* GI = GetGameInstance())
	{
		if (USaveGameSubsystem* SaveSubsystem = GI->GetSubsystem<USaveGameSubsystem>())
		{
			MostRecentSlotIndex = SaveSubsystem->GetMostRecentSaveSlot();
		}
	}

	if (ContinueButton)
	{
		ContinueButton->SetVisibility(MostRecentSlotIndex != INDEX_NONE ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UVelmaraMainMenuWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	RefreshContinueVisibility();
	if (UWidget* Target = NativeGetDesiredFocusTarget())
	{
		Target->SetKeyboardFocus();
	}
}

UWidget* UVelmaraMainMenuWidget::NativeGetDesiredFocusTarget() const
{
	if (ContinueButton && MostRecentSlotIndex != INDEX_NONE)
	{
		return ContinueButton;
	}
	return NewGameButton;
}

bool UVelmaraMainMenuWidget::NativeOnHandleBackAction()
{
	HandleQuitClicked();
	return true;
}

FReply UVelmaraMainMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
	{
		HandleQuitClicked();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UVelmaraMainMenuWidget::HandleContinueClicked()
{
	if (MostRecentSlotIndex == INDEX_NONE)
	{
		return;
	}
	if (UVelmaraGameInstance* GI = Cast<UVelmaraGameInstance>(GetGameInstance()))
	{
		GI->LoadGame(MostRecentSlotIndex);
	}
}

void UVelmaraMainMenuWidget::HandleNewGameClicked()
{
	if (UVelmaraPrimaryGameLayout* Layout = UVelmaraPrimaryGameLayout::Get(this))
	{
		Layout->PushMenuWidget<UVelmaraSaveSlotsWidget>(
			UVelmaraSaveSlotsWidget::StaticClass(),
			[](UVelmaraSaveSlotsWidget& SlotsWidget)
			{
				SlotsWidget.SetSlotsMode(ESaveSlotsMode::NewGame);
			});
	}
}

void UVelmaraMainMenuWidget::HandleLoadGameClicked()
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

void UVelmaraMainMenuWidget::HandlePracticeZoneClicked()
{
	if (UVelmaraGameInstance* GI = Cast<UVelmaraGameInstance>(GetGameInstance()))
	{
		GI->ShowLoadingScreen();
	}
	UGameplayStatics::OpenLevel(this, FName(TEXT("Felipe")));
}

void UVelmaraMainMenuWidget::HandleSettingsClicked()
{
	if (UVelmaraPrimaryGameLayout* Layout = UVelmaraPrimaryGameLayout::Get(this))
	{
		Layout->PushMenuWidget<UVelmaraSettingsMenuWidget>(UVelmaraSettingsMenuWidget::StaticClass());
	}
}

void UVelmaraMainMenuWidget::HandleQuitClicked()
{
	TWeakObjectPtr<UVelmaraMainMenuWidget> WeakThis(this);
	UVelmaraConfirmModalWidget::ShowConfirmModal(
		this,
		NSLOCTEXT("VelmaraUI", "QuitModalTitle", "¿SALIR DE VELMARA?"),
		NSLOCTEXT("VelmaraUI", "QuitModalBody", "¿Deseas cerrar el juego y regresar al escritorio?"),
		NSLOCTEXT("VelmaraUI", "QuitModalConfirm", "SALIR"),
		NSLOCTEXT("VelmaraUI", "ModalCancel", "CANCELAR"),
		[WeakThis]()
		{
			if (WeakThis.IsValid())
			{
				UKismetSystemLibrary::QuitGame(WeakThis.Get(), WeakThis->GetOwningPlayer(), EQuitPreference::Quit, false);
			}
		});
}

void UVelmaraTitleScreenBridgeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (UVelmaraPrimaryGameLayout* Layout = UVelmaraPrimaryGameLayout::GetOrCreate(PC))
		{
			if (!Layout->HasActiveMenu())
			{
				Layout->PushMenuWidget<UVelmaraMainMenuWidget>(UVelmaraMainMenuWidget::StaticClass());
			}
		}
	}

	SetVisibility(ESlateVisibility::Collapsed);
	RemoveFromParent();
}
