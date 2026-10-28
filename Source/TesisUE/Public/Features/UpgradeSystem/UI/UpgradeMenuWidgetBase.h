#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UpgradeMenuWidgetBase.generated.h"

class UUpgradeComponentBase;

/*
 * Base de los menús de mejora. Se encarga de:
 * - recibir el componente dueño (SetUpgradeComponent) y refrescarse cuando cambia la moneda,
 * - cerrarse con Esc, B/Start del gamepad o la tecla del ToggleMenuInputAction del componente,
 * - no perder el foco del teclado al clickear el fondo.
 */
UCLASS(Abstract)
class TESISUE_API UUpgradeMenuWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Upgrades")
	void SetUpgradeComponent(UUpgradeComponentBase* InUpgradeComponent);

	UFUNCTION(BlueprintPure, Category = "Upgrades")
	UUpgradeComponentBase* GetUpgradeComponent() const { return UpgradeComponent.Get(); }

	UFUNCTION(BlueprintCallable, Category = "Upgrades")
	virtual void RefreshMenu() {}

protected:
	virtual void NativeOnInitialized() override;

	virtual void NativeDestruct() override;

	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// Las subclases agregan acá sus propias suscripciones (llamando a Super).
	virtual void BindToUpgradeComponent();

	virtual void UnbindFromUpgradeComponent();

	// Se llama cada vez que se abre el menú con un componente (antes de mostrarse).
	virtual void OnUpgradeComponentSet() { RefreshMenu(); }

	void RequestClose();

private:
	bool IsCloseKey(const FKey& Key) const;

	UFUNCTION()
	void HandleCurrencyChanged(int32 NewCurrencyAmount);

	TWeakObjectPtr<UUpgradeComponentBase> UpgradeComponent;
};
