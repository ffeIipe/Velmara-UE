#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "VelmaraDeathScreenWidget.generated.h"

class UOverlay;
class UVelmaraButtonBase;
class UVelmaraFooterBar;

UCLASS()
class TESISUE_API UVelmaraDeathScreenWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UVelmaraDeathScreenWidget(const FObjectInitializer& ObjectInitializer);

	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

private:
	void BuildScreenTree();
	void HandleRetryClicked();
	void HandleMainMenuClicked();

	UPROPERTY(Transient)
	TObjectPtr<UOverlay> FadeRoot;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraButtonBase> RetryButton;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraButtonBase> MainMenuButton;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraFooterBar> FooterBar;

	float FadeElapsed = 0.0f;
	static constexpr float FadeDuration = 1.2f;
};
