#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Features/GlobalEffectsSystem/Interfaces/EffectManagerProvider.h"
#include "VelmaraGameInstance.generated.h"

struct FGameplayTag;
class UPlayerProgressSaveGame;
class USettingsSaveGame;
class ACharacter;
class AEnemy;
class USoundControlBus;
class USoundControlBusMix;
class USoundClass;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSaveDataUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSettingsChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameLanguageChanged, const FString&, NewLanguageCode);

USTRUCT(BlueprintType)
struct TESISUE_API FVelmaraGraphicsSettingsSnapshot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<EWindowMode::Type> WindowMode = EWindowMode::WindowedFullscreen;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint ScreenResolution = FIntPoint(1920, 1080);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 OverallQualityPreset = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ViewDistanceQuality = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AntiAliasingQuality = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PostProcessingQuality = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ShadowQuality = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 GlobalIlluminationQuality = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 VisualEffectQuality = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TextureQuality = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ShadingQuality = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bLumenEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 FrameRateLimit = 60;

	bool operator==(const FVelmaraGraphicsSettingsSnapshot& Other) const
	{
		return WindowMode == Other.WindowMode
			&& ScreenResolution == Other.ScreenResolution
			&& OverallQualityPreset == Other.OverallQualityPreset
			&& ViewDistanceQuality == Other.ViewDistanceQuality
			&& AntiAliasingQuality == Other.AntiAliasingQuality
			&& PostProcessingQuality == Other.PostProcessingQuality
			&& ShadowQuality == Other.ShadowQuality
			&& GlobalIlluminationQuality == Other.GlobalIlluminationQuality
			&& VisualEffectQuality == Other.VisualEffectQuality
			&& TextureQuality == Other.TextureQuality
			&& ShadingQuality == Other.ShadingQuality
			&& bLumenEnabled == Other.bLumenEnabled
			&& FrameRateLimit == Other.FrameRateLimit;
	}

	bool operator!=(const FVelmaraGraphicsSettingsSnapshot& Other) const
	{
		return !(*this == Other);
	}

	void ApplyPresetToScalability(int32 PresetIndex)
	{
		const int32 Clamped = FMath::Clamp(PresetIndex, 0, 3);
		OverallQualityPreset = Clamped;
		ViewDistanceQuality = Clamped;
		AntiAliasingQuality = Clamped;
		PostProcessingQuality = Clamped;
		ShadowQuality = Clamped;
		GlobalIlluminationQuality = Clamped;
		VisualEffectQuality = Clamped;
		TextureQuality = Clamped;
		ShadingQuality = Clamped;
	}

	void RecalculateOverallPreset()
	{
		const int32 First = ViewDistanceQuality;
		const bool bAllEqual = (AntiAliasingQuality == First)
			&& (PostProcessingQuality == First)
			&& (ShadowQuality == First)
			&& (GlobalIlluminationQuality == First)
			&& (VisualEffectQuality == First)
			&& (TextureQuality == First)
			&& (ShadingQuality == First)
			&& (First >= 0 && First <= 3);
		OverallQualityPreset = bAllEqual ? First : 4;
	}
};

UCLASS()
class TESISUE_API UVelmaraGameInstance : public UGameInstance, public IEffectManagerProvider
{
	GENERATED_BODY()

public:
	UVelmaraGameInstance();

	virtual void Init() override;
	virtual void Shutdown() override;

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SetWindowMode(EWindowMode::Type NewWindowMode);

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SetScreenResolution(FIntPoint NewResolution);

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SetOverallQualityPreset(int32 NewPreset);

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SetTextureQuality(int32 NewQuality);

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SetShadowQuality(int32 NewQuality);

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SetAntiAliasing(int32 NewQuality);

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SetViewDistance(int32 NewQuality);

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SetPostProcess(int32 NewQuality);

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SetVisualEffect(int32 NewQuality);

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SetGlobalIllumination(int32 NewQuality);

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SetShadingQuality(int32 NewQuality);

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SetLumenEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SetFrameRateLimit(int32 NewFrameRateLimit);

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SetGamma(float NewGamma);

	void ApplyGamma() const;
	void ApplyLumenSettings() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetMasterVolume(float NewVolume);

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetMusicVolume(float NewVolume);

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetSFXVolume(float NewVolume);

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetUIVolume(float NewVolume);

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void ApplyAudioSettings();

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetPauseAudioMixActive(bool bActive);

	UFUNCTION(BlueprintCallable, Category = "Settings|Controls")
	void SetMouseSensitivity(float NewSensitivity);

	UFUNCTION(BlueprintCallable, Category = "Settings|Controls")
	void SetGamepadSensitivity(float NewSensitivity);

	UFUNCTION(BlueprintCallable, Category = "Settings|Controls")
	void SetInvertY(bool bInvert);

	UFUNCTION(BlueprintCallable, Category = "Settings|Gameplay")
	void SetVibrationEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Settings|Gameplay")
	void SetCameraAssist(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Settings|Accessibility")
	void SetSubtitlesEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Settings|Accessibility")
	void SetSubtitleSizeIndex(int32 NewSizeIndex);

	UFUNCTION(BlueprintCallable, Category = "Settings|Language")
	void SetLanguageCode(const FString& NewLanguageCode);

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void ApplyStagedGraphicsSettings(const FVelmaraGraphicsSettingsSnapshot& Snapshot);

	UFUNCTION(BlueprintPure, Category = "Settings")
	FVelmaraGraphicsSettingsSnapshot GetCurrentGraphicsSnapshot() const;

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void ResetAllSettingsToDefaults();

	UFUNCTION(BlueprintPure, Category = "Settings")
	EWindowMode::Type GetWindowMode() const;

	UFUNCTION(BlueprintPure, Category = "Settings")
	FIntPoint GetScreenResolution() const;

	UFUNCTION(BlueprintPure, Category = "Settings")
	int32 GetOverallQualityPreset() const;

	UFUNCTION(BlueprintPure, Category = "Settings")
	int32 GetTextureQuality() const;

	UFUNCTION(BlueprintPure, Category = "Settings")
	int32 GetShadowQuality() const;

	UFUNCTION(BlueprintPure, Category = "Settings")
	int32 GetAntiAliasingQuality() const;

	UFUNCTION(BlueprintPure, Category = "Settings")
	int32 GetViewDistanceQuality() const;

	UFUNCTION(BlueprintPure, Category = "Settings")
	int32 GetPostProcessQuality() const;

	UFUNCTION(BlueprintPure, Category = "Settings")
	int32 GetGlobalIlluminationQuality() const;

	UFUNCTION(BlueprintPure, Category = "Settings")
	int32 GetShadingQuality() const;

	UFUNCTION(BlueprintPure, Category = "Settings")
	int32 GetVisualEffectsQuality() const;

	UFUNCTION(BlueprintPure, Category = "Settings")
	bool IsLumenEnabled() const;

	UFUNCTION(BlueprintPure, Category = "Settings")
	int32 GetFrameRateLimit() const;

	UFUNCTION(BlueprintPure, Category = "Settings")
	float GetGamma() const;

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetMasterVolume() const;

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetMusicVolume() const;

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetSFXVolume() const;

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetUIVolume() const;

	UFUNCTION(BlueprintPure, Category = "Settings|Controls")
	float GetMouseSensitivity() const;

	UFUNCTION(BlueprintPure, Category = "Settings|Controls")
	float GetGamepadSensitivity() const;

	UFUNCTION(BlueprintPure, Category = "Settings|Controls")
	bool IsInvertY() const;

	UFUNCTION(BlueprintPure, Category = "Settings|Gameplay")
	bool IsVibrationEnabled() const;

	UFUNCTION(BlueprintPure, Category = "Settings|Gameplay")
	bool IsCameraAssist() const;

	UFUNCTION(BlueprintPure, Category = "Settings|Accessibility")
	bool AreSubtitlesEnabled() const;

	UFUNCTION(BlueprintPure, Category = "Settings|Accessibility")
	int32 GetSubtitleSizeIndex() const;

	UFUNCTION(BlueprintPure, Category = "Settings|Language")
	FString GetLanguageCode() const;

	UFUNCTION(BlueprintPure, Category = "Settings|Language")
	FString GetActiveGameLanguageCode() const;

	UPROPERTY(BlueprintAssignable, Category = "Settings|Language")
	FOnGameLanguageChanged OnGameLanguageChanged;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings|Language")
	bool bEnableGameLocalizationRuntime = false;

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SaveGameSettings();

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void LoadGameSettings();

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void ApplyGraphicsSettings();

	UPROPERTY(BlueprintReadOnly, Category = "Settings")
	USettingsSaveGame* CurrentSettings;

	UFUNCTION(BlueprintCallable, Category = "GameFlow")
	void CreateNewGame(int32 SlotIndex, FString StartLevelName);

	UFUNCTION(BlueprintCallable, Category = "GameFlow")
	bool DoesProgressSaveExist(int32 SlotIndex) const;

	UFUNCTION(BlueprintCallable, Category = "SaveSystem")
	void SaveGame(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "SaveSystem")
	void LoadGame(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "SaveSystem")
	UPlayerProgressSaveGame* GetSaveGameInfo(int32 SlotIndex) const;

	FString GetSlotNameByIndex(int32 SlotIndex) const;

	UPROPERTY(BlueprintReadWrite, Category = "SaveSystem")
	int32 ActiveSaveSlotIndex;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Loading")
	TSubclassOf<UUserWidget> LoadingScreenWidgetClass;

	UFUNCTION(BlueprintCallable, Category = "UI|Loading")
	void ShowLoadingScreen();

	UFUNCTION(BlueprintCallable, Category = "UI|Loading")
	void HideLoadingScreen();

	UPROPERTY(BlueprintAssignable, Category = "SaveSystem")
	FOnSaveDataUpdated OnSaveDataUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Settings")
	FOnSettingsChanged OnSettingsChanged;

	virtual void PlayGameplayEffect_Implementation(FGameplayTag EffectTag, FVector Location) override;

private:
	void LoadAudioAssets();
	void ApplyControlAndGameplaySettings() const;
	void ApplyLanguageSetting();
	static FString NormalizeLanguageCode(const FString& RawLanguageCode);
	void HandlePostLoadMapWithWorld(UWorld* LoadedWorld);

	const FString SettingsSlotName = TEXT("GameSettings");

	UFUNCTION(BlueprintCallable, Category = "GameSettings")
	void SetDefaultGameSettings();

	UPROPERTY(VisibleAnywhere, Category = "Metadata")
	FString CurrentSlotName;

	const FString ProgressSaveSlotPrefix = TEXT("PlayerProgressSlot_");

	UPROPERTY(VisibleAnywhere, Category = "Metadata")
	uint32 UserIndex;

	const int32 DefaultUserIndex = 0;

	UPROPERTY()
	UUserWidget* CurrentLoadingScreenInstance;

	UPROPERTY()
	TObjectPtr<USoundControlBus> MasterVolumeBus;

	UPROPERTY()
	TObjectPtr<USoundControlBus> MusicVolumeBus;

	UPROPERTY()
	TObjectPtr<USoundControlBus> SFXVolumeBus;

	UPROPERTY()
	TObjectPtr<USoundControlBusMix> MenuSettingsBusMix;

	UPROPERTY()
	TObjectPtr<USoundControlBusMix> PauseBusMix;

	UPROPERTY()
	TObjectPtr<USoundClass> MasterSoundClass;

	UPROPERTY()
	TObjectPtr<USoundClass> MusicSoundClass;

	UPROPERTY()
	TObjectPtr<USoundClass> SFXSoundClass;

	UPROPERTY()
	TObjectPtr<USoundClass> UISoundClass;

	FDelegateHandle PostLoadMapHandle;
	FDelegateHandle FocusChangingHandle;

	FString ActiveGameLanguageCode = TEXT("es");
};
