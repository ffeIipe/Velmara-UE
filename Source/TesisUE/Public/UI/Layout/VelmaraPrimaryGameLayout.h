#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "VelmaraPrimaryGameLayout.generated.h"

class UOverlay;
class UCommonActivatableWidget;
class UCommonActivatableWidgetStack;
class UBorder;
class UCommonTextBlock;
class APlayerController;

UCLASS()
class TESISUE_API UVelmaraPrimaryGameLayout : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UVelmaraPrimaryGameLayout(const FObjectInitializer& ObjectInitializer);

	static UVelmaraPrimaryGameLayout* Get(const UObject* WorldContext);
	static UVelmaraPrimaryGameLayout* GetOrCreate(APlayerController* PlayerController);

	UFUNCTION(BlueprintPure, Category = "Velmara|UI")
	UOverlay* GetHUDLayer() const { return HUDLayer; }

	UFUNCTION(BlueprintPure, Category = "Velmara|UI")
	UOverlay* GetToastLayer() const { return ToastLayer; }

	UFUNCTION(BlueprintPure, Category = "Velmara|UI")
	UCommonActivatableWidgetStack* GetMenuStack() const { return MenuStack; }

	UFUNCTION(BlueprintPure, Category = "Velmara|UI")
	UCommonActivatableWidgetStack* GetModalStack() const { return ModalStack; }

	template <typename TWidget = UCommonActivatableWidget>
	TWidget* PushMenuWidget(TSubclassOf<UCommonActivatableWidget> WidgetClass)
	{
		if (!MenuStack || !WidgetClass)
		{
			return nullptr;
		}
		TWidget* Pushed = MenuStack->AddWidget<TWidget>(WidgetClass);
		RefreshInputMode();
		return Pushed;
	}

	template <typename TWidget = UCommonActivatableWidget>
	TWidget* PushMenuWidget(TSubclassOf<UCommonActivatableWidget> WidgetClass, TFunctionRef<void(TWidget&)> InitFunc)
	{
		if (!MenuStack || !WidgetClass)
		{
			return nullptr;
		}
		TWidget* Pushed = MenuStack->AddWidget<TWidget>(WidgetClass, InitFunc);
		RefreshInputMode();
		return Pushed;
	}

	template <typename TWidget = UCommonActivatableWidget>
	TWidget* PushModalWidget(TSubclassOf<UCommonActivatableWidget> WidgetClass, TFunctionRef<void(TWidget&)> InitFunc)
	{
		if (!ModalStack || !WidgetClass)
		{
			return nullptr;
		}
		TWidget* Pushed = ModalStack->AddWidget<TWidget>(WidgetClass, InitFunc);
		RefreshInputMode();
		return Pushed;
	}

	UFUNCTION(BlueprintCallable, Category = "Velmara|UI")
	UCommonActivatableWidget* PushMenuWidgetByClass(TSubclassOf<UCommonActivatableWidget> WidgetClass);

	UFUNCTION(BlueprintCallable, Category = "Velmara|UI")
	void PushMenuWidgetInstance(UCommonActivatableWidget* WidgetInstance);

	UFUNCTION(BlueprintCallable, Category = "Velmara|UI")
	void ClearMenuStack();

	UFUNCTION(BlueprintPure, Category = "Velmara|UI")
	bool HasActiveMenu() const;

	UFUNCTION(BlueprintPure, Category = "Velmara|UI")
	bool HasActiveModal() const;

	UFUNCTION(BlueprintPure, Category = "Velmara|UI")
	bool HasActiveMenuOrModal() const;

	UFUNCTION(BlueprintCallable, Category = "Velmara|UI")
	void AttachToHUDLayer(UUserWidget* Widget, bool bFill = true);

	UFUNCTION(BlueprintCallable, Category = "Velmara|UI")
	void AttachToToastLayer(UUserWidget* Widget);

	UFUNCTION(BlueprintCallable, Category = "Velmara|UI")
	void ShowTransientNotification(const FText& Message, float DurationSeconds = 3.0f);

	UFUNCTION(BlueprintCallable, Category = "Velmara|UI")
	void RefreshInputMode();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildLayoutTree();
	void HandleDisplayedWidgetChanged(UCommonActivatableWidget* DisplayedWidget);

	UPROPERTY(Transient)
	TObjectPtr<UOverlay> HUDLayer;

	UPROPERTY(Transient)
	TObjectPtr<UOverlay> ToastLayer;

	UPROPERTY(Transient)
	TObjectPtr<UCommonActivatableWidgetStack> MenuStack;

	UPROPERTY(Transient)
	TObjectPtr<UCommonActivatableWidgetStack> ModalStack;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> NotificationBanner;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> NotificationText;

	float NotificationRemainingSeconds = 0.0f;
	float NotificationTotalSeconds = 3.0f;

	static TMap<TWeakObjectPtr<ULocalPlayer>, TWeakObjectPtr<UVelmaraPrimaryGameLayout>> ActiveLayouts;
};
