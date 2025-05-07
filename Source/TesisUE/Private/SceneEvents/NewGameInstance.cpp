#include "SceneEvents/NewGameInstance.h"
#include "LoadSystem/SettingsSaveGame.h"
#include "LoadSystem/PlayerProgressSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/Character.h"
#include "Components/MementoComponent.h"
#include "EngineUtils.h" 
#include "Enemy/Enemy.h"
#include "Blueprint/UserWidget.h"

UNewGameInstance::UNewGameInstance()
{
    CurrentSettings = nullptr;
    ActiveSaveSlotIndex = 0;
    PendingGameDataToLoad = nullptr;

    CurrentLoadingScreenInstance = nullptr;
    bIsLoadingPlayerProgress = false;
}

void UNewGameInstance::Init()
{
    Super::Init();
    LoadGameSettings();
}

void UNewGameInstance::Shutdown()
{
    Super::Shutdown();
}

void UNewGameInstance::SetDefaultGameSettings()
{
    if (!CurrentSettings)
    {
        CurrentSettings = Cast<USettingsSaveGame>(UGameplayStatics::CreateSaveGameObject(USettingsSaveGame::StaticClass()));
    }

    if (CurrentSettings)
    {
        // Obtener resolución del escritorio como un buen valor por defecto
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

void UNewGameInstance::LoadGameSettings()
{
    USettingsSaveGame* LoadedSettings = Cast<USettingsSaveGame>(UGameplayStatics::LoadGameFromSlot(SettingsSlotName, DefaultUserIndex));

    if (LoadedSettings)
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

void UNewGameInstance::SaveGameSettings()
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

void UNewGameInstance::ApplyGraphicsSettings()
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

        // Para calidad de texturas y sombras, usamos los grupos de escalabilidad.
        // Los valores suelen ser 0 (Bajo), 1 (Medio), 2 (Alto), 3 (Épico).
        // Puedes ajustar esto según cómo hayas definido tus niveles de calidad.
        UserSettings->SetTextureQuality(CurrentSettings->TextureQuality);
        UserSettings->SetShadowQuality(CurrentSettings->ShadowQuality);
        // Podrías añadir más aquí: AntiAliasingQuality, ViewDistanceQuality, etc.
        // UserSettings->SetAntiAliasingQuality(CurrentSettings->AntiAliasingQuality); 
        // UserSettings->SetViewDistanceQuality(CurrentSettings->ViewDistanceQuality);
        // UserSettings->SetPostProcessingQuality(CurrentSettings->PostProcessingQuality);
        // UserSettings->SetVisualEffectQuality(CurrentSettings->VisualEffectQuality);
        // UserSettings->SetFoliageQuality(CurrentSettings->FoliageQuality);

        UserSettings->ApplySettings(false); // false para aplicar inmediatamente sin confirmación
        UE_LOG(LogTemp, Log, TEXT("NewGameInstance: Graphics settings applied. Resolution: %s, TextureQ: %d, ShadowQ: %d"),
            *CurrentSettings->ScreenResolution.ToString(), CurrentSettings->TextureQuality, CurrentSettings->ShadowQuality);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("NewGameInstance: GEngine or GameUserSettings is null."));
    }
}

void UNewGameInstance::SetScreenResolution(FIntPoint NewResolution)
{
    if (CurrentSettings)
    {
        CurrentSettings->ScreenResolution = NewResolution;
        ApplyGraphicsSettings();
        SaveGameSettings();
    }
}

void UNewGameInstance::SetTextureQuality(int32 NewQuality)
{
    if (CurrentSettings)
    {
        NewQuality = FMath::Clamp(NewQuality, 0, 3);
        CurrentSettings->TextureQuality = NewQuality;
        ApplyGraphicsSettings();
        SaveGameSettings();
    }
}

void UNewGameInstance::SetShadowQuality(int32 NewQuality)
{
    if (CurrentSettings)
    {
        NewQuality = FMath::Clamp(NewQuality, 0, 3);
        CurrentSettings->ShadowQuality = NewQuality;
        ApplyGraphicsSettings();
        SaveGameSettings();
    }
}

FIntPoint UNewGameInstance::GetScreenResolution() const
{
    return CurrentSettings ? CurrentSettings->ScreenResolution : FIntPoint(0, 0);
}

int32 UNewGameInstance::GetTextureQuality() const
{
    return CurrentSettings ? CurrentSettings->TextureQuality : 2;
}

int32 UNewGameInstance::GetShadowQuality() const
{
    return CurrentSettings ? CurrentSettings->ShadowQuality : 2;
}

void UNewGameInstance::CreateNewGame(int32 SlotIndex, FString StartLevelName)
{
    ActiveSaveSlotIndex = FMath::Clamp(SlotIndex, 0, 2);
    PendingGameDataToLoad = nullptr;

    // FString SlotNameToDelete = FString::Printf(TEXT("%s%d"), *ProgressSaveSlotPrefix, ActiveSaveSlotIndex);
    // if (UGameplayStatics::DoesSaveGameExist(SlotNameToDelete, DefaultUserIndex))
    // {
    //     UGameplayStatics::DeleteGameInSlot(SlotNameToDelete, DefaultUserIndex);
    // }

    UE_LOG(LogTemp, Log, TEXT("NewGameInstance: Creating new game in Slot %d, starting level %s"), ActiveSaveSlotIndex, *StartLevelName);
    UGameplayStatics::OpenLevel(this, FName(*StartLevelName));
}

bool UNewGameInstance::SavePlayerProgress(int32 SlotIndex)
{
    ActiveSaveSlotIndex = FMath::Clamp(SlotIndex, 0, 2);
    FString CurrentSlotName = FString::Printf(TEXT("%s%d"), *ProgressSaveSlotPrefix, ActiveSaveSlotIndex);

    UPlayerProgressSaveGame* SaveGameInstance = Cast<UPlayerProgressSaveGame>(UGameplayStatics::CreateSaveGameObject(UPlayerProgressSaveGame::StaticClass()));
    if (!SaveGameInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("NewGameInstance: Could not create SaveGameObject for PlayerProgress."));
        return false;
    }

    SaveGameInstance->CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this);

    ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
    if (PlayerCharacter)
    {
        UMementoComponent* PlayerMemento = PlayerCharacter->FindComponentByClass<UMementoComponent>();
        if (PlayerMemento)
        {
            PlayerMemento->SaveState();
            SaveGameInstance->PlayerState = PlayerMemento->GetCurrentSavedState();
            UE_LOG(LogTemp, Log, TEXT("NewGameInstance: Player state saved. Pos: %s"), *SaveGameInstance->PlayerState.Transform.GetLocation().ToString());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("NewGameInstance: PlayerCharacter does not have a MementoComponent. Player state not saved."));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("NewGameInstance: PlayerCharacter not found. Player state not saved."));
    }

    SaveGameInstance->EnemiesData.Empty();
    UWorld* CurrentWorld = GetWorld();
    if (CurrentWorld)
    {
        for (TActorIterator<AActor> It(CurrentWorld); It; ++It)
        {
            AActor* Actor = *It;
            if (Actor && Actor != PlayerCharacter)
            {
                UMementoComponent* EnemyMemento = Actor->FindComponentByClass<UMementoComponent>();
                if (EnemyMemento)
                {
                    EnemyMemento->SaveState();
                    FEnemySaveData EnemyData;
                    EnemyData.EnemyID = Actor->GetFName(); // ¡IMPORTANTE! Esto solo funciona bien para enemigos con nombres únicos persistentes en el nivel.
                    // Para enemigos spawneados dinámicamente, necesitarás un sistema de ID más robusto.
                    EnemyData.EnemyState = EnemyMemento->GetCurrentSavedState();
                    // Determinar si está "vivo" (podrías tener una función IsAlive() en AttributeComponent o en el enemigo mismo)
                    // Por ahora, asumiremos que si tiene Memento y se guarda, está "vivo" en algún estado.
                    // Podrías querer chequear su vida actual desde EnemyState.Health.
                    EnemyData.bIsAlive = EnemyData.EnemyState.Health > 0;

                    SaveGameInstance->EnemiesData.Add(EnemyData);
                }
            }
        }
        UE_LOG(LogTemp, Log, TEXT("NewGameInstance: Saved data for %d enemies."), SaveGameInstance->EnemiesData.Num());
    }

    SaveGameInstance->Timestamp = FDateTime::UtcNow();
    SaveGameInstance->SaveSlotIndex = ActiveSaveSlotIndex;

    if (UGameplayStatics::SaveGameToSlot(SaveGameInstance, CurrentSlotName, DefaultUserIndex))
    {
        UE_LOG(LogTemp, Log, TEXT("NewGameInstance: Player progress saved successfully to slot %s."), *CurrentSlotName);
        return true;
    }

    UE_LOG(LogTemp, Error, TEXT("NewGameInstance: Failed to save player progress to slot %s."), *CurrentSlotName);
    return false;
}

bool UNewGameInstance::LoadPlayerProgress(int32 SlotIndex)
{
    ActiveSaveSlotIndex = FMath::Clamp(SlotIndex, 0, 2);
    FString CurrentSlotName = FString::Printf(TEXT("%s%d"), *ProgressSaveSlotPrefix, ActiveSaveSlotIndex);

    if (!DoesProgressSaveExist(ActiveSaveSlotIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("NewGameInstance: No save game found in slot %s."), *CurrentSlotName);
        bIsLoadingPlayerProgress = false;
        return false;
    }

    PendingGameDataToLoad = Cast<UPlayerProgressSaveGame>(UGameplayStatics::LoadGameFromSlot(CurrentSlotName, DefaultUserIndex));

    if (PendingGameDataToLoad)
    {
        bIsLoadingPlayerProgress = true;
        ShowLoadingScreen();

        UE_LOG(LogTemp, Log, TEXT("NewGameInstance: Save game loaded from slot %s. Opening level %s."), *CurrentSlotName, *PendingGameDataToLoad->CurrentLevelName);
        UGameplayStatics::OpenLevel(this, FName(*PendingGameDataToLoad->CurrentLevelName));
        return true;
    }

    UE_LOG(LogTemp, Error, TEXT("NewGameInstance: Failed to load save game from slot %s or cast failed."), *CurrentSlotName);
    bIsLoadingPlayerProgress = false;
    return false;
}

void UNewGameInstance::ApplyPendingLoadedDataToWorld()
{
    if (!PendingGameDataToLoad)
    {
        UE_LOG(LogTemp, Log, TEXT("NewGameInstance::ApplyPendingLoadedDataToWorld: No pending data to apply."));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("NewGameInstance::ApplyPendingLoadedDataToWorld: Applying data for level %s"), *PendingGameDataToLoad->CurrentLevelName);

    ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
    if (PlayerCharacter)
    {
        UMementoComponent* PlayerMemento = PlayerCharacter->FindComponentByClass<UMementoComponent>();
        if (PlayerMemento)
        {
            PlayerMemento->ApplyExternalState(PendingGameDataToLoad->PlayerState);
            UE_LOG(LogTemp, Log, TEXT("NewGameInstance: Player state applied. New Pos: %s"), *PlayerCharacter->GetActorLocation().ToString());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("NewGameInstance: PlayerCharacter has no MementoComponent. Cannot apply saved state."));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("NewGameInstance: PlayerCharacter not found in ApplyPendingLoadedDataToWorld."));
    }

    UWorld* CurrentWorld = GetWorld();
    if (CurrentWorld)
    {
        for (const FEnemySaveData& SavedEnemyData : PendingGameDataToLoad->EnemiesData)
        {
            AActor* FoundEnemy = nullptr;
            for (TActorIterator<AActor> It(CurrentWorld); It; ++It)
            {
                AActor* PotentialEnemy = *It;
                if (PotentialEnemy && PotentialEnemy != PlayerCharacter && PotentialEnemy->GetFName() == SavedEnemyData.EnemyID)
                {
                    FoundEnemy = PotentialEnemy;
                    break;
                }
            }

            if (FoundEnemy)
            {
                if (SavedEnemyData.bIsAlive)
                {
                    UMementoComponent* EnemyMemento = FoundEnemy->FindComponentByClass<UMementoComponent>();
                    if (EnemyMemento)
                    {
                        EnemyMemento->ApplyExternalState(SavedEnemyData.EnemyState);
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("NewGameInstance: Enemy %s found but has no MementoComponent. State partially/not applied."), *FoundEnemy->GetName());
                    }
                }
                else
                {
                    FoundEnemy->Destroy();
                }
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("NewGameInstance: Finished applying loaded data."));
    PendingGameDataToLoad = nullptr;

    bIsLoadingPlayerProgress = false;
}

bool UNewGameInstance::DoesProgressSaveExist(int32 SlotIndex) const
{
    FString CurrentSlotName = FString::Printf(TEXT("%s%d"), *ProgressSaveSlotPrefix, FMath::Clamp(SlotIndex, 0, 2));
    return UGameplayStatics::DoesSaveGameExist(CurrentSlotName, DefaultUserIndex);
}

UPlayerProgressSaveGame* UNewGameInstance::GetSaveGameInfo(int32 SlotIndex) const
{
    FString CurrentSlotName = FString::Printf(TEXT("%s%d"), *ProgressSaveSlotPrefix, FMath::Clamp(SlotIndex, 0, 2));
    if (DoesProgressSaveExist(SlotIndex))
    {
        return Cast<UPlayerProgressSaveGame>(UGameplayStatics::LoadGameFromSlot(CurrentSlotName, DefaultUserIndex));
    }
    return nullptr;
}

void UNewGameInstance::ShowLoadingScreen()
{
    if (LoadingScreenWidgetClass && !CurrentLoadingScreenInstance)
    {
        APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
        if (PC)
        {
            CurrentLoadingScreenInstance = CreateWidget<UUserWidget>(PC, LoadingScreenWidgetClass);
            if (CurrentLoadingScreenInstance)
            {
                CurrentLoadingScreenInstance->AddToViewport(100);
                UE_LOG(LogTemp, Log, TEXT("NewGameInstance: Showing Loading Screen."));
            }
        }
    }
}

void UNewGameInstance::HideLoadingScreen()
{
    if (CurrentLoadingScreenInstance)
    {
        CurrentLoadingScreenInstance->RemoveFromParent();
        CurrentLoadingScreenInstance = nullptr;
        UE_LOG(LogTemp, Log, TEXT("NewGameInstance: Hiding Loading Screen."));
    }
}