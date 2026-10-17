#include "Features/UpgradeSystem/UI/UpgradeMenuWidgetBase.h"

#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "Engine/LocalPlayer.h"
#include "Features/UpgradeSystem/Core/UpgradeComponentBase.h"

void UUpgradeMenuWidgetBase::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// Necesario para recibir Esc / la tecla de cierre.
	SetIsFocusable(true);
}

void UUpgradeMenuWidgetBase::NativeDestruct()
{
	UnbindFromUpgradeComponent();

	Super::NativeDestruct();
}

void UUpgradeMenuWidgetBase::SetUpgradeComponent(UUpgradeComponentBase* InUpgradeComponent)
{
	UnbindFromUpgradeComponent();
	UpgradeComponent = InUpgradeComponent;
	BindToUpgradeComponent();

	OnUpgradeComponentSet();
}

void UUpgradeMenuWidgetBase::BindToUpgradeComponent()
{
	if (UUpgradeComponentBase* Component = UpgradeComponent.Get())
	{
		Component->OnCurrencyChanged.AddUniqueDynamic(this, &UUpgradeMenuWidgetBase::HandleCurrencyChanged);
	}
}

void UUpgradeMenuWidgetBase::UnbindFromUpgradeComponent()
{
	if (UUpgradeComponentBase* Component = UpgradeComponent.Get())
	{
		Component->OnCurrencyChanged.RemoveDynamic(this, &UUpgradeMenuWidgetBase::HandleCurrencyChanged);
	}
}

void UUpgradeMenuWidgetBase::HandleCurrencyChanged(int32 NewCurrencyAmount)
{
	RefreshMenu();
}

FReply UUpgradeMenuWidgetBase::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (IsCloseKey(InKeyEvent.GetKey()))
	{
		// Ignoramos la repetición por mantener la tecla para no cerrar/abrir en bucle.
		if (!InKeyEvent.IsRepeat())
		{
			RequestClose();
		}
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UUpgradeMenuWidgetBase::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// Clickear el fondo no debe hacer perder el foco del teclado (si no, Esc dejaría de cerrar el menú).
	return FReply::Handled().SetUserFocus(TakeWidget(), EFocusCause::Mouse);
}

bool UUpgradeMenuWidgetBase::IsCloseKey(const FKey& Key) const
{
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::Gamepad_Special_Right)
	{
		return true;
	}

	const UUpgradeComponentBase* Component = UpgradeComponent.Get();
	if (!Component || !Component->ToggleMenuInputAction) return false;

	if (const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
	{
		if (const UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			return InputSubsystem->QueryKeysMappedToAction(Component->ToggleMenuInputAction).Contains(Key);
		}
	}

	return false;
}

void UUpgradeMenuWidgetBase::RequestClose()
{
	if (UUpgradeComponentBase* Component = UpgradeComponent.Get())
	{
		Component->CloseMenu();
	}
	else
	{
		RemoveFromParent();
	}
}
