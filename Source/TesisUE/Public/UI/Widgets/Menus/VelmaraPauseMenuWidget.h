#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "VelmaraPauseMenuWidget.generated.h"

class UVelmaraButtonBase;
class UVelmaraBracketPanel;
class UVelmaraFooterBar;
class UCommonTextBlock;

UCLASS()
class TESISUE_API UVelmaraPauseMenuWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UVelmaraPauseMenuWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Velmara|UI")
	void RefreshStatusPanel();

	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual bool NativeOnHandleBackAction() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void BuildScreenTree();
	void ApplyPauseState(bool bPaused);
	void HandleResumeClicked();
	void HandleInventoryClicked();
	void HandleSettingsClicked();
	void HandleSaveGameClicked();
	void HandleLoadGameClicked();
	void HandleMainMenuClicked();
	void HandleQuitDesktopClicked();

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraButtonBase> ResumeButton;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraButtonBase> InventoryButton;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraButtonBase> SettingsButton;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraButtonBase> SaveGameButton;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraButtonBase> LoadGameButton;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraButtonBase> MainMenuButton;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraButtonBase> QuitDesktopButton;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraBracketPanel> StatusPanel;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> ZoneTitleText;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> StatusBodyText;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> BloodValueText;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraFooterBar> FooterBar;
};
