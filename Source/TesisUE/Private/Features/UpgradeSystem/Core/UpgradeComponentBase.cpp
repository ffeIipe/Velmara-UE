#include "Features/UpgradeSystem/Core/UpgradeComponentBase.h"

#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
#include "Features/CurrencySystem/Data/CurrencyTags.h"
#include "Features/CurrencySystem/Interfaces/CurrencyProvider.h"
#include "Features/UpgradeSystem/UI/UpgradeMenuWidgetBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"

#define LOCTEXT_NAMESPACE "Upgrades"

UUpgradeComponentBase::UUpgradeComponentBase()
{
	// Componente puramente reactivo.
	PrimaryComponentTick.bCanEverTick = false;

	CurrencyType = CurrencyTags::Blood;
	CurrencyDisplayName = LOCTEXT("DefaultCurrencyName", "Sangre");
}

void UUpgradeComponentBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CloseMenu();

	Super::EndPlay(EndPlayReason);
}

/* MENÚ */

void UUpgradeComponentBase::ToggleMenu()
{
	if (IsMenuOpen())
	{
		CloseMenu();
	}
	else
	{
		OpenMenu();
	}
}

void UUpgradeComponentBase::OpenMenu()
{
	if (IsMenuOpen() || !MenuWidgetClass) return;

	APlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController || !PlayerController->IsLocalController()) return;

	// Un solo menú de mejoras abierto a la vez (ej. árbol de atributos y altar).
	TInlineComponentArray<UUpgradeComponentBase*> UpgradeComponents(GetOwner());
	for (UUpgradeComponentBase* Other : UpgradeComponents)
	{
		if (Other != this)
		{
			Other->CloseMenu();
		}
	}

	if (!MenuInstance || MenuInstance->GetOwningPlayer() != PlayerController)
	{
		MenuInstance = CreateWidget<UUpgradeMenuWidgetBase>(PlayerController, MenuWidgetClass);
		if (!MenuInstance) return;
	}

	MenuInstance->SetUpgradeComponent(this);
	MenuInstance->AddToViewport(MenuZOrder);

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(MenuInstance->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	PlayerController->SetInputMode(InputMode);
	PlayerController->SetShowMouseCursor(true);

	if (bPauseGameWhileOpen && !UGameplayStatics::IsGamePaused(this))
	{
		bPausedByMenu = UGameplayStatics::SetGamePaused(this, true);
	}

	OnMenuToggled.Broadcast(true);
}

void UUpgradeComponentBase::CloseMenu()
{
	if (!IsMenuOpen()) return;

	MenuInstance->RemoveFromParent();

	if (APlayerController* PlayerController = MenuInstance->GetOwningPlayer())
	{
		PlayerController->SetInputMode(FInputModeGameOnly());
		PlayerController->SetShowMouseCursor(false);
	}

	if (bPausedByMenu)
	{
		UGameplayStatics::SetGamePaused(this, false);
		bPausedByMenu = false;
	}

	OnMenuToggled.Broadcast(false);
}

bool UUpgradeComponentBase::IsMenuOpen() const
{
	return MenuInstance && MenuInstance->IsInViewport();
}

APlayerController* UUpgradeComponentBase::GetOwningPlayerController() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn ? OwnerPawn->GetController<APlayerController>() : nullptr;
}

/* MONEDA */

void UUpgradeComponentBase::SetCurrencyProvider(UObject* NewProvider)
{
	if (NewProvider && !NewProvider->GetClass()->ImplementsInterface(UCurrencyProvider::StaticClass()))
	{
		UE_LOG(LogTemp, Warning, TEXT("Upgrades: %s no implementa ICurrencyProvider."), *GetNameSafe(NewProvider));
		return;
	}

	CurrencyProviderOverride = NewProvider;
	NotifyCurrencyChanged();
}

int32 UUpgradeComponentBase::GetCurrencyAmount() const
{
	if (UObject* CurrencyProvider = ResolveCurrencyProvider())
	{
		return ICurrencyProvider::Execute_GetCurrencyAmount(CurrencyProvider, CurrencyType);
	}
	return 0;
}

bool UUpgradeComponentBase::CanPay(const int32 Cost) const
{
	if (UObject* CurrencyProvider = ResolveCurrencyProvider())
	{
		return ICurrencyProvider::Execute_GetCurrencyAmount(CurrencyProvider, CurrencyType) >= Cost;
	}
	return bFreeUpgradesWithoutCurrency;
}

EUpgradePaymentResult UUpgradeComponentBase::TryPay(const int32 Cost)
{
	UObject* CurrencyProvider = ResolveCurrencyProvider();
	if (!CurrencyProvider)
	{
		return bFreeUpgradesWithoutCurrency ? EUpgradePaymentResult::Paid : EUpgradePaymentResult::NoCurrencySystem;
	}

	if (ICurrencyProvider::Execute_GetCurrencyAmount(CurrencyProvider, CurrencyType) < Cost)
	{
		return EUpgradePaymentResult::NotEnoughCurrency;
	}

	if (Cost > 0 && !ICurrencyProvider::Execute_TrySpendCurrency(CurrencyProvider, CurrencyType, Cost))
	{
		return EUpgradePaymentResult::NotEnoughCurrency;
	}

	return EUpgradePaymentResult::Paid;
}

void UUpgradeComponentBase::NotifyCurrencyChanged()
{
	OnCurrencyChanged.Broadcast(GetCurrencyAmount());
}

UObject* UUpgradeComponentBase::ResolveCurrencyProvider() const
{
	if (CurrencyProviderOverride) return CurrencyProviderOverride;

	const FGameplayTag RequestedCurrency = CurrencyType;
	auto ProvidesRequestedCurrency = [&RequestedCurrency](const UObject* Object)
	{
		return Object
			&& Object->GetClass()->ImplementsInterface(UCurrencyProvider::StaticClass())
			&& ICurrencyProvider::Execute_ProvidesCurrency(Object, RequestedCurrency);
	};

	auto SearchActor = [&ProvidesRequestedCurrency](AActor* Actor) -> UObject*
	{
		if (!Actor) return nullptr;
		if (ProvidesRequestedCurrency(Actor)) return Actor;

		for (UActorComponent* Component : Actor->GetComponents())
		{
			if (ProvidesRequestedCurrency(Component)) return Component;
		}
		return nullptr;
	};

	// Orden de búsqueda: Pawn (y componentes) -> Controller -> PlayerState -> GameInstance.
	AActor* Owner = GetOwner();
	if (UObject* Found = SearchActor(Owner)) return Found;

	if (const APawn* OwnerPawn = Cast<APawn>(Owner))
	{
		if (UObject* Found = SearchActor(OwnerPawn->GetController())) return Found;
		if (UObject* Found = SearchActor(OwnerPawn->GetPlayerState())) return Found;
	}

	if (UGameInstance* GameInstance = Owner ? Owner->GetGameInstance() : nullptr)
	{
		if (ProvidesRequestedCurrency(GameInstance)) return GameInstance;
	}

	return nullptr;
}

#undef LOCTEXT_NAMESPACE
