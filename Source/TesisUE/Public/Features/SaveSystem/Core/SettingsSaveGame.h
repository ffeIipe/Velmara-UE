#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SettingsSaveGame.generated.h"

UCLASS()
class TESISUE_API USettingsSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	USettingsSaveGame();

	UPROPERTY(VisibleAnywhere, Category = "Settings|Graphics")
	FIntPoint ScreenResolution;

	UPROPERTY(VisibleAnywhere, Category = "Settings|Graphics")
	int32 TextureQuality;

	UPROPERTY(VisibleAnywhere, Category = "Settings|Graphics")
	int32 ShadowQuality;

	UPROPERTY(VisibleAnywhere, Category = "Settings|Graphics")
	int AntiAliasingQuality;

	UPROPERTY(VisibleAnywhere, Category = "Settings|Graphics")
	int ViewDistanceQuality;

	UPROPERTY(VisibleAnywhere, Category = "Settings|Graphics")
	int PostProcessingQuality;

	UPROPERTY(VisibleAnywhere, Category = "Settings|Graphics")
	int VisualEffectQuality;

	UPROPERTY(VisibleAnywhere, Category = "Settings|Graphics")
	int GlobalIlluminationQuality;

	UPROPERTY(VisibleAnywhere, Category = "Settings|Graphics")
	int ShadingQuality;

	UPROPERTY(VisibleAnywhere, Category = "Settings|Graphics")
	float FrameRateLimit = 120.0f;

	UPROPERTY(VisibleAnywhere, Category = "Settings|Graphics")
	float GammaValue = 2.2f;

	UPROPERTY(VisibleAnywhere, Category = "Settings|Graphics")
	TEnumAsByte<EWindowMode::Type> WindowMode = EWindowMode::WindowedFullscreen;

	UPROPERTY(VisibleAnywhere, Category = "Settings|Graphics")
	bool bLumenEnabled = true;

	UPROPERTY(VisibleAnywhere, Category = "Settings|Graphics")
	int32 OverallQualityPreset = 2;

	UPROPERTY(VisibleAnywhere, Category = "Settings|Audio")
	float MasterVolume = 1.0f;

	UPROPERTY(VisibleAnywhere, Category = "Settings|Audio")
	float MusicVolume = 0.8f;

	UPROPERTY(VisibleAnywhere, Category = "Settings|Audio")
	float SFXVolume = 1.0f;

	UPROPERTY(VisibleAnywhere, Category = "Settings|Audio")
	float UIVolume = 1.0f;

	UPROPERTY(VisibleAnywhere, Category = "Settings|Controls")
	float MouseSensitivity = 1.0f;

	UPROPERTY(VisibleAnywhere, Category = "Settings|Controls")
	float GamepadSensitivity = 1.0f;

	UPROPERTY(VisibleAnywhere, Category = "Settings|Controls")
	bool bInvertY = false;

	UPROPERTY(VisibleAnywhere, Category = "Settings|Gameplay")
	bool bVibrationEnabled = true;

	UPROPERTY(VisibleAnywhere, Category = "Settings|Gameplay")
	bool bCameraAssist = false;

	UPROPERTY(VisibleAnywhere, Category = "Settings|Accessibility")
	bool bSubtitlesEnabled = true;

	UPROPERTY(VisibleAnywhere, Category = "Settings|Accessibility")
	int32 SubtitleSizeIndex = 1;

	UPROPERTY(VisibleAnywhere, Category = "Settings|Language")
	FString LanguageCode = TEXT("es");
};
