#include "Features/UpgradeSystem/UI/UpgradeMenuWidgetBase.h"

#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "Engine/LocalPlayer.h"
#include "Features/UpgradeSystem/Core/UpgradeComponentBase.h"

void UUpgradeMenuWidgetBase::NativeOnInitialized()
{
	Super::NativeOnInitialized();

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

void UUpgradeMenuWidgetBase::SetEmbeddedMode(const bool bInEmbedded)
{
	bEmbeddedMode = bInEmbedded;
	OnEmbeddedModeChanged();
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
	if (!bEmbeddedMode && IsCloseKey(InKeyEvent.GetKey()))
	{
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
	if (bEmbeddedMode)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

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
