#include "UI/Layout/VelmaraPrimaryGameLayout.h"
#include "Blueprint/WidgetTree.h"
#include "CommonActivatableWidget.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Input/CommonUIActionRouterBase.h"
#include "Kismet/GameplayStatics.h"
#include "UI/Style/VelmaraUITheme.h"
#include "UI/Widgets/HUD/VelmaraTutorialToastWidget.h"
#include "UI/Widgets/HUD/VelmaraWorldPromptsWidget.h"

TMap<TWeakObjectPtr<ULocalPlayer>, TWeakObjectPtr<UVelmaraPrimaryGameLayout>> UVelmaraPrimaryGameLayout::ActiveLayouts;

UVelmaraPrimaryGameLayout::UVelmaraPrimaryGameLayout(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UVelmaraPrimaryGameLayout* UVelmaraPrimaryGameLayout::Get(const UObject* WorldContext)
{
	if (!WorldContext)
	{
		return nullptr;
	}

	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		return nullptr;
	}

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (const TWeakObjectPtr<UVelmaraPrimaryGameLayout>* Found = ActiveLayouts.Find(LP))
			{
				if (Found->IsValid())
				{
					return Found->Get();
				}
			}
		}
	}
	return nullptr;
}

UVelmaraPrimaryGameLayout* UVelmaraPrimaryGameLayout::GetOrCreate(APlayerController* PlayerController)
{
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return nullptr;
	}

	ULocalPlayer* LP = PlayerController->GetLocalPlayer();
	if (!LP)
	{
		return nullptr;
	}

	if (const TWeakObjectPtr<UVelmaraPrimaryGameLayout>* Found = ActiveLayouts.Find(LP))
	{
		if (Found->IsValid())
		{
			return Found->Get();
		}
	}

	UVelmaraPrimaryGameLayout* NewLayout = CreateWidget<UVelmaraPrimaryGameLayout>(PlayerController, StaticClass());
	if (NewLayout)
	{
		ActiveLayouts.Add(LP, NewLayout);
		NewLayout->AddToViewport(20);
	}
	return NewLayout;
}

TSharedRef<SWidget> UVelmaraPrimaryGameLayout::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayoutTree();
	}
	return Super::RebuildWidget();
}

void UVelmaraPrimaryGameLayout::BuildLayoutTree()
{
	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("PrimaryLayoutRoot"));
	RootOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = RootOverlay;

	UVelmaraWorldPromptsWidget* WorldPrompts = WidgetTree->ConstructWidget<UVelmaraWorldPromptsWidget>(UVelmaraWorldPromptsWidget::StaticClass(), TEXT("PrimaryWorldPromptsLayer"));
	WorldPrompts->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UOverlaySlot* PromptsSlot = RootOverlay->AddChildToOverlay(WorldPrompts))
	{
		PromptsSlot->SetHorizontalAlignment(HAlign_Fill);
		PromptsSlot->SetVerticalAlignment(VAlign_Fill);
	}

	HUDLayer = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("HUDLayer"));
	HUDLayer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (UOverlaySlot* HudSlot = RootOverlay->AddChildToOverlay(HUDLayer))
	{
		HudSlot->SetHorizontalAlignment(HAlign_Fill);
		HudSlot->SetVerticalAlignment(VAlign_Fill);
	}

	ToastLayer = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("ToastLayer"));
	ToastLayer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (UOverlaySlot* ToastSlot = RootOverlay->AddChildToOverlay(ToastLayer))
	{
		ToastSlot->SetHorizontalAlignment(HAlign_Fill);
		ToastSlot->SetVerticalAlignment(VAlign_Fill);
	}

	NotificationBanner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("NotificationBanner"));
	NotificationBanner->SetVisibility(ESlateVisibility::Collapsed);
	NotificationBanner->SetPadding(FMargin(24.0f, 12.0f, 28.0f, 12.0f));
	NotificationBanner->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::GradHFadeRightToLeft),
		FVector2D(512.0f, 8.0f),
		FVelmaraUITheme::Void(0.88f)));

	UHorizontalBox* NotifRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("NotifRow"));
	NotificationBanner->SetContent(NotifRow);

	USizeBox* NotifDiamondSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("NotifDiamondSize"));
	NotifDiamondSize->SetWidthOverride(12.0f);
	NotifDiamondSize->SetHeightOverride(12.0f);
	UImage* NotifDiamond = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("NotifDiamond"));
	NotifDiamond->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::MarkerDiamondSolid),
		FVector2D(12.0f, 12.0f),
		FVelmaraUITheme::BloodBright()));
	NotifDiamondSize->SetContent(NotifDiamond);
	if (UHorizontalBoxSlot* DiamondSlot = NotifRow->AddChildToHorizontalBox(NotifDiamondSize))
	{
		DiamondSlot->SetVerticalAlignment(VAlign_Center);
		DiamondSlot->SetPadding(FMargin(0.0f, 0.0f, 12.0f, 0.0f));
	}

	NotificationText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("NotificationText"));
	NotificationText->SetFont(FVelmaraUITheme::GetDisplayFont(17, 200));
	NotificationText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
	if (UHorizontalBoxSlot* TextSlot = NotifRow->AddChildToHorizontalBox(NotificationText))
	{
		TextSlot->SetVerticalAlignment(VAlign_Center);
	}

	if (UOverlaySlot* NotifSlot = ToastLayer->AddChildToOverlay(NotificationBanner))
	{
		NotifSlot->SetHorizontalAlignment(HAlign_Right);
		NotifSlot->SetVerticalAlignment(VAlign_Top);
		NotifSlot->SetPadding(FMargin(0.0f, 64.0f, 64.0f, 0.0f));
	}

	MenuStack = WidgetTree->ConstructWidget<UCommonActivatableWidgetStack>(UCommonActivatableWidgetStack::StaticClass(), TEXT("MenuStack"));
	MenuStack->SetTransitionDuration(0.0f);
	if (UOverlaySlot* MenuSlot = RootOverlay->AddChildToOverlay(MenuStack))
	{
		MenuSlot->SetHorizontalAlignment(HAlign_Fill);
		MenuSlot->SetVerticalAlignment(VAlign_Fill);
	}

	ModalStack = WidgetTree->ConstructWidget<UCommonActivatableWidgetStack>(UCommonActivatableWidgetStack::StaticClass(), TEXT("ModalStack"));
	ModalStack->SetTransitionDuration(0.0f);
	if (UOverlaySlot* ModalSlot = RootOverlay->AddChildToOverlay(ModalStack))
	{
		ModalSlot->SetHorizontalAlignment(HAlign_Fill);
		ModalSlot->SetVerticalAlignment(VAlign_Fill);
	}
}

void UVelmaraPrimaryGameLayout::NativeConstruct()
{
	Super::NativeConstruct();

	if (ULocalPlayer* LP = GetOwningLocalPlayer())
	{
		ActiveLayouts.Add(LP, this);
	}

	if (MenuStack)
	{
		MenuStack->OnDisplayedWidgetChanged().AddUObject(this, &UVelmaraPrimaryGameLayout::HandleDisplayedWidgetChanged);
	}
	if (ModalStack)
	{
		ModalStack->OnDisplayedWidgetChanged().AddUObject(this, &UVelmaraPrimaryGameLayout::HandleDisplayedWidgetChanged);
	}
}

void UVelmaraPrimaryGameLayout::NativeDestruct()
{
	if (ULocalPlayer* LP = GetOwningLocalPlayer())
	{
		if (const TWeakObjectPtr<UVelmaraPrimaryGameLayout>* Existing = ActiveLayouts.Find(LP))
		{
			if (Existing->Get() == this)
			{
				ActiveLayouts.Remove(LP);
			}
		}
	}

	if (MenuStack)
	{
		MenuStack->OnDisplayedWidgetChanged().RemoveAll(this);
	}
	if (ModalStack)
	{
		ModalStack->OnDisplayedWidgetChanged().RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UVelmaraPrimaryGameLayout::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (NotificationRemainingSeconds > 0.0f && NotificationBanner)
	{
		NotificationRemainingSeconds -= InDeltaTime;
		if (NotificationRemainingSeconds <= 0.0f)
		{
			NotificationBanner->SetVisibility(ESlateVisibility::Collapsed);
			NotificationBanner->SetRenderOpacity(1.0f);
		}
		else if (NotificationRemainingSeconds < 0.35f)
		{
			NotificationBanner->SetRenderOpacity(FMath::Clamp(NotificationRemainingSeconds / 0.35f, 0.0f, 1.0f));
		}
	}
}

UCommonActivatableWidget* UVelmaraPrimaryGameLayout::PushMenuWidgetByClass(TSubclassOf<UCommonActivatableWidget> WidgetClass)
{
	return PushMenuWidget<UCommonActivatableWidget>(WidgetClass);
}

void UVelmaraPrimaryGameLayout::PushMenuWidgetInstance(UCommonActivatableWidget* WidgetInstance)
{
	if (!MenuStack || !WidgetInstance)
	{
		return;
	}
	MenuStack->AddWidgetInstance(*WidgetInstance);
	RefreshInputMode();
}

void UVelmaraPrimaryGameLayout::ClearMenuStack()
{
	if (MenuStack)
	{
		MenuStack->ClearWidgets();
	}
	RefreshInputMode();
}

bool UVelmaraPrimaryGameLayout::IsInteractiveMenuWidget(const UCommonActivatableWidget* Widget) const
{
	if (!Widget || Widget->IsA<UVelmaraLegacyPromptBridgeWidget>())
	{
		return false;
	}

	const TOptional<FUIInputConfig> DesiredConfig = Widget->GetDesiredInputConfig();
	if (DesiredConfig.IsSet())
	{
		return DesiredConfig->GetInputMode() != ECommonInputMode::Game;
	}

	return Widget->SupportsActivationFocus();
}

bool UVelmaraPrimaryGameLayout::HasActiveMenu() const
{
	return MenuStack && IsInteractiveMenuWidget(MenuStack->GetActiveWidget());
}

bool UVelmaraPrimaryGameLayout::HasActiveModal() const
{
	return ModalStack && IsInteractiveMenuWidget(ModalStack->GetActiveWidget());
}

bool UVelmaraPrimaryGameLayout::HasActiveMenuOrModal() const
{
	return HasActiveMenu() || HasActiveModal();
}

void UVelmaraPrimaryGameLayout::AttachToHUDLayer(UUserWidget* Widget, bool bFill)
{
	if (!HUDLayer || !Widget)
	{
		return;
	}
	if (Widget->GetParent() == HUDLayer)
	{
		return;
	}
	Widget->RemoveFromParent();
	if (UOverlaySlot* ChildSlot = HUDLayer->AddChildToOverlay(Widget))
	{
		ChildSlot->SetHorizontalAlignment(bFill ? HAlign_Fill : HAlign_Center);
		ChildSlot->SetVerticalAlignment(bFill ? VAlign_Fill : VAlign_Bottom);
	}
}

void UVelmaraPrimaryGameLayout::AttachToToastLayer(UUserWidget* Widget)
{
	if (!ToastLayer || !Widget)
	{
		return;
	}
	if (Widget->GetParent() == ToastLayer)
	{
		return;
	}
	Widget->RemoveFromParent();
	if (UOverlaySlot* ChildSlot = ToastLayer->AddChildToOverlay(Widget))
	{
		ChildSlot->SetHorizontalAlignment(HAlign_Fill);
		ChildSlot->SetVerticalAlignment(VAlign_Fill);
	}
}

void UVelmaraPrimaryGameLayout::ShowTransientNotification(const FText& Message, float DurationSeconds)
{
	if (!NotificationBanner || !NotificationText)
	{
		return;
	}
	NotificationText->SetText(Message.ToUpper());
	NotificationTotalSeconds = FMath::Max(0.5f, DurationSeconds);
	NotificationRemainingSeconds = NotificationTotalSeconds;
	NotificationBanner->SetRenderOpacity(1.0f);
	NotificationBanner->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UVelmaraPrimaryGameLayout::HandleDisplayedWidgetChanged(UCommonActivatableWidget* DisplayedWidget)
{
	RefreshInputMode();
}

void UVelmaraPrimaryGameLayout::RefreshInputMode()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}

	UCommonActivatableWidget* TopWidget = nullptr;
	if (ModalStack && IsInteractiveMenuWidget(ModalStack->GetActiveWidget()))
	{
		TopWidget = ModalStack->GetActiveWidget();
	}
	else if (MenuStack && IsInteractiveMenuWidget(MenuStack->GetActiveWidget()))
	{
		TopWidget = MenuStack->GetActiveWidget();
	}

	UCommonUIActionRouterBase* ActionRouter = UCommonUIActionRouterBase::Get(*this);

	if (TopWidget != nullptr)
	{
		if (ActionRouter)
		{
			ActionRouter->SetActiveUIInputConfig(FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture, false), TopWidget);
		}
		PC->SetShowMouseCursor(true);
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		UWidget* FocusTarget = TopWidget->GetDesiredFocusTarget();
		InputMode.SetWidgetToFocus(FocusTarget ? FocusTarget->TakeWidget() : TopWidget->TakeWidget());
		PC->SetInputMode(InputMode);
	}
	else
	{
		if (ActionRouter)
		{
			ActionRouter->SetActiveUIInputConfig(FUIInputConfig(ECommonInputMode::Game, EMouseCaptureMode::CapturePermanently, true), this);
		}
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
	}
}
