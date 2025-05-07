// Fill out your copyright notice in the Description page of Project Settings.


#include "LoadSystem/SettingsSaveGame.h"

USettingsSaveGame::USettingsSaveGame()
{
    ScreenResolution = FIntPoint(1920, 1080);
    TextureQuality = 2;
    ShadowQuality = 2;

    SaveSlotName = TEXT("GameSettings");
    UserIndex = 0;
}