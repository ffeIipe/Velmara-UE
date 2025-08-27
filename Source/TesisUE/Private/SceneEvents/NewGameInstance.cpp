#include "SceneEvents/NewGameInstance.h"
#include "LoadSystem/SettingsSaveGame.h"
#include "LoadSystem/PlayerProgressSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/Character.h"
#include "Components/MementoComponent.h"
#include "Components/PossessionComponent.h"
#include "Enemy/Enemy.h"
#include "Blueprint/UserWidget.h"
#include "Player/PlayerMain.h"
#include "Components/InventoryComponent.h"
#include "Items/Item.h"
#include "Items/Weapons/Sword.h"
#include <SceneEvents/NewGameStateBase.h>

#include "DataAssets/EntityData.h"

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

void UNewGameInstance::SetAntiAliasing(int32 NewQuality)
{
    if (CurrentSettings)
    {
        NewQuality = FMath::Clamp(NewQuality, 0, 3);
        CurrentSettings->AntiAliasingQuality = NewQuality;
        ApplyGraphicsSettings();
        SaveGameSettings();
    }
}

void UNewGameInstance::SetViewDistance(int32 NewQuality)
{
    if (CurrentSettings)
    {
        NewQuality = FMath::Clamp(NewQuality, 0, 3);
        CurrentSettings->ViewDistanceQuality = NewQuality;
        ApplyGraphicsSettings();
        SaveGameSettings();
    }
}

void UNewGameInstance::SetPostProcess(int32 NewQuality)
{
    if (CurrentSettings)
    {
        NewQuality = FMath::Clamp(NewQuality, 0, 3);
        CurrentSettings->PostProcessingQuality = NewQuality;
        ApplyGraphicsSettings();
        SaveGameSettings();
    }
}

void UNewGameInstance::SetVisualEffect(int32 NewQuality)
{
    if (CurrentSettings)
    {
        NewQuality = FMath::Clamp(NewQuality, 0, 3);
        CurrentSettings->VisualEffectQuality = NewQuality;
        ApplyGraphicsSettings();
        SaveGameSettings();
    }
}

void UNewGameInstance::SetGlobalIllumination(int32 NewQuality)
{
    if (CurrentSettings)
    {
        NewQuality = FMath::Clamp(NewQuality, 0, 3);
        CurrentSettings->GlobalIlluminationQuality = NewQuality;
        ApplyGraphicsSettings();
        SaveGameSettings();
    }
}

void UNewGameInstance::SetShadingQuality(int32 NewQuality)
{
    if (CurrentSettings)
    {
        NewQuality = FMath::Clamp(NewQuality, 0, 3);
        CurrentSettings->ShadingQuality = NewQuality;
        ApplyGraphicsSettings();
        SaveGameSettings();
    }
}

void UNewGameInstance::SetFrameRateLimit(int32 NewFrameRateLimit)
{
    if (CurrentSettings)
    {
        CurrentSettings->FrameRateLimit = NewFrameRateLimit;
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

int32 UNewGameInstance::GetAntiAliasingQuality() const
{
    return CurrentSettings ? CurrentSettings->AntiAliasingQuality : 2;
}

int32 UNewGameInstance::GetViewDistanceQuality() const
{
    return CurrentSettings ? CurrentSettings->AntiAliasingQuality : 2;
}

int32 UNewGameInstance::GetPostProcessQuality() const
{
    return CurrentSettings ? CurrentSettings->PostProcessingQuality : 2;
}

int32 UNewGameInstance::GetGlobalIlluminationQuality() const
{
    return CurrentSettings ? CurrentSettings->GlobalIlluminationQuality : 2;
}

int32 UNewGameInstance::GetShadingQuality() const
{
    return CurrentSettings ? CurrentSettings->ShadingQuality : 2;
}

int32 UNewGameInstance::GetVisualEffectsQuality() const
{
    return CurrentSettings ? CurrentSettings->VisualEffectQuality : 2;
}

int32 UNewGameInstance::GetFrameRateLimit() const
{
    return CurrentSettings ? CurrentSettings->FrameRateLimit : 60;
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

    UGameplayStatics::OpenLevel(this, FName(*StartLevelName));
}

bool UNewGameInstance::SavePlayerProgress(int32 SlotIndex, APawn* Entity)
{
    ActiveSaveSlotIndex = FMath::Clamp(SlotIndex, 0, 2);
    FString CurrentSlotName = FString::Printf(TEXT("%s%d"), *ProgressSaveSlotPrefix, ActiveSaveSlotIndex);

    UPlayerProgressSaveGame* SaveGameInstance = Cast<UPlayerProgressSaveGame>(UGameplayStatics::CreateSaveGameObject(UPlayerProgressSaveGame::StaticClass()));
    if (!SaveGameInstance)
    {
        return false;
    }

    SaveGameInstance->CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this);

    APlayerMain* PlayerCharacter = Cast<APlayerMain>(Entity);

    if (PlayerCharacter)
    {
        //if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Cyan, FString("Player reference obtained from Player."));
    }
    else
    {
        if (AEnemy* EnemyRef = Cast<AEnemy>(Entity))
        {
            PlayerCharacter = Cast<APlayerMain>(EnemyRef->GetPossessionComponent()->GetPossessingEntity());
            //if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Magenta, FString("Player reference obtained from Enemy possessed."));
        }
        else
        {
            //if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, FString("Player reference not obtained."));
        }
    }

    UInventoryComponent* PlayerInventory = IsValid(PlayerCharacter) ? PlayerCharacter->GetInventoryComponent() : nullptr;

    if (IsValid(PlayerCharacter))
    {
        if (UMementoComponent* PlayerMemento = PlayerCharacter->GetMementoComponent())
        {
            PlayerMemento->SaveState();
            SaveGameInstance->PlayerState = PlayerMemento->GetCurrentSavedState();
        }

        if (PlayerInventory)
        {
            SaveGameInstance->InventorySlotsData.Empty();
            const TArray<AItem*>& CurrentInventoryItems = PlayerInventory->GetInventoryItems();
            AItem* CurrentlyEquippedItem = PlayerInventory->GetEquippedItem();

            for (AItem* ItemInSlot : CurrentInventoryItems)
            {
                FInventoryItemSaveData ItemData;
                if (IsValid(ItemInSlot))
                {
                    ItemData.ItemClass = ItemInSlot->GetClass();
                }
                else
                {
                    ItemData.ItemClass = nullptr;
                }
                SaveGameInstance->InventorySlotsData.Add(ItemData);
            }
            SaveGameInstance->EquippedSlotIndexInSave = CurrentInventoryItems.Find(CurrentlyEquippedItem);
        }
    }

    if (ANewGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState<ANewGameStateBase>() : nullptr)
    {
        GameState->GetAllEnemyStates(SaveGameInstance->EnemiesData);

        TArray<FInteractedItemSaveData> AllWorldItemStates;
        GameState->GetAllInteractedItemStates(AllWorldItemStates);

        if (PlayerInventory)
        {
            TSet<FName> InventoryItemIDs;
            for (AItem* ItemInInventory : PlayerInventory->GetInventoryItems())
            {
                if (IsValid(ItemInInventory))
                {
                    InventoryItemIDs.Add(ItemInInventory->GetUniqueSaveID());
                }
            }
        }

        SaveGameInstance->InteractedItemsData.Empty();

        for (const FInteractedItemSaveData& ItemState : AllWorldItemStates)
        {
            SaveGameInstance->InteractedItemsData.Add(ItemState);
        }
    }

    SaveGameInstance->Timestamp = FDateTime::UtcNow();
    SaveGameInstance->SaveSlotIndex = ActiveSaveSlotIndex;

    bool bSaveSuccess = UGameplayStatics::SaveGameToSlot(SaveGameInstance, CurrentSlotName, DefaultUserIndex);

    return bSaveSuccess;
}

bool UNewGameInstance::LoadPlayerProgress(int32 SlotIndex)
{
    ActiveSaveSlotIndex = FMath::Clamp(SlotIndex, 0, 2);
    FString CurrentSlotName = FString::Printf(TEXT("%s%d"), *ProgressSaveSlotPrefix, ActiveSaveSlotIndex);

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

void UNewGameInstance::ApplyPendingLoadedDataToWorld()
{
    if (!PendingGameDataToLoad)
    {
        bIsLoadingPlayerProgress = false;
        HideLoadingScreen();
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        PendingGameDataToLoad = nullptr;
        bIsLoadingPlayerProgress = false;
        HideLoadingScreen();
        return;
    }

    if (APlayerMain* PlayerCharacter = Cast<APlayerMain>(UGameplayStatics::GetPlayerCharacter(this, 0)))
    {
        if (UMementoComponent* PlayerMemento = PlayerCharacter->GetMementoComponent())
        {
            PlayerMemento->ApplyExternalState(PendingGameDataToLoad->PlayerState);
        }

        if (UInventoryComponent* PlayerInventory = PlayerCharacter->GetInventoryComponent())
        {
            PlayerInventory->InventorySlots.Init(nullptr, PlayerCharacter->EntityData->InventoryData.MaxSlots);

            FActorSpawnParameters SpawnParams;
            SpawnParams.Owner = PlayerCharacter;
            SpawnParams.Instigator = PlayerCharacter;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

            for (int32 i = 0; i < PendingGameDataToLoad->InventorySlotsData.Num(); ++i)
            {
                if (!PlayerInventory->InventorySlots.IsValidIndex(i)) continue;

                const FInventoryItemSaveData& SavedItemData = PendingGameDataToLoad->InventorySlotsData[i];
                if (SavedItemData.ItemClass != nullptr)
                {
                    if (AItem* NewItem = GetWorld()->SpawnActor<AItem>(SavedItemData.ItemClass, PlayerCharacter->GetActorLocation(), PlayerCharacter->GetActorRotation(), SpawnParams))
                    {
                        //GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, FString("Item reconciliated..."));
                        NewItem->SetOwner(PlayerCharacter);

                        if (UCharacterStateComponent* PlayerCharStateComp = PlayerCharacter->GetComponentByClass<UCharacterStateComponent>())
                        {
                            PlayerCharStateComp->SetCharacterState(ECharacterStates::ECS_EquippedSword);
                        }

                        PlayerInventory->InventorySlots[i] = NewItem;
                        if (ASword* Sword = Cast<ASword>(NewItem))
                        {
                            if (APlayerMain* Player = Cast<APlayerMain>(PlayerCharacter))
                            {
                                Sword->EnableSwordState(true);
                                Sword->OnWallHit.AddDynamic(Player, &APlayerMain::OnWallCollision);
                            }
                        }
                    }
                }
            }
            PlayerInventory->UpdateInventoryUI();

            if (PendingGameDataToLoad->EquippedSlotIndexInSave != -1 && PlayerInventory->InventorySlots.IsValidIndex(PendingGameDataToLoad->EquippedSlotIndexInSave) && PlayerInventory->InventorySlots[PendingGameDataToLoad->EquippedSlotIndexInSave] != nullptr)
            {
                PlayerInventory->EquipItemFromSlot(PendingGameDataToLoad->EquippedSlotIndexInSave);
            }
        }
    }

    if (ANewGameStateBase* GameState = World->GetGameState<ANewGameStateBase>())
    {
        GameState->InitializeWorldState(PendingGameDataToLoad->EnemiesData);
        GameState->InitializeWorldInteractedItemsState(PendingGameDataToLoad->InteractedItemsData);
    }

    PendingGameDataToLoad = nullptr;
    bIsLoadingPlayerProgress = false;
    HideLoadingScreen();
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

void UNewGameInstance::HideLoadingScreen()
{
    if (CurrentLoadingScreenInstance)
    {
        CurrentLoadingScreenInstance->RemoveFromParent();
        CurrentLoadingScreenInstance = nullptr;
    }
}