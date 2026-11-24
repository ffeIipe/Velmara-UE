#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "Blueprint/UserWidget.h"
#include "VelmaraMainMenuWidget.generated.h"

class UVelmaraButtonBase;
class UVelmaraBracketPanel;
class UVelmaraFooterBar;
class UImage;

UCLASS()
class TESISUE_API UVelmaraMainMenuWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UVelmaraMainMenuWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Velmara|UI")
	void RefreshContinueVisibility();

	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnActivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual bool NativeOnHandleBackAction() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void BuildScreenTree();
	void HandleContinueClicked();
	void HandleNewGameClicked();
	void HandleLoadGameClicked();
	void HandlePracticeZoneClicked();
	void HandleSettingsClicked();
	void HandleQuitClicked();

	UPROPERTY(Transient)
	TObjectPtr<UImage> LogoImage;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraBracketPanel> NavPanel;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraButtonBase> ContinueButton;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraButtonBase> NewGameButton;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraButtonBase> LoadGameButton;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraButtonBase> PracticeZoneButton;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraButtonBase> SettingsButton;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraButtonBase> QuitButton;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraFooterBar> FooterBar;

	int32 MostRecentSlotIndex = INDEX_NONE;
};

UCLASS()
class TESISUE_API UVelmaraTitleScreenBridgeWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
};
