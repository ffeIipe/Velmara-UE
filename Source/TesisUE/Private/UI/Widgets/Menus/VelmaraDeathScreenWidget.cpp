#include "UI/Widgets/Menus/VelmaraDeathScreenWidget.h"
#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Features/SaveSystem/Subsystems/SaveGameSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "SceneEvents/VelmaraGameInstance.h"
#include "UI/Style/VelmaraUITheme.h"
#include "UI/Widgets/Common/VelmaraButtonBase.h"
#include "UI/Widgets/Common/VelmaraFooterBar.h"

UVelmaraDeathScreenWidget::UVelmaraDeathScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = false;
	bSupportsActivationFocus = true;
	bAutoActivate = true;
}

TOptional<FUIInputConfig> UVelmaraDeathScreenWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture, false);
}

TSharedRef<SWidget> UVelmaraDeathScreenWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildScreenTree();
	}
	return Super::RebuildWidget();
}

void UVelmaraDeathScreenWidget::BuildScreenTree()
{
	FadeRoot = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("DeathScreenRoot"));
	FadeRoot->SetRenderOpacity(0.0f);
	WidgetTree->RootWidget = FadeRoot;

	UBorder* VoidBackdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DeathVoidBg"));
	VoidBackdrop->SetBrush(FVelmaraUITheme::MakeSolidBrush(FVelmaraUITheme::Void(0.92f)));
	if (UOverlaySlot* BgSlot = FadeRoot->AddChildToOverlay(VoidBackdrop))
	{
		BgSlot->SetHorizontalAlignment(HAlign_Fill);
		BgSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UImage* BloodVignette = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("DeathBloodVignette"));
	BloodVignette->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	BloodVignette->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::GradVignetteRadial),
		FVector2D(512.0f, 512.0f),
		FVelmaraUITheme::BloodDried(0.95f)));
	if (UOverlaySlot* VigSlot = FadeRoot->AddChildToOverlay(BloodVignette))
	{
		VigSlot->SetHorizontalAlignment(HAlign_Fill);
		VigSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UVerticalBox* CenterVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DeathCenterVBox"));
	if (UOverlaySlot* CenterSlot = FadeRoot->AddChildToOverlay(CenterVBox))
	{
		CenterSlot->SetHorizontalAlignment(HAlign_Center);
		CenterSlot->SetVerticalAlignment(VAlign_Center);
	}

	USizeBox* TopSepSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DeathTopSepSize"));
	TopSepSize->SetWidthOverride(640.0f);
	TopSepSize->SetHeightOverride(30.0f);
	UImage* TopSep = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("DeathTopSep"));
	TopSep->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnSepStar),
		FVector2D(640.0f, 30.0f),
		FVelmaraUITheme::Bone(0.75f)));
	TopSepSize->SetContent(TopSep);
	if (UVerticalBoxSlot* TSepSlot = CenterVBox->AddChildToVerticalBox(TopSepSize))
	{
		TSepSlot->SetHorizontalAlignment(HAlign_Center);
		TSepSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	UOverlay* BannerOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("DeathBannerOverlay"));
	if (UVerticalBoxSlot* BannerSlot = CenterVBox->AddChildToVerticalBox(BannerOverlay))
	{
		BannerSlot->SetHorizontalAlignment(HAlign_Center);
		BannerSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	USizeBox* StrokeSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DeathStrokeSize"));
	StrokeSize->SetWidthOverride(1080.0f);
	StrokeSize->SetHeightOverride(180.0f);
	UImage* BloodStroke = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("DeathBloodStroke"));
	BloodStroke->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::MaskBloodStrokeLongA),
		FVector2D(1080.0f, 180.0f),
		FVelmaraUITheme::Blood(0.90f)));
	StrokeSize->SetContent(BloodStroke);
	if (UOverlaySlot* StrokeSlot = BannerOverlay->AddChildToOverlay(StrokeSize))
	{
		StrokeSlot->SetHorizontalAlignment(HAlign_Center);
		StrokeSlot->SetVerticalAlignment(VAlign_Center);
	}

	UCommonTextBlock* FallenText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("DeathFallenText"));
	FallenText->SetText(NSLOCTEXT("VelmaraUI", "DeathTitle", "H A S   C A Í D O"));
	FallenText->SetFont(FVelmaraUITheme::GetDisplayFont(64, 320));
	FallenText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
	FallenText->SetJustification(ETextJustify::Center);
	if (UOverlaySlot* TextSlot = BannerOverlay->AddChildToOverlay(FallenText))
	{
		TextSlot->SetHorizontalAlignment(HAlign_Center);
		TextSlot->SetVerticalAlignment(VAlign_Center);
	}

	USizeBox* BotSepSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DeathBotSepSize"));
	BotSepSize->SetWidthOverride(640.0f);
	BotSepSize->SetHeightOverride(30.0f);
	UImage* BotSep = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("DeathBotSep"));
	BotSep->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnSepStar),
		FVector2D(640.0f, 30.0f),
		FVelmaraUITheme::Bone(0.75f)));
	BotSepSize->SetContent(BotSep);
	if (UVerticalBoxSlot* BSepSlot = CenterVBox->AddChildToVerticalBox(BotSepSize))
	{
		BSepSlot->SetHorizontalAlignment(HAlign_Center);
		BSepSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 52.0f));
	}

	RetryButton = WidgetTree->ConstructWidget<UVelmaraButtonBase>(UVelmaraButtonBase::StaticClass(), TEXT("DeathRetryBtn"));
	RetryButton->SetVariant(EVelmaraButtonVariant::MinimalList);
	RetryButton->SetCenterAligned(true);
	RetryButton->SetFontSize(22);
	RetryButton->SetButtonMinSize(380.0f, 54.0f);
	RetryButton->SetButtonText(NSLOCTEXT("VelmaraUI", "DeathRetry", "REINTENTAR"));
	RetryButton->OnClicked().AddUObject(this, &UVelmaraDeathScreenWidget::HandleRetryClicked);
	if (UVerticalBoxSlot* RSlot = CenterVBox->AddChildToVerticalBox(RetryButton))
	{
		RSlot->SetHorizontalAlignment(HAlign_Center);
		RSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	MainMenuButton = WidgetTree->ConstructWidget<UVelmaraButtonBase>(UVelmaraButtonBase::StaticClass(), TEXT("DeathMainMenuBtn"));
	MainMenuButton->SetVariant(EVelmaraButtonVariant::MinimalList);
	MainMenuButton->SetCenterAligned(true);
	MainMenuButton->SetFontSize(22);
	MainMenuButton->SetButtonMinSize(380.0f, 54.0f);
	MainMenuButton->SetButtonText(NSLOCTEXT("VelmaraUI", "DeathMainMenu", "MENÚ PRINCIPAL"));
	MainMenuButton->OnClicked().AddUObject(this, &UVelmaraDeathScreenWidget::HandleMainMenuClicked);
	if (UVerticalBoxSlot* MSlot = CenterVBox->AddChildToVerticalBox(MainMenuButton))
	{
		MSlot->SetHorizontalAlignment(HAlign_Center);
	}

	FooterBar = WidgetTree->ConstructWidget<UVelmaraFooterBar>(UVelmaraFooterBar::StaticClass(), TEXT("DeathFooterBar"));
	FooterBar->SetShowMotto(true);
	FooterBar->SetActions({
		FVelmaraFooterAction(TEXT("Confirm"), NSLOCTEXT("VelmaraUI", "FooterSelect", "SELECCIONAR"))
	});
	if (UOverlaySlot* FootSlot = FadeRoot->AddChildToOverlay(FooterBar))
	{
		FootSlot->SetHorizontalAlignment(HAlign_Fill);
		FootSlot->SetVerticalAlignment(VAlign_Bottom);
		FootSlot->SetPadding(FMargin(64.0f, 0.0f, 64.0f, 36.0f));
	}
}

void UVelmaraDeathScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	FadeElapsed = 0.0f;
	if (FadeRoot)
	{
		FadeRoot->SetRenderOpacity(0.0f);
	}
	if (!IsActivated())
	{
		ActivateWidget();
	}
}

void UVelmaraDeathScreenWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	FadeElapsed = 0.0f;

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetShowMouseCursor(true);
		FInputModeUIOnly UIOnly;
		if (RetryButton)
		{
			UIOnly.SetWidgetToFocus(RetryButton->TakeWidget());
		}
		UIOnly.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(UIOnly);
	}

	if (RetryButton)
	{
		RetryButton->SetKeyboardFocus();
	}
}

void UVelmaraDeathScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (FadeRoot && FadeElapsed < FadeDuration)
	{
		FadeElapsed = FMath::Min(FadeDuration, FadeElapsed + InDeltaTime);
		const float Alpha = FMath::InterpEaseOut(0.0f, 1.0f, FadeElapsed / FadeDuration, 2.0f);
		FadeRoot->SetRenderOpacity(Alpha);
	}
}

UWidget* UVelmaraDeathScreenWidget::NativeGetDesiredFocusTarget() const
{
	return RetryButton;
}

void UVelmaraDeathScreenWidget::HandleRetryClicked()
{
	UGameplayStatics::SetGamePaused(GetWorld(), false);

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
	}

	UVelmaraGameInstance* GI = Cast<UVelmaraGameInstance>(GetGameInstance());
	USaveGameSubsystem* SaveSubsystem = GI ? GI->GetSubsystem<USaveGameSubsystem>() : nullptr;

	DeactivateWidget();
	RemoveFromParent();

	if (GI && SaveSubsystem)
	{
		const int32 ActiveSlot = GI->ActiveSaveSlotIndex;
		if (ActiveSlot >= 0 && SaveSubsystem->DoesSaveGameExist(ActiveSlot))
		{
			SaveSubsystem->LoadGame(ActiveSlot);
			return;
		}
		const int32 RecentSlot = SaveSubsystem->GetMostRecentSaveSlot();
		if (RecentSlot != INDEX_NONE)
		{
			SaveSubsystem->LoadGame(RecentSlot);
			return;
		}
	}

	if (GI)
	{
		GI->ShowLoadingScreen();
	}
	const FString CurrentLevel = UGameplayStatics::GetCurrentLevelName(this, true);
	UGameplayStatics::OpenLevel(this, FName(*CurrentLevel));
}

void UVelmaraDeathScreenWidget::HandleMainMenuClicked()
{
	UGameplayStatics::SetGamePaused(GetWorld(), false);

	if (UVelmaraGameInstance* GI = Cast<UVelmaraGameInstance>(GetGameInstance()))
	{
		GI->ShowLoadingScreen();
	}
	DeactivateWidget();
	RemoveFromParent();
	UGameplayStatics::OpenLevel(this, FName(TEXT("MainMenu")));
}
