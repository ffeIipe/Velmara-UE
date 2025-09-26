#include "SceneEvents/VelmaraGameInstance.h"
#include "LoadSystem/SettingsSaveGame.h"
#include "LoadSystem/PlayerProgressSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameUserSettings.h"
#include "Blueprint/UserWidget.h"
#include <SceneEvents/VelmaraGameStateBase.h>

UVelmaraGameInstance::UVelmaraGameInstance()
{
    CurrentSettings = nullptr;
    ActiveSaveSlotIndex = 0;
    PendingGameDataToLoad = nullptr;

    CurrentLoadingScreenInstance = nullptr;
    bIsLoadingPlayerProgress = false;
}

void UVelmaraGameInstance::Init()
{
    Super::Init();
    LoadGameSettings();
}

void UVelmaraGameInstance::Shutdown()
{
    Super::Shutdown();
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
        }
        else
        {
            CurrentSettings->ScreenResolution = FIntPoint(1920, 1080); // Fallback
        }

        CurrentSettings->TextureQuality = 2; // Alto
        CurrentSettings->ShadowQuality = 2;  // Alto
        // ... otros defaults

        UE_LOG(LogTemp, Warning, TEXT("NewGameInstance: Setting default game settings."));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("NewGameInstance: Failed to create SaveGameObject for default settings."));
    }
}

void UVelmaraGameInstance::LoadGameSettings()
{
    if (USettingsSaveGame* LoadedSettings = Cast<USettingsSaveGame>(UGameplayStatics::LoadGameFromSlot(SettingsSlotName, DefaultUserIndex)))
    {
        CurrentSettings = LoadedSettings;
        UE_LOG(LogTemp, Log, TEXT("NewGameInstance: Game settings loaded successfully."));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("NewGameInstance: No saved game settings found or failed to load. Using defaults."));
        SetDefaultGameSettings();
        SaveGameSettings();
    }

    ApplyGraphicsSettings();
}

void UVelmaraGameInstance::SaveGameSettings()
{
    if (CurrentSettings)
    {
        UGameplayStatics::SaveGameToSlot(CurrentSettings, SettingsSlotName, DefaultUserIndex);
        UE_LOG(LogTemp, Log, TEXT("NewGameInstance: Game settings saved."));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("NewGameInstance: CurrentSettings is null, cannot save."));
    }
}

void UVelmaraGameInstance::ApplyGraphicsSettings()
{
    if (!CurrentSettings)
    {
        UE_LOG(LogTemp, Error, TEXT("NewGameInstance: CurrentSettings is null, cannot apply graphics settings."));
        return;
    }

    if (GEngine && GEngine->GameUserSettings)
    {
        UGameUserSettings* UserSettings = GEngine->GameUserSettings;

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

        UserSettings->ApplySettings(false);
        UE_LOG(LogTemp, Log, TEXT("NewGameInstance: Graphics settings applied. Resolution: %s, TextureQ: %d, ShadowQ: %d"),
            *CurrentSettings->ScreenResolution.ToString(), CurrentSettings->TextureQuality, CurrentSettings->ShadowQuality);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("NewGameInstance: GEngine or GameUserSettings is null."));
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

void UVelmaraGameInstance::SetTextureQuality(int32 NewQuality)
{
    if (CurrentSettings)
    {
        NewQuality = FMath::Clamp(NewQuality, 0, 3);
        CurrentSettings->TextureQuality = NewQuality;
        ApplyGraphicsSettings();
        SaveGameSettings();
    }
}

void UVelmaraGameInstance::SetShadowQuality(int32 NewQuality)
{
    if (CurrentSettings)
    {
        NewQuality = FMath::Clamp(NewQuality, 0, 3);
        CurrentSettings->ShadowQuality = NewQuality;
        ApplyGraphicsSettings();

        if (NewQuality == 0)
        {
            GEngine->Exec(nullptr, TEXT("r.ShadowQuality 1"));
            GEngine->Exec(nullptr, TEXT("r.Shadow.CSM.MaxCascades 1"));
            GEngine->Exec(nullptr, TEXT("r.Shadow.MaxResolution 512"));
            GEngine->Exec(nullptr, TEXT("r.Shadow.RadiusThreshold 0.08"));
            GEngine->Exec(nullptr, TEXT("r.Shadow.DistanceScale 0.6"));
 
        }
        SaveGameSettings();
    }
}

void UVelmaraGameInstance::SetAntiAliasing(int32 NewQuality)
{
    if (CurrentSettings)
    {
        NewQuality = FMath::Clamp(NewQuality, 0, 3);
        CurrentSettings->AntiAliasingQuality = NewQuality;
        ApplyGraphicsSettings();
        SaveGameSettings();
    }
}

void UVelmaraGameInstance::SetViewDistance(int32 NewQuality)
{
    if (CurrentSettings)
    {
        NewQuality = FMath::Clamp(NewQuality, 0, 3);
        CurrentSettings->ViewDistanceQuality = NewQuality;
        ApplyGraphicsSettings();
        SaveGameSettings();
    }
}

void UVelmaraGameInstance::SetPostProcess(int32 NewQuality)
{
    if (CurrentSettings)
    {
        NewQuality = FMath::Clamp(NewQuality, 0, 3);
        CurrentSettings->PostProcessingQuality = NewQuality;
        ApplyGraphicsSettings();
        SaveGameSettings();
    }
}

void UVelmaraGameInstance::SetVisualEffect(int32 NewQuality)
{
    if (CurrentSettings)
    {
        NewQuality = FMath::Clamp(NewQuality, 0, 3);
        CurrentSettings->VisualEffectQuality = NewQuality;
        ApplyGraphicsSettings();
        SaveGameSettings();
    }
}

void UVelmaraGameInstance::SetGlobalIllumination(int32 NewQuality)
{
    if (CurrentSettings)
    {
        NewQuality = FMath::Clamp(NewQuality, 0, 3);
        CurrentSettings->GlobalIlluminationQuality = NewQuality;
        ApplyGraphicsSettings();
        SaveGameSettings();
    }
}

void UVelmaraGameInstance::SetShadingQuality(int32 NewQuality)
{
    if (CurrentSettings)
    {
        NewQuality = FMath::Clamp(NewQuality, 0, 3);
        CurrentSettings->ShadingQuality = NewQuality;
        ApplyGraphicsSettings();
        SaveGameSettings();
    }
}

void UVelmaraGameInstance::SetFrameRateLimit(int32 NewFrameRateLimit)
{
    if (CurrentSettings)
    {
        CurrentSettings->FrameRateLimit = NewFrameRateLimit;
        ApplyGraphicsSettings();
        SaveGameSettings();
    }
}

FIntPoint UVelmaraGameInstance::GetScreenResolution() const
{
    return CurrentSettings ? CurrentSettings->ScreenResolution : FIntPoint(0, 0);
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
    return CurrentSettings ? CurrentSettings->AntiAliasingQuality : 2;
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

int32 UVelmaraGameInstance::GetFrameRateLimit() const
{
    return CurrentSettings ? CurrentSettings->FrameRateLimit : 60;
}

void UVelmaraGameInstance::CreateNewGame(int32 SlotIndex, FString StartLevelName)
{
    ActiveSaveSlotIndex = FMath::Clamp(SlotIndex, 0, 2);
    PendingGameDataToLoad = nullptr;

    // FString SlotNameToDelete = FString::Printf(TEXT("%s%d"), *ProgressSaveSlotPrefix, ActiveSaveSlotIndex);
    // if (UGameplayStatics::DoesSaveGameExist(SlotNameToDelete, DefaultUserIndex))
    // {
    //     UGameplayStatics::DeleteGameInSlot(SlotNameToDelete, DefaultUserIndex);
    // }

    UGameplayStatics::OpenLevel(this, FName(*StartLevelName));
}

bool UVelmaraGameInstance::SavePlayerProgress(const int32 SlotIndex, APawn* Pawn)
{
    if (Pawn->GetController() == UGameplayStatics::GetPlayerController(this, 0))
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Purple, "Proceed to Save Player progress.");

        ActiveSaveSlotIndex = FMath::Clamp(SlotIndex, 0, 2);
        const FString CurrentSlotName = FString::Printf(TEXT("%s%d"), *ProgressSaveSlotPrefix, ActiveSaveSlotIndex);

        UPlayerProgressSaveGame* SaveGameInstance = Cast<UPlayerProgressSaveGame>(UGameplayStatics::CreateSaveGameObject(UPlayerProgressSaveGame::StaticClass()));
        if (!SaveGameInstance)
        {
            if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "Failed Cast to UPlayerSaveProgress.");
            return false;
        }

        SaveGameInstance->CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this);

        if (AVelmaraGameStateBase* GameState = GetWorld()->GetGameState<AVelmaraGameStateBase>())
        {
            if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Green, "Game State looking for Memento Entities States...");
            
            SaveGameInstance->EntitiesStates = GameState->SaveAllEntityMementoStates();
            SaveGameInstance->ItemsStates = GameState->SaveAllItemMementoStates();
        }

        SaveGameInstance->Timestamp = FDateTime::UtcNow();
        SaveGameInstance->SaveSlotIndex = ActiveSaveSlotIndex;

        const bool bSaveSuccess = UGameplayStatics::SaveGameToSlot(SaveGameInstance, CurrentSlotName, DefaultUserIndex);
        return bSaveSuccess;
    }

    if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "Failed to Save Data.");

    return false;
}

bool UVelmaraGameInstance::LoadPlayerProgress(int32 SlotIndex)
{
    ActiveSaveSlotIndex = FMath::Clamp(SlotIndex, 0, 2);
    const FString CurrentSlotName = FString::Printf(TEXT("%s%d"), *ProgressSaveSlotPrefix, ActiveSaveSlotIndex);

    if (!DoesProgressSaveExist(ActiveSaveSlotIndex))
    {
        bIsLoadingPlayerProgress = false;
        return false;
    }

    PendingGameDataToLoad = Cast<UPlayerProgressSaveGame>(UGameplayStatics::LoadGameFromSlot(CurrentSlotName, DefaultUserIndex));

    if (PendingGameDataToLoad)
    {
        bIsLoadingPlayerProgress = true;
        ShowLoadingScreen();

        UGameplayStatics::OpenLevel(this, FName(*PendingGameDataToLoad->CurrentLevelName));
        return true;
    }

    bIsLoadingPlayerProgress = false;
    return false;
}

void UVelmaraGameInstance::ApplyPendingLoadedDataToWorld()
{
    if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Cyan, "Applying Pending Data to world.");
    
    if (!PendingGameDataToLoad)
    {
        bIsLoadingPlayerProgress = false;
        HideLoadingScreen();
        if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "Non-Pending Data to apply to world.");
        
        return;
    }

    const UWorld* World = GetWorld();
    if (!World)
    {
        PendingGameDataToLoad = nullptr;
        bIsLoadingPlayerProgress = false;
        HideLoadingScreen();
        return;
    }

    if (AVelmaraGameStateBase* GameState = World->GetGameState<AVelmaraGameStateBase>())
    {
        GameState->InitializeEntities(PendingGameDataToLoad->EntitiesStates);
        GameState->InitializeItems(PendingGameDataToLoad->ItemsStates);
    }

    PendingGameDataToLoad = nullptr;
    bIsLoadingPlayerProgress = false;
    HideLoadingScreen();
}

bool UVelmaraGameInstance::DoesProgressSaveExist(int32 SlotIndex) const
{
    FString CurrentSlotName = FString::Printf(TEXT("%s%d"), *ProgressSaveSlotPrefix, FMath::Clamp(SlotIndex, 0, 2));
    return UGameplayStatics::DoesSaveGameExist(CurrentSlotName, DefaultUserIndex);
}

UPlayerProgressSaveGame* UVelmaraGameInstance::GetSaveGameInfo(int32 SlotIndex) const
{
    FString CurrentSlotName = FString::Printf(TEXT("%s%d"), *ProgressSaveSlotPrefix, FMath::Clamp(SlotIndex, 0, 2));
    if (DoesProgressSaveExist(SlotIndex))
    {
        return Cast<UPlayerProgressSaveGame>(UGameplayStatics::LoadGameFromSlot(CurrentSlotName, DefaultUserIndex));
    }
    return nullptr;
}

void UVelmaraGameInstance::ShowLoadingScreen()
{
    if (LoadingScreenWidgetClass && !CurrentLoadingScreenInstance)
    {
        if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
        {
            CurrentLoadingScreenInstance = CreateWidget<UUserWidget>(PC, LoadingScreenWidgetClass);
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