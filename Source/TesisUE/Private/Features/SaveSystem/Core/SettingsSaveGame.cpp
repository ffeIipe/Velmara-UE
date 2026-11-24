#include "Features/SaveSystem/Core/SettingsSaveGame.h"

USettingsSaveGame::USettingsSaveGame()
{
	ScreenResolution = FIntPoint(1920, 1080);
	TextureQuality = 2;
	ShadowQuality = 2;
	AntiAliasingQuality = 2;
	ViewDistanceQuality = 2;
	PostProcessingQuality = 2;
	VisualEffectQuality = 2;
	GlobalIlluminationQuality = 2;
	ShadingQuality = 2;
	FrameRateLimit = 60.0f;
	GammaValue = 2.2f;
	WindowMode = EWindowMode::WindowedFullscreen;
	bLumenEnabled = true;
	OverallQualityPreset = 2;

	MasterVolume = 1.0f;
	MusicVolume = 0.8f;
	SFXVolume = 1.0f;
	UIVolume = 1.0f;

	MouseSensitivity = 1.0f;
	GamepadSensitivity = 1.0f;
	bInvertY = false;
	bVibrationEnabled = true;
	bCameraAssist = false;

	bSubtitlesEnabled = true;
	SubtitleSizeIndex = 1;
	LanguageCode = TEXT("es");
}
