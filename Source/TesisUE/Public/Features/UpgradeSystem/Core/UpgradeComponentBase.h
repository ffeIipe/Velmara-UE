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

UCLASS(Abstract)
class TESISUE_API UUpgradeComponentBase : public UActorComponent
{
	GENERATED_BODY()

public:
	UUpgradeComponentBase();

	UFUNCTION(BlueprintCallable, Category = "Upgrades | Menu")
	void ToggleMenu();

	UFUNCTION(BlueprintCallable, Category = "Upgrades | Menu")
	void OpenMenu();

	UFUNCTION(BlueprintCallable, Category = "Upgrades | Menu")
	void CloseMenu();

	UFUNCTION(BlueprintPure, Category = "Upgrades | Menu")
	bool IsMenuOpen() const;

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

	UFUNCTION(BlueprintCallable, Category = "Upgrades | Currency")
	void NotifyCurrencyChanged();

	UPROPERTY(BlueprintAssignable, Category = "Upgrades | Events")
	FOnUpgradeCurrencyChangedSignature OnCurrencyChanged;

	UPROPERTY(BlueprintAssignable, Category = "Upgrades | Events")
	FOnUpgradeMenuToggledSignature OnMenuToggled;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrades | Menu")
	TObjectPtr<UInputAction> ToggleMenuInputAction;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	EUpgradePaymentResult TryPay(int32 Cost);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrades | Currency", meta = (Categories = "Currency"))
	FGameplayTag CurrencyType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrades | Currency")
	FText CurrencyDisplayName;

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
