#include "SceneEvents/VelmaraGameInstance.h"
#include "Framework/Application/SlateApplication.h"
#include "AudioModulationStatics.h"
#include "Blueprint/UserWidget.h"
#include "Features/GlobalEffectsSystem/Core/EffectsManager.h"
#include "Features/GlobalEffectsSystem/Core/EffectsTags.h"
#include "Features/SaveSystem/Core/SettingsSaveGame.h"
#include "Features/SaveSystem/Subsystems/SaveGameSubsystem.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerController.h"
#include "GAS/VelmaraGameplayTags.h"
#if !WITH_EDITOR
#include "Internationalization/Internationalization.h"
#endif
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundClass.h"
#include "SoundControlBus.h"
#include "SoundControlBusMix.h"
#include "StageZeroSubsystem.h"
#include "UI/Style/VelmaraUITheme.h"
#include "UI/Widgets/Menus/VelmaraLoadingScreenWidget.h"

UVelmaraGameInstance::UVelmaraGameInstance()
{
	CurrentSettings = nullptr;
	ActiveSaveSlotIndex = 0;
	CurrentLoadingScreenInstance = nullptr;
	CurrentSlotName = TEXT("GameSettings");
	UserIndex = 0;
}

void UVelmaraGameInstance::Init()
{
	Super::Init();
	FVelmaraGameplayTags::InitializeGameplayTags();
	LoadAudioAssets();
	LoadGameSettings();

	if (FSlateApplication::IsInitialized())
	{
		FocusChangingHandle = FSlateApplication::Get().OnFocusChanging().AddWeakLambda(this,
			[](const FFocusEvent& FocusEvent, const FWeakWidgetPath&, const TSharedPtr<SWidget>&, const FWidgetPath&, const TSharedPtr<SWidget>&)
			{
				if (FocusEvent.GetCause() == EFocusCause::Navigation)
				{
					FVelmaraUITheme::PlayHoverSound();
				}
			});
	}

	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UVelmaraGameInstance::HandlePostLoadMapWithWorld);

	const FWorldContext* Context = GetWorldContext();
	const bool bIsPIE = Context && Context->WorldType == EWorldType::PIE;

	if (UStageZeroSubsystem* StageZero = GetSubsystem<UStageZeroSubsystem>(); StageZero && !bIsPIE)
	{
		StageZero->SetPrimaryLevel(TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Maps/MainMenu/MainMenu.MainMenu"))));
	}
}

void UVelmaraGameInstance::Shutdown()
{
	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}
	if (FocusChangingHandle.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().OnFocusChanging().Remove(FocusChangingHandle);
		FocusChangingHandle.Reset();
	}
	Super::Shutdown();
}

void UVelmaraGameInstance::LoadAudioAssets()
{
	MasterVolumeBus = LoadObject<USoundControlBus>(nullptr, VelmaraUIPaths::AudioBusMaster);
	MusicVolumeBus = LoadObject<USoundControlBus>(nullptr, VelmaraUIPaths::AudioBusMusic);
	SFXVolumeBus = LoadObject<USoundControlBus>(nullptr, VelmaraUIPaths::AudioBusSFX);
	MenuSettingsBusMix = LoadObject<USoundControlBusMix>(nullptr, VelmaraUIPaths::AudioMixMenuSettings);
	PauseBusMix = LoadObject<USoundControlBusMix>(nullptr, VelmaraUIPaths::AudioMixPause);

	MasterSoundClass = LoadObject<USoundClass>(nullptr, VelmaraUIPaths::AudioClassMaster);
	MusicSoundClass = LoadObject<USoundClass>(nullptr, VelmaraUIPaths::AudioClassMusic);
	SFXSoundClass = LoadObject<USoundClass>(nullptr, VelmaraUIPaths::AudioClassSFX);
	UISoundClass = LoadObject<USoundClass>(nullptr, VelmaraUIPaths::AudioClassUI);
}

void UVelmaraGameInstance::HandlePostLoadMapWithWorld(UWorld* LoadedWorld)
{
	HideLoadingScreen();
	ApplyAudioSettings();
	ApplyGamma();
	ApplyLumenSettings();
	ApplyControlAndGameplaySettings();
}

void UVelmaraGameInstance::SetDefaultGameSettings()
{
	if (!CurrentSettings)
	{
		CurrentSettings = Cast<USettingsSaveGame>(UGameplayStatics::CreateSaveGameObject(USettingsSaveGame::StaticClass()));
	}

	if (CurrentSettings)
	{
		if (GEngine && GEngine->GameUserSettings)
		{
			CurrentSettings->ScreenResolution = GEngine->GameUserSettings->GetDesktopResolution();
			CurrentSettings->WindowMode = EWindowMode::WindowedFullscreen;
		}
		else
		{
			CurrentSettings->ScreenResolution = FIntPoint(1920, 1080);
			CurrentSettings->WindowMode = EWindowMode::WindowedFullscreen;
		}

		CurrentSettings->OverallQualityPreset = 2;
		CurrentSettings->TextureQuality = 2;
		CurrentSettings->ShadowQuality = 2;
		CurrentSettings->AntiAliasingQuality = 2;
		CurrentSettings->ViewDistanceQuality = 2;
		CurrentSettings->PostProcessingQuality = 2;
		CurrentSettings->VisualEffectQuality = 2;
		CurrentSettings->GlobalIlluminationQuality = 2;
		CurrentSettings->ShadingQuality = 2;
		CurrentSettings->bLumenEnabled = true;
		CurrentSettings->FrameRateLimit = 60.0f;
		CurrentSettings->GammaValue = 2.2f;

		CurrentSettings->MasterVolume = 1.0f;
		CurrentSettings->MusicVolume = 0.8f;
		CurrentSettings->SFXVolume = 1.0f;
		CurrentSettings->UIVolume = 1.0f;

		CurrentSettings->MouseSensitivity = 1.0f;
		CurrentSettings->GamepadSensitivity = 1.0f;
		CurrentSettings->bInvertY = false;
		CurrentSettings->bVibrationEnabled = true;
		CurrentSettings->bCameraAssist = false;

		CurrentSettings->bSubtitlesEnabled = true;
		CurrentSettings->SubtitleSizeIndex = 1;
		CurrentSettings->LanguageCode = TEXT("es");

		ApplyGraphicsSettings();
		ApplyAudioSettings();
		ApplyControlAndGameplaySettings();
		ApplyLanguageSetting();
	}
}

void UVelmaraGameInstance::ResetAllSettingsToDefaults()
{
	SetDefaultGameSettings();
	SaveGameSettings();
	OnSettingsChanged.Broadcast();
}

void UVelmaraGameInstance::LoadGameSettings()
{
	if (USettingsSaveGame* LoadedSettings = Cast<USettingsSaveGame>(UGameplayStatics::LoadGameFromSlot(SettingsSlotName, 0)))
	{
		CurrentSettings = LoadedSettings;
		if (CurrentSettings->LanguageCode.IsEmpty())
		{
			CurrentSettings->LanguageCode = TEXT("es");
		}
	}
	else
	{
		SetDefaultGameSettings();
		SaveGameSettings();
	}

	ApplyGraphicsSettings();
	ApplyAudioSettings();
	ApplyControlAndGameplaySettings();
	ApplyLanguageSetting();
}

void UVelmaraGameInstance::SaveGameSettings()
{
	if (CurrentSettings)
	{
		UGameplayStatics::SaveGameToSlot(CurrentSettings, SettingsSlotName, 0);
	}
}

void UVelmaraGameInstance::ApplyGraphicsSettings()
{
	if (!CurrentSettings)
	{
		return;
	}

	if (GEngine && GEngine->GameUserSettings)
	{
		UGameUserSettings* UserSettings = GEngine->GameUserSettings;

		UserSettings->SetFullscreenMode(CurrentSettings->WindowMode);
		UserSettings->SetScreenResolution(CurrentSettings->ScreenResolution);
		UserSettings->SetTextureQuality(CurrentSettings->TextureQuality);
		UserSettings->SetShadowQuality(CurrentSettings->ShadowQuality);
		UserSettings->SetAntiAliasingQuality(CurrentSettings->AntiAliasingQuality);
		UserSettings->SetViewDistanceQuality(CurrentSettings->ViewDistanceQuality);
		UserSettings->SetPostProcessingQuality(CurrentSettings->PostProcessingQuality);
		UserSettings->SetVisualEffectQuality(CurrentSettings->VisualEffectQuality);
		UserSettings->SetGlobalIlluminationQuality(CurrentSettings->GlobalIlluminationQuality);
		UserSettings->SetShadingQuality(CurrentSettings->ShadingQuality);
		UserSettings->SetFrameRateLimit(CurrentSettings->FrameRateLimit);
		UserSettings->SetFoliageQuality(0);
		ApplyGamma();
		ApplyLumenSettings();
		UserSettings->ApplySettings(false);

		if (CurrentSettings->ShadowQuality == 0)
		{
			GEngine->Exec(GetWorld(), TEXT("r.ShadowQuality 1"));
			GEngine->Exec(GetWorld(), TEXT("r.Shadow.CSM.MaxCascades 1"));
			GEngine->Exec(GetWorld(), TEXT("r.Shadow.MaxResolution 512"));
			GEngine->Exec(GetWorld(), TEXT("r.Shadow.RadiusThreshold 0.08"));
			GEngine->Exec(GetWorld(), TEXT("r.Shadow.DistanceScale 0.6"));
		}

		SaveGameSettings();
	}
}

void UVelmaraGameInstance::ApplyLumenSettings() const
{
	if (!GEngine || !CurrentSettings)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (CurrentSettings->bLumenEnabled)
	{
		GEngine->Exec(World, TEXT("r.DynamicGlobalIlluminationMethod 1"));
		GEngine->Exec(World, TEXT("r.ReflectionMethod 1"));
	}
	else
	{
		GEngine->Exec(World, TEXT("r.DynamicGlobalIlluminationMethod 2"));
		GEngine->Exec(World, TEXT("r.ReflectionMethod 2"));
	}
}

void UVelmaraGameInstance::ApplyAudioSettings()
{
	if (!CurrentSettings)
	{
		return;
	}

	const float Master = FMath::Clamp(CurrentSettings->MasterVolume, 0.0f, 1.0f);
	const float Music = FMath::Clamp(CurrentSettings->MusicVolume, 0.0f, 1.0f);
	const float SFX = FMath::Clamp(CurrentSettings->SFXVolume, 0.0f, 1.0f);
	const float UI = FMath::Clamp(CurrentSettings->UIVolume, 0.0f, 1.0f);

	if (MasterSoundClass)
	{
		MasterSoundClass->Properties.Volume = Master;
	}
	if (MusicSoundClass)
	{
		MusicSoundClass->Properties.Volume = Music;
	}
	if (SFXSoundClass)
	{
		SFXSoundClass->Properties.Volume = SFX;
	}
	if (UISoundClass)
	{
		UISoundClass->Properties.Volume = UI;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (MenuSettingsBusMix)
	{
		UAudioModulationStatics::ActivateBusMix(World, MenuSettingsBusMix);

		TArray<FSoundControlBusMixStage> Stages;
		if (MasterVolumeBus)
		{
			Stages.Add(UAudioModulationStatics::CreateBusMixStage(World, MasterVolumeBus, Master, 0.05f, 0.05f));
		}
		if (MusicVolumeBus)
		{
			Stages.Add(UAudioModulationStatics::CreateBusMixStage(World, MusicVolumeBus, Music, 0.05f, 0.05f));
		}
		if (SFXVolumeBus)
		{
			Stages.Add(UAudioModulationStatics::CreateBusMixStage(World, SFXVolumeBus, SFX, 0.05f, 0.05f));
		}
		if (Stages.Num() > 0)
		{
			UAudioModulationStatics::UpdateMix(World, MenuSettingsBusMix, Stages, 0.05f);
		}
	}

	if (MasterVolumeBus)
	{
		UAudioModulationStatics::SetGlobalBusMixValue(World, MasterVolumeBus, Master, 0.05f);
	}
	if (MusicVolumeBus)
	{
		UAudioModulationStatics::SetGlobalBusMixValue(World, MusicVolumeBus, Music, 0.05f);
	}
	if (SFXVolumeBus)
	{
		UAudioModulationStatics::SetGlobalBusMixValue(World, SFXVolumeBus, SFX, 0.05f);
	}
}

void UVelmaraGameInstance::SetPauseAudioMixActive(bool bActive)
{
	UWorld* World = GetWorld();
	if (!World || !PauseBusMix)
	{
		return;
	}

	if (bActive)
	{
		UAudioModulationStatics::ActivateBusMix(World, PauseBusMix);
	}
	else
	{
		UAudioModulationStatics::DeactivateBusMix(World, PauseBusMix);
	}
}

void UVelmaraGameInstance::ApplyControlAndGameplaySettings() const
{
	if (!CurrentSettings)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
		{
			PC->bForceFeedbackEnabled = CurrentSettings->bVibrationEnabled;
		}
	}
}

FString UVelmaraGameInstance::NormalizeLanguageCode(const FString& RawLanguageCode)
{
	const FString Trimmed = RawLanguageCode.TrimStartAndEnd();
	if (Trimmed.StartsWith(TEXT("en"), ESearchCase::IgnoreCase))
	{
		return TEXT("en");
	}
	return TEXT("es");
}

void UVelmaraGameInstance::ApplyLanguageSetting()
{
	const FString TargetCode = NormalizeLanguageCode(CurrentSettings ? CurrentSettings->LanguageCode : TEXT("es"));
	if (CurrentSettings)
	{
		CurrentSettings->LanguageCode = TargetCode;
	}
	ActiveGameLanguageCode = TargetCode;
	OnGameLanguageChanged.Broadcast(ActiveGameLanguageCode);
#if !WITH_EDITOR
	if (bEnableGameLocalizationRuntime)
	{
		FInternationalization::Get().SetCurrentLanguage(ActiveGameLanguageCode);
	}
#endif
}

void UVelmaraGameInstance::ApplyStagedGraphicsSettings(const FVelmaraGraphicsSettingsSnapshot& Snapshot)
{
	if (!CurrentSettings)
	{
		return;
	}

	CurrentSettings->WindowMode = Snapshot.WindowMode;
	CurrentSettings->ScreenResolution = Snapshot.ScreenResolution;
	CurrentSettings->OverallQualityPreset = Snapshot.OverallQualityPreset;
	CurrentSettings->ViewDistanceQuality = FMath::Clamp(Snapshot.ViewDistanceQuality, 0, 3);
	CurrentSettings->AntiAliasingQuality = FMath::Clamp(Snapshot.AntiAliasingQuality, 0, 3);
	CurrentSettings->PostProcessingQuality = FMath::Clamp(Snapshot.PostProcessingQuality, 0, 3);
	CurrentSettings->ShadowQuality = FMath::Clamp(Snapshot.ShadowQuality, 0, 3);
	CurrentSettings->GlobalIlluminationQuality = FMath::Clamp(Snapshot.GlobalIlluminationQuality, 0, 3);
	CurrentSettings->VisualEffectQuality = FMath::Clamp(Snapshot.VisualEffectQuality, 0, 3);
	CurrentSettings->TextureQuality = FMath::Clamp(Snapshot.TextureQuality, 0, 3);
	CurrentSettings->ShadingQuality = FMath::Clamp(Snapshot.ShadingQuality, 0, 3);
	CurrentSettings->bLumenEnabled = Snapshot.bLumenEnabled;
	CurrentSettings->FrameRateLimit = static_cast<float>(FMath::Max(0, Snapshot.FrameRateLimit));

	ApplyGraphicsSettings();
	OnSettingsChanged.Broadcast();
}

FVelmaraGraphicsSettingsSnapshot UVelmaraGameInstance::GetCurrentGraphicsSnapshot() const
{
	FVelmaraGraphicsSettingsSnapshot Snapshot;
	if (!CurrentSettings)
	{
		return Snapshot;
	}

	Snapshot.WindowMode = CurrentSettings->WindowMode;
	Snapshot.ScreenResolution = CurrentSettings->ScreenResolution;
	Snapshot.OverallQualityPreset = CurrentSettings->OverallQualityPreset;
	Snapshot.ViewDistanceQuality = CurrentSettings->ViewDistanceQuality;
	Snapshot.AntiAliasingQuality = CurrentSettings->AntiAliasingQuality;
	Snapshot.PostProcessingQuality = CurrentSettings->PostProcessingQuality;
	Snapshot.ShadowQuality = CurrentSettings->ShadowQuality;
	Snapshot.GlobalIlluminationQuality = CurrentSettings->GlobalIlluminationQuality;
	Snapshot.VisualEffectQuality = CurrentSettings->VisualEffectQuality;
	Snapshot.TextureQuality = CurrentSettings->TextureQuality;
	Snapshot.ShadingQuality = CurrentSettings->ShadingQuality;
	Snapshot.bLumenEnabled = CurrentSettings->bLumenEnabled;
	Snapshot.FrameRateLimit = FMath::RoundToInt(CurrentSettings->FrameRateLimit);
	Snapshot.RecalculateOverallPreset();
	return Snapshot;
}

void UVelmaraGameInstance::SetWindowMode(EWindowMode::Type NewWindowMode)
{
	if (CurrentSettings)
	{
		CurrentSettings->WindowMode = NewWindowMode;
		ApplyGraphicsSettings();
		SaveGameSettings();
	}
}

void UVelmaraGameInstance::SetScreenResolution(FIntPoint NewResolution)
{
	if (CurrentSettings)
	{
		CurrentSettings->ScreenResolution = NewResolution;
		ApplyGraphicsSettings();
		SaveGameSettings();
	}
}

void UVelmaraGameInstance::SetOverallQualityPreset(int32 NewPreset)
{
	if (CurrentSettings)
	{
		const int32 Clamped = FMath::Clamp(NewPreset, 0, 4);
		CurrentSettings->OverallQualityPreset = Clamped;
		if (Clamped <= 3)
		{
			CurrentSettings->TextureQuality = Clamped;
			CurrentSettings->ShadowQuality = Clamped;
			CurrentSettings->AntiAliasingQuality = Clamped;
			CurrentSettings->ViewDistanceQuality = Clamped;
			CurrentSettings->PostProcessingQuality = Clamped;
			CurrentSettings->VisualEffectQuality = Clamped;
			CurrentSettings->GlobalIlluminationQuality = Clamped;
			CurrentSettings->ShadingQuality = Clamped;
		}
		ApplyGraphicsSettings();
		SaveGameSettings();
	}
}

void UVelmaraGameInstance::SetTextureQuality(int32 NewQuality)
{
	if (CurrentSettings)
	{
		CurrentSettings->TextureQuality = FMath::Clamp(NewQuality, 0, 3);
		ApplyGraphicsSettings();
		SaveGameSettings();
	}
}

void UVelmaraGameInstance::SetShadowQuality(int32 NewQuality)
{
	if (CurrentSettings)
	{
		CurrentSettings->ShadowQuality = FMath::Clamp(NewQuality, 0, 3);
		ApplyGraphicsSettings();
		SaveGameSettings();
	}
}

void UVelmaraGameInstance::SetAntiAliasing(int32 NewQuality)
{
	if (CurrentSettings)
	{
		CurrentSettings->AntiAliasingQuality = FMath::Clamp(NewQuality, 0, 3);
		ApplyGraphicsSettings();
		SaveGameSettings();
	}
}

void UVelmaraGameInstance::SetViewDistance(int32 NewQuality)
{
	if (CurrentSettings)
	{
		CurrentSettings->ViewDistanceQuality = FMath::Clamp(NewQuality, 0, 3);
		ApplyGraphicsSettings();
		SaveGameSettings();
	}
}

void UVelmaraGameInstance::SetPostProcess(int32 NewQuality)
{
	if (CurrentSettings)
	{
		CurrentSettings->PostProcessingQuality = FMath::Clamp(NewQuality, 0, 3);
		ApplyGraphicsSettings();
		SaveGameSettings();
	}
}

void UVelmaraGameInstance::SetVisualEffect(int32 NewQuality)
{
	if (CurrentSettings)
	{
		CurrentSettings->VisualEffectQuality = FMath::Clamp(NewQuality, 0, 3);
		ApplyGraphicsSettings();
		SaveGameSettings();
	}
}

void UVelmaraGameInstance::SetGlobalIllumination(int32 NewQuality)
{
	if (CurrentSettings)
	{
		CurrentSettings->GlobalIlluminationQuality = FMath::Clamp(NewQuality, 0, 3);
		ApplyGraphicsSettings();
		SaveGameSettings();
	}
}

void UVelmaraGameInstance::SetShadingQuality(int32 NewQuality)
{
	if (CurrentSettings)
	{
		CurrentSettings->ShadingQuality = FMath::Clamp(NewQuality, 0, 3);
		ApplyGraphicsSettings();
		SaveGameSettings();
	}
}

void UVelmaraGameInstance::SetLumenEnabled(bool bEnabled)
{
	if (CurrentSettings)
	{
		CurrentSettings->bLumenEnabled = bEnabled;
		ApplyLumenSettings();
		SaveGameSettings();
	}
}

void UVelmaraGameInstance::SetFrameRateLimit(int32 NewFrameRateLimit)
{
	if (CurrentSettings)
	{
		CurrentSettings->FrameRateLimit = static_cast<float>(FMath::Max(0, NewFrameRateLimit));
		ApplyGraphicsSettings();
		SaveGameSettings();
	}
}

void UVelmaraGameInstance::SetGamma(float NewGamma)
{
	if (CurrentSettings)
	{
		CurrentSettings->GammaValue = FMath::Clamp(NewGamma, 1.4f, 3.0f);
		ApplyGamma();
		SaveGameSettings();
		OnSettingsChanged.Broadcast();
	}
}

void UVelmaraGameInstance::ApplyGamma() const
{
	if (GEngine && CurrentSettings)
	{
		const FString GammaCommand = FString::Printf(TEXT("gamma %f"), CurrentSettings->GammaValue);
		GEngine->Exec(GetWorld(), *GammaCommand);
	}
}

void UVelmaraGameInstance::SetMasterVolume(float NewVolume)
{
	if (CurrentSettings)
	{
		CurrentSettings->MasterVolume = FMath::Clamp(NewVolume, 0.0f, 1.0f);
		ApplyAudioSettings();
		SaveGameSettings();
		OnSettingsChanged.Broadcast();
	}
}

void UVelmaraGameInstance::SetMusicVolume(float NewVolume)
{
	if (CurrentSettings)
	{
		CurrentSettings->MusicVolume = FMath::Clamp(NewVolume, 0.0f, 1.0f);
		ApplyAudioSettings();
		SaveGameSettings();
		OnSettingsChanged.Broadcast();
	}
}

void UVelmaraGameInstance::SetSFXVolume(float NewVolume)
{
	if (CurrentSettings)
	{
		CurrentSettings->SFXVolume = FMath::Clamp(NewVolume, 0.0f, 1.0f);
		ApplyAudioSettings();
		SaveGameSettings();
		OnSettingsChanged.Broadcast();
	}
}

void UVelmaraGameInstance::SetUIVolume(float NewVolume)
{
	if (CurrentSettings)
	{
		CurrentSettings->UIVolume = FMath::Clamp(NewVolume, 0.0f, 1.0f);
		ApplyAudioSettings();
		SaveGameSettings();
		OnSettingsChanged.Broadcast();
	}
}

void UVelmaraGameInstance::SetMouseSensitivity(float NewSensitivity)
{
	if (CurrentSettings)
	{
		CurrentSettings->MouseSensitivity = FMath::Clamp(NewSensitivity, 0.2f, 3.0f);
		ApplyControlAndGameplaySettings();
		SaveGameSettings();
		OnSettingsChanged.Broadcast();
	}
}

void UVelmaraGameInstance::SetGamepadSensitivity(float NewSensitivity)
{
	if (CurrentSettings)
	{
		CurrentSettings->GamepadSensitivity = FMath::Clamp(NewSensitivity, 0.2f, 3.0f);
		ApplyControlAndGameplaySettings();
		SaveGameSettings();
		OnSettingsChanged.Broadcast();
	}
}

void UVelmaraGameInstance::SetInvertY(bool bInvert)
{
	if (CurrentSettings)
	{
		CurrentSettings->bInvertY = bInvert;
		ApplyControlAndGameplaySettings();
		SaveGameSettings();
		OnSettingsChanged.Broadcast();
	}
}

void UVelmaraGameInstance::SetVibrationEnabled(bool bEnabled)
{
	if (CurrentSettings)
	{
		CurrentSettings->bVibrationEnabled = bEnabled;
		ApplyControlAndGameplaySettings();
		SaveGameSettings();
		OnSettingsChanged.Broadcast();
	}
}

void UVelmaraGameInstance::SetCameraAssist(bool bEnabled)
{
	if (CurrentSettings)
	{
		CurrentSettings->bCameraAssist = bEnabled;
		SaveGameSettings();
		OnSettingsChanged.Broadcast();
	}
}

void UVelmaraGameInstance::SetSubtitlesEnabled(bool bEnabled)
{
	if (CurrentSettings)
	{
		CurrentSettings->bSubtitlesEnabled = bEnabled;
		SaveGameSettings();
		OnSettingsChanged.Broadcast();
	}
}

void UVelmaraGameInstance::SetSubtitleSizeIndex(int32 NewSizeIndex)
{
	if (CurrentSettings)
	{
		CurrentSettings->SubtitleSizeIndex = FMath::Clamp(NewSizeIndex, 0, 2);
		SaveGameSettings();
		OnSettingsChanged.Broadcast();
	}
}

void UVelmaraGameInstance::SetLanguageCode(const FString& NewLanguageCode)
{
	if (CurrentSettings)
	{
		CurrentSettings->LanguageCode = NormalizeLanguageCode(NewLanguageCode);
		ApplyLanguageSetting();
		SaveGameSettings();
		OnSettingsChanged.Broadcast();
	}
}

EWindowMode::Type UVelmaraGameInstance::GetWindowMode() const
{
	return CurrentSettings ? CurrentSettings->WindowMode.GetValue() : EWindowMode::WindowedFullscreen;
}

FIntPoint UVelmaraGameInstance::GetScreenResolution() const
{
	return CurrentSettings ? CurrentSettings->ScreenResolution : FIntPoint(1920, 1080);
}

int32 UVelmaraGameInstance::GetOverallQualityPreset() const
{
	return CurrentSettings ? CurrentSettings->OverallQualityPreset : 2;
}

int32 UVelmaraGameInstance::GetTextureQuality() const
{
	return CurrentSettings ? CurrentSettings->TextureQuality : 2;
}

int32 UVelmaraGameInstance::GetShadowQuality() const
{
	return CurrentSettings ? CurrentSettings->ShadowQuality : 2;
}

int32 UVelmaraGameInstance::GetAntiAliasingQuality() const
{
	return CurrentSettings ? CurrentSettings->AntiAliasingQuality : 2;
}

int32 UVelmaraGameInstance::GetViewDistanceQuality() const
{
	return CurrentSettings ? CurrentSettings->ViewDistanceQuality : 2;
}

int32 UVelmaraGameInstance::GetPostProcessQuality() const
{
	return CurrentSettings ? CurrentSettings->PostProcessingQuality : 2;
}

int32 UVelmaraGameInstance::GetGlobalIlluminationQuality() const
{
	return CurrentSettings ? CurrentSettings->GlobalIlluminationQuality : 2;
}

int32 UVelmaraGameInstance::GetShadingQuality() const
{
	return CurrentSettings ? CurrentSettings->ShadingQuality : 2;
}

int32 UVelmaraGameInstance::GetVisualEffectsQuality() const
{
	return CurrentSettings ? CurrentSettings->VisualEffectQuality : 2;
}

bool UVelmaraGameInstance::IsLumenEnabled() const
{
	return CurrentSettings ? CurrentSettings->bLumenEnabled : true;
}

int32 UVelmaraGameInstance::GetFrameRateLimit() const
{
	return CurrentSettings ? FMath::RoundToInt(CurrentSettings->FrameRateLimit) : 60;
}

float UVelmaraGameInstance::GetGamma() const
{
	return CurrentSettings ? CurrentSettings->GammaValue : 2.2f;
}

float UVelmaraGameInstance::GetMasterVolume() const
{
	return CurrentSettings ? CurrentSettings->MasterVolume : 1.0f;
}

float UVelmaraGameInstance::GetMusicVolume() const
{
	return CurrentSettings ? CurrentSettings->MusicVolume : 0.8f;
}

float UVelmaraGameInstance::GetSFXVolume() const
{
	return CurrentSettings ? CurrentSettings->SFXVolume : 1.0f;
}

float UVelmaraGameInstance::GetUIVolume() const
{
	return CurrentSettings ? CurrentSettings->UIVolume : 1.0f;
}

float UVelmaraGameInstance::GetMouseSensitivity() const
{
	return CurrentSettings ? CurrentSettings->MouseSensitivity : 1.0f;
}

float UVelmaraGameInstance::GetGamepadSensitivity() const
{
	return CurrentSettings ? CurrentSettings->GamepadSensitivity : 1.0f;
}

bool UVelmaraGameInstance::IsInvertY() const
{
	return CurrentSettings ? CurrentSettings->bInvertY : false;
}

bool UVelmaraGameInstance::IsVibrationEnabled() const
{
	return CurrentSettings ? CurrentSettings->bVibrationEnabled : true;
}

bool UVelmaraGameInstance::IsCameraAssist() const
{
	return CurrentSettings ? CurrentSettings->bCameraAssist : false;
}

bool UVelmaraGameInstance::AreSubtitlesEnabled() const
{
	return CurrentSettings ? CurrentSettings->bSubtitlesEnabled : true;
}

int32 UVelmaraGameInstance::GetSubtitleSizeIndex() const
{
	return CurrentSettings ? CurrentSettings->SubtitleSizeIndex : 1;
}

FString UVelmaraGameInstance::GetLanguageCode() const
{
	return NormalizeLanguageCode(CurrentSettings ? CurrentSettings->LanguageCode : ActiveGameLanguageCode);
}

FString UVelmaraGameInstance::GetActiveGameLanguageCode() const
{
	return NormalizeLanguageCode(CurrentSettings ? CurrentSettings->LanguageCode : ActiveGameLanguageCode);
}

void UVelmaraGameInstance::ShowLoadingScreen()
{
	TSubclassOf<UUserWidget> EffectiveClass = LoadingScreenWidgetClass;
	if (!EffectiveClass)
	{
		EffectiveClass = UVelmaraLoadingScreenWidget::StaticClass();
	}

	if (EffectiveClass && !CurrentLoadingScreenInstance)
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			CurrentLoadingScreenInstance = CreateWidget<UUserWidget>(PC, EffectiveClass);
			if (CurrentLoadingScreenInstance)
			{
				CurrentLoadingScreenInstance->AddToViewport(100);
			}
		}
	}
}

void UVelmaraGameInstance::HideLoadingScreen()
{
	if (CurrentLoadingScreenInstance)
	{
		CurrentLoadingScreenInstance->RemoveFromParent();
		CurrentLoadingScreenInstance = nullptr;
	}
}

void UVelmaraGameInstance::PlayGameplayEffect_Implementation(FGameplayTag EffectTag, const FVector Location)
{
	UEffectsManager* EffectsSys = nullptr;

	if (const UWorld* World = GetWorld())
	{
		EffectsSys = World->GetSubsystem<UEffectsManager>();
	}

	if (!EffectsSys) return;

	if (EffectTag.MatchesTag(EffectsTags::CameraShake))
	{
		EffectsSys->CameraShake(EffectTag, Location);
	}
	else if (EffectTag.MatchesTag(EffectsTags::CameraZoom))
	{
		EffectsSys->CameraZoom(EffectTag);
	}
	else if (EffectTag.MatchesTag(EffectsTags::TimeWarp))
	{
		EffectsSys->TimeWarp(EffectTag);
	}
	else if (EffectTag.MatchesTag(EffectsTags::HitStop))
	{
		EffectsSys->HitStop(EffectTag);
	}
}

void UVelmaraGameInstance::SaveGame(const int32 SlotIndex)
{
	ActiveSaveSlotIndex = FMath::Clamp(SlotIndex, 0, 2);

	if (USaveGameSubsystem* SaveSystem = GetSubsystem<USaveGameSubsystem>())
	{
		SaveSystem->SaveGame(ActiveSaveSlotIndex);
	}
}

void UVelmaraGameInstance::LoadGame(const int32 SlotIndex)
{
	ActiveSaveSlotIndex = FMath::Clamp(SlotIndex, 0, 2);

	if (USaveGameSubsystem* SaveSystem = GetSubsystem<USaveGameSubsystem>())
	{
		SaveSystem->LoadGame(ActiveSaveSlotIndex);
	}
}

void UVelmaraGameInstance::CreateNewGame(const int32 SlotIndex, const FString StartLevelName)
{
	ActiveSaveSlotIndex = FMath::Clamp(SlotIndex, 0, 2);

	const FString LegacySlotName = GetSlotNameByIndex(ActiveSaveSlotIndex);
	if (UGameplayStatics::DoesSaveGameExist(LegacySlotName, DefaultUserIndex))
	{
		UGameplayStatics::DeleteGameInSlot(LegacySlotName, DefaultUserIndex);
	}

	if (USaveGameSubsystem* SaveSystem = GetSubsystem<USaveGameSubsystem>())
	{
		SaveSystem->CreateNewGame(ActiveSaveSlotIndex, StartLevelName);
		return;
	}

	ShowLoadingScreen();
	UGameplayStatics::OpenLevel(this, FName(*StartLevelName));
}

bool UVelmaraGameInstance::DoesProgressSaveExist(const int32 SlotIndex) const
{
	if (const USaveGameSubsystem* SaveSystem = GetSubsystem<USaveGameSubsystem>())
	{
		if (SaveSystem->DoesSaveGameExist(SlotIndex))
		{
			return true;
		}
	}
	return UGameplayStatics::DoesSaveGameExist(GetSlotNameByIndex(SlotIndex), DefaultUserIndex);
}

FString UVelmaraGameInstance::GetSlotNameByIndex(const int32 SlotIndex) const
{
	return FString::Printf(TEXT("%s%d"), *ProgressSaveSlotPrefix, FMath::Clamp(SlotIndex, 0, 2));
}

UPlayerProgressSaveGame* UVelmaraGameInstance::GetSaveGameInfo(const int32 SlotIndex) const
{
	if (const USaveGameSubsystem* SaveSystem = GetSubsystem<USaveGameSubsystem>())
	{
		if (UPlayerProgressSaveGame* SaveObj = SaveSystem->LoadSaveSlotObject(SlotIndex))
		{
			return SaveObj;
		}
	}
	if (UGameplayStatics::DoesSaveGameExist(GetSlotNameByIndex(SlotIndex), DefaultUserIndex))
	{
		return Cast<UPlayerProgressSaveGame>(UGameplayStatics::LoadGameFromSlot(GetSlotNameByIndex(SlotIndex), DefaultUserIndex));
	}
	return nullptr;
}
