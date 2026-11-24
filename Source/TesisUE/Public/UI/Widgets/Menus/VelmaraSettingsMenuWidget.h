#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "SceneEvents/VelmaraGameInstance.h"
#include "UI/Widgets/Common/VelmaraSettingRowBase.h"
#include "VelmaraSettingsMenuWidget.generated.h"

class UVelmaraButtonBase;
class UVelmaraSettingCarouselRow;
class UVelmaraSettingSliderRow;
class UVelmaraBracketPanel;
class UVelmaraFooterBar;
class UWidgetSwitcher;
class UVerticalBox;
class UCommonTextBlock;
class UHorizontalBox;

UCLASS()
class TESISUE_API UVelmaraSettingsMenuWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UVelmaraSettingsMenuWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Velmara|Settings")
	void SelectCategory(int32 CategoryIndex);

	UFUNCTION(BlueprintCallable, Category = "Velmara|Settings")
	void RefreshAllFromGameInstance();

	UFUNCTION(BlueprintCallable, Category = "Velmara|Settings")
	void ApplyPendingGraphicsChanges();

	UFUNCTION(BlueprintCallable, Category = "Velmara|Settings")
	void RequestResetToDefaults();

	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnActivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual bool NativeOnHandleBackAction() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void BuildScreenTree();
	UVerticalBox* CreateScrollPage(UWidgetSwitcher* Switcher);
	void BuildDisplayPage(UVerticalBox* PageBox);
	void BuildAudioPage(UVerticalBox* PageBox);
	void BuildGameplayPage(UVerticalBox* PageBox);
	void BuildControlsPage(UVerticalBox* PageBox);
	void BuildAccessibilityPage(UVerticalBox* PageBox);
	void BuildLanguagePage(UVerticalBox* PageBox);
	void BuildCreditsPage(UVerticalBox* PageBox);

	void PopulateSupportedResolutions();
	void PopulateControlBindingsTable();
	void SyncScalabilityRowsFromStaged();
	void UpdateDirtyState();
	void UpdateDescriptionPanel(const FVelmaraSettingMetadata& Meta);
	void UpdateFooterActions();
	void HandleBackRequested();
	void StepCategory(int32 Delta);

	UVelmaraSettingCarouselRow* AddCarouselRow(
		UVerticalBox* Container,
		const FText& Label,
		const TArray<FText>& Options,
		int32 InitialIndex,
		bool bWrap,
		const FVelmaraSettingMetadata& Meta,
		TFunction<void(int32)> OnChanged);

	UVelmaraSettingSliderRow* AddSliderRow(
		UVerticalBox* Container,
		const FText& Label,
		float MinVal,
		float MaxVal,
		float Step,
		float InitialVal,
		int32 FormatEnumIndex,
		const FVelmaraSettingMetadata& Meta,
		TFunction<void(float)> OnChanged);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UVelmaraButtonBase>> CategoryButtons;

	UPROPERTY(Transient)
	TObjectPtr<UWidgetSwitcher> CategorySwitcher;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidget>> CategoryFirstFocusTargets;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraSettingCarouselRow> WindowModeRow;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraSettingCarouselRow> ResolutionRow;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraSettingCarouselRow> OverallQualityRow;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraSettingCarouselRow> ViewDistanceRow;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraSettingCarouselRow> AntiAliasingRow;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraSettingCarouselRow> PostProcessRow;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraSettingCarouselRow> ShadowQualityRow;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraSettingCarouselRow> GlobalIllumRow;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraSettingCarouselRow> VisualEffectsRow;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraSettingCarouselRow> TextureQualityRow;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraSettingCarouselRow> ShadingQualityRow;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraSettingCarouselRow> LumenRow;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraSettingCarouselRow> MaxFpsRow;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraSettingSliderRow> GammaRow;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraSettingSliderRow> MasterVolumeRow;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraSettingSliderRow> MusicVolumeRow;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraSettingSliderRow> SFXVolumeRow;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraSettingSliderRow> UIVolumeRow;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraSettingCarouselRow> VibrationRow;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraSettingCarouselRow> CameraAssistRow;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraSettingSliderRow> MouseSensRow;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraSettingSliderRow> GamepadSensRow;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraSettingCarouselRow> InvertYRow;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> BindingsTableBox;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraSettingCarouselRow> SubtitlesEnabledRow;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraSettingCarouselRow> SubtitleSizeRow;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraSettingCarouselRow> LanguageRow;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraBracketPanel> DescriptionPanel;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> DescTitleText;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> DescBodyText;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> PendingWarningText;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> DescQuoteText;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraButtonBase> ApplyChangesButton;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraButtonBase> ResetDefaultsButton;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraFooterBar> FooterBar;

	TArray<FIntPoint> SupportedResolutions;
	TArray<int32> FpsOptions;

	FVelmaraGraphicsSettingsSnapshot AppliedGraphics;
	FVelmaraGraphicsSettingsSnapshot StagedGraphics;
	bool bHasPendingGraphicsChanges = false;
	bool bSyncingRows = false;
	int32 ActiveCategoryIndex = 0;
};
