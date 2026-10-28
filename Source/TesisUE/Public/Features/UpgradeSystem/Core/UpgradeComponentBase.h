#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "UpgradeComponentBase.generated.h"

class UInputAction;
class UUpgradeMenuWidgetBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUpgradeCurrencyChangedSignature, int32, NewCurrencyAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUpgradeMenuToggledSignature, bool, bIsOpen);

UENUM(BlueprintType)
enum class EUpgradePaymentResult : uint8
{
	Paid,
	NotEnoughCurrency,
	NoCurrencySystem
};

/*
 * Base de los sistemas de mejora (árbol de atributos, mejoras de arma).
 *
 * - Moneda: se obtiene a través de ICurrencyProvider para el tipo CurrencyType, así que no depende de ningún sistema
 *   concreto. Mientras no haya proveedor y bFreeUpgradesWithoutCurrency esté activo, las mejoras son gratis.
 * - Menú: ToggleMenu/OpenMenu/CloseMenu crean el widget, pausan el juego y manejan el modo de input.
 */
UCLASS(Abstract)
class TESISUE_API UUpgradeComponentBase : public UActorComponent
{
	GENERATED_BODY()

public:
	UUpgradeComponentBase();

	/* MENÚ */

	// Abre o cierra el menú de este sistema de mejoras.
	UFUNCTION(BlueprintCallable, Category = "Upgrades | Menu")
	void ToggleMenu();

	UFUNCTION(BlueprintCallable, Category = "Upgrades | Menu")
	void OpenMenu();

	UFUNCTION(BlueprintCallable, Category = "Upgrades | Menu")
	void CloseMenu();

	UFUNCTION(BlueprintPure, Category = "Upgrades | Menu")
	bool IsMenuOpen() const;

	/* MONEDA */

	// Fuerza un proveedor concreto en lugar de buscarlo automáticamente. Debe implementar ICurrencyProvider.
	UFUNCTION(BlueprintCallable, Category = "Upgrades | Currency")
	void SetCurrencyProvider(UObject* NewProvider);

	UFUNCTION(BlueprintPure, Category = "Upgrades | Currency")
	bool HasCurrencyProvider() const { return ResolveCurrencyProvider() != nullptr; }

	UFUNCTION(BlueprintPure, Category = "Upgrades | Currency")
	int32 GetCurrencyAmount() const;

	UFUNCTION(BlueprintPure, Category = "Upgrades | Currency")
	bool AreUpgradesFree() const { return bFreeUpgradesWithoutCurrency && !HasCurrencyProvider(); }

	UFUNCTION(BlueprintPure, Category = "Upgrades | Currency")
	bool CanPay(int32 Cost) const;

	UFUNCTION(BlueprintPure, Category = "Upgrades | Currency")
	FGameplayTag GetCurrencyType() const { return CurrencyType; }

	UFUNCTION(BlueprintPure, Category = "Upgrades | Currency")
	FText GetCurrencyDisplayName() const { return CurrencyDisplayName; }

	// El sistema de moneda puede llamarlo cuando cambia la cantidad para que la UI se actualice.
	UFUNCTION(BlueprintCallable, Category = "Upgrades | Currency")
	void NotifyCurrencyChanged();

	/* EVENTOS */

	UPROPERTY(BlueprintAssignable, Category = "Upgrades | Events")
	FOnUpgradeCurrencyChangedSignature OnCurrencyChanged;

	UPROPERTY(BlueprintAssignable, Category = "Upgrades | Events")
	FOnUpgradeMenuToggledSignature OnMenuToggled;

	// Input con el que se abre el menú. Si se asigna, presionarlo con el menú abierto también lo cierra (aunque el juego esté pausado).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrades | Menu")
	TObjectPtr<UInputAction> ToggleMenuInputAction;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Cobra Cost al proveedor de moneda (o no cobra nada en modo prueba). No modifica nada si no se pudo pagar.
	EUpgradePaymentResult TryPay(int32 Cost);

	/* CONFIGURACIÓN */

	// Moneda con la que se pagan las mejoras de este sistema.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrades | Currency", meta = (Categories = "Currency"))
	FGameplayTag CurrencyType;

	// Nombre de la moneda en la UI.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrades | Currency")
	FText CurrencyDisplayName;

	// Mientras no haya un ICurrencyProvider para CurrencyType, las mejoras no cuestan nada (útil hasta que exista la moneda).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrades | Currency")
	bool bFreeUpgradesWithoutCurrency = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrades | Menu")
	TSubclassOf<UUpgradeMenuWidgetBase> MenuWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrades | Menu")
	bool bPauseGameWhileOpen = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrades | Menu")
	int32 MenuZOrder = 50;

private:
	UObject* ResolveCurrencyProvider() const;

	APlayerController* GetOwningPlayerController() const;

	UPROPERTY(Transient)
	TObjectPtr<UUpgradeMenuWidgetBase> MenuInstance;

	UPROPERTY(Transient)
	TObjectPtr<UObject> CurrencyProviderOverride;

	bool bPausedByMenu = false;
};
