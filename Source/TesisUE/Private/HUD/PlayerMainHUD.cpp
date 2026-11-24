#include "HUD/PlayerMainHUD.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "HUD/PlayerMainWidget.h"
#include "Kismet/GameplayStatics.h"
#include "UI/Layout/VelmaraPrimaryGameLayout.h"
#include "UI/Widgets/Menus/VelmaraMainMenuWidget.h"
#include "UI/Widgets/Menus/VelmaraPauseMenuWidget.h"
#include "UI/Widgets/HUD/VelmaraGameplayHUDWidget.h"

void APlayerMainHUD::BeginPlay()
{
	Super::BeginPlay();

	EnsurePrimaryLayout();

	const FString LevelName = UGameplayStatics::GetCurrentLevelName(GetWorld(), true);
	if (LevelName.Contains(TEXT("MainMenu"), ESearchCase::IgnoreCase))
	{
		ShowMainMenu();
	}
}

UVelmaraPrimaryGameLayout* APlayerMainHUD::EnsurePrimaryLayout()
{
	if (!PrimaryLayout)
	{
		if (APlayerController* PC = GetOwningPlayerController())
		{
			PrimaryLayout = UVelmaraPrimaryGameLayout::GetOrCreate(PC);
		}
	}
	if (PrimaryLayout && PrimaryLayout->GetMenuStack())
	{
		if (FObjectProperty* StackProp = FindFProperty<FObjectProperty>(GetClass(), TEXT("CurrentWidgetStack")))
		{
			StackProp->SetObjectPropertyValue_InContainer(this, PrimaryLayout->GetMenuStack());
		}
	}
	return PrimaryLayout;
}

void APlayerMainHUD::ShowMainMenu()
{
	if (UVelmaraPrimaryGameLayout* Layout = EnsurePrimaryLayout())
	{
		if (!Layout->HasActiveMenu())
		{
			Layout->PushMenuWidget<UVelmaraMainMenuWidget>(UVelmaraMainMenuWidget::StaticClass());
		}
	}
}

void APlayerMainHUD::TogglePauseMenu()
{
	UVelmaraPrimaryGameLayout* Layout = EnsurePrimaryLayout();
	if (!Layout)
	{
		return;
	}

	if (Layout->HasActiveModal())
	{
		return;
	}

	if (UCommonActivatableWidgetStack* MenuStack = Layout->GetMenuStack())
	{
		if (UCommonActivatableWidget* ActiveMenu = MenuStack->GetActiveWidget())
		{
			if (ActiveMenu->IsA<UVelmaraMainMenuWidget>())
			{
				return;
			}
			ActiveMenu->DeactivateWidget();
			return;
		}
	}

	Layout->PushMenuWidget<UVelmaraPauseMenuWidget>(UVelmaraPauseMenuWidget::StaticClass());
}

void APlayerMainHUD::ShowNotification(const FText& Message, float DurationSeconds)
{
	if (UVelmaraPrimaryGameLayout* Layout = EnsurePrimaryLayout())
	{
		Layout->ShowTransientNotification(Message, DurationSeconds);
	}
}

void APlayerMainHUD::TogglePlayerUI(bool Bool)
{
	if (Bool)
	{
		TSubclassOf<UPlayerMainWidget> EffectiveHUDClass = PlayerMainWidgetClass;
		if (!EffectiveHUDClass)
		{
			EffectiveHUDClass = UVelmaraGameplayHUDWidget::StaticClass();
		}

		if (!PlayerMainWidgetInstance && EffectiveHUDClass)
		{
			APlayerController* PC = GetOwningPlayerController();
			if (!PC)
			{
				PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
			}
			if (PC)
			{
				PlayerMainWidgetInstance = CreateWidget<UPlayerMainWidget>(PC, EffectiveHUDClass);
			}
		}

		if (PlayerMainWidgetInstance)
		{
			if (UVelmaraPrimaryGameLayout* Layout = EnsurePrimaryLayout())
			{
				Layout->AttachToHUDLayer(PlayerMainWidgetInstance, true);
			}
			else if (!PlayerMainWidgetInstance->IsInViewport())
			{
				PlayerMainWidgetInstance->AddToViewport();
			}
		}
	}
	else
	{
		if (PlayerMainWidgetInstance)
		{
			PlayerMainWidgetInstance->RemoveFromParent();
			PlayerMainWidgetInstance = nullptr;
		}
	}
}
