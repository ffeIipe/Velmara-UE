#include "SceneEvents/VelmaraGameInstance.h"
#include "EngineUtils.h"
#include "LoadSystem/SettingsSaveGame.h"
#include "LoadSystem/PlayerProgressSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameUserSettings.h"
#include "Blueprint/UserWidget.h"
#include "GAS/VelmaraGameplayTags.h"

class FActorIterator;

UVelmaraGameInstance::UVelmaraGameInstance()
{
    CurrentSettings = nullptr;
	ActiveSaveSlotIndex = 0;
	CurrentLoadingScreenInstance = nullptr;
	CurrentSlotName = TEXT("GameSettings");
	UserIndex = 0;
    PendingSaveData = nullptr;

    FVelmaraGameplayTags::InitializeGameplayTags(); //initialize basic tags
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
    if (USettingsSaveGame* LoadedSettings = Cast<USettingsSaveGame>(UGameplayStatics::LoadGameFromSlot(SettingsSlotName, 0)))
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
        UGameplayStatics::SaveGameToSlot(CurrentSettings, SettingsSlotName, 0);
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

void UVelmaraGameInstance::SaveGame()
{
	UPlayerProgressSaveGame* SaveGameObject = Cast<UPlayerProgressSaveGame>(UGameplayStatics::CreateSaveGameObject(UPlayerProgressSaveGame::StaticClass()));

	for (FActorIterator It(GetWorld()); It; ++It)
	{
		if (AActor* Actor = *It; Actor && Actor->Implements<USaveInterface>())
		{
			FEntitySaveData ActorData;
			ActorData.UniqueSaveID = ISaveInterface::Execute_GetUniqueSaveID(Actor);
			ActorData.Transform = Actor->GetTransform();

			ISaveInterface::Execute_OnSaveGame(Actor, ActorData);
			
			SaveGameObject->SavedActors.Add(ActorData.UniqueSaveID, ActorData);

			//if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Emerald, ActorData.UniqueSaveID.ToString() + " found from: " + Actor->GetName());
		}
	}

	if (SaveGameObject)
	{
		SaveGameObject->CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this);
		SaveGameObject->Timestamp = FDateTime::UtcNow();
		SaveGameObject->SaveSlotIndex = ActiveSaveSlotIndex;
	}
    
	CurrentSlotName = FString::Printf(TEXT("%s%d"), *ProgressSaveSlotPrefix, ActiveSaveSlotIndex);
	UGameplayStatics::SaveGameToSlot(SaveGameObject, CurrentSlotName, DefaultUserIndex);

	if (OnSaveDataUpdated.IsBound()) OnSaveDataUpdated.Broadcast();
}

void UVelmaraGameInstance::LoadGame(const int32 SlotIndex)
{
	const FString SlotName = GetSlotNameByIndex(SlotIndex);
	if (!UGameplayStatics::DoesSaveGameExist(SlotName, DefaultUserIndex)) return;

	PendingSaveData = Cast<UPlayerProgressSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, DefaultUserIndex));
    
	if (PendingSaveData)
	{
		UGameplayStatics::OpenLevel(this, FName(*PendingSaveData->CurrentLevelName));
	}
}

void UVelmaraGameInstance::RestoreLoadedData()
{
    if (!PendingSaveData) return;

    for (FActorIterator It(GetWorld()); It; ++It)
    {
        if (It->Implements<USaveInterface>())
        {
            FName UniqueID = ISaveInterface::Execute_GetUniqueSaveID(*It);
            
            if (PendingSaveData->SavedActors.Contains(UniqueID))
            {
                const FEntitySaveData& ActorData = PendingSaveData->SavedActors[UniqueID];
                
                It->SetActorTransform(ActorData.Transform);
                ISaveInterface::Execute_OnLoadGame(*It, ActorData);

                //if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, "Restored: " + It->GetName());
            }
        }
    }

    PendingSaveData = nullptr; 
}

void UVelmaraGameInstance::CreateNewGame(const int32 SlotIndex, const FString StartLevelName)
{
	ActiveSaveSlotIndex = FMath::Clamp(SlotIndex, 0, 2);

	if (const FString SlotNameToDelete = FString::Printf(TEXT("%s%d"), *ProgressSaveSlotPrefix, ActiveSaveSlotIndex);
		UGameplayStatics::DoesSaveGameExist(SlotNameToDelete, DefaultUserIndex))
	{
	    UGameplayStatics::DeleteGameInSlot(SlotNameToDelete, DefaultUserIndex);
	}

	UGameplayStatics::OpenLevel(this, FName(*StartLevelName));
}

bool UVelmaraGameInstance::DoesProgressSaveExist(const int32 SlotIndex) const
{
	return UGameplayStatics::DoesSaveGameExist(GetSlotNameByIndex(SlotIndex), DefaultUserIndex);
}

FString UVelmaraGameInstance::GetSlotNameByIndex(const int32 SlotIndex) const
{
	return FString::Printf(TEXT("%s%d"), *ProgressSaveSlotPrefix, FMath::Clamp(SlotIndex, 0, 2));
}

UPlayerProgressSaveGame* UVelmaraGameInstance::GetSaveGameInfo(const int32 SlotIndex) const
{
	if (DoesProgressSaveExist(SlotIndex))
	{
		return Cast<UPlayerProgressSaveGame>(UGameplayStatics::LoadGameFromSlot(GetSlotNameByIndex(SlotIndex), DefaultUserIndex));
	}
	return nullptr;
}