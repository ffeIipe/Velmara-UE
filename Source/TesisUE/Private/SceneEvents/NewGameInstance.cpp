#include "SceneEvents/NewGameInstance.h"
#include "LoadSystem/SettingsSaveGame.h"
#include "LoadSystem/PlayerProgressSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/Character.h"
#include "Components/MementoComponent.h"
#include "Components/AttributeComponent.h"
#include "Components/PossessionComponent.h"
#include "EngineUtils.h" 
#include "Enemy/Enemy.h"
#include "Blueprint/UserWidget.h"
#include "Player/PlayerMain.h"
#include "Components/InventoryComponent.h"
#include "Items/Item.h"
#include "Items/Weapons/Sword.h"
#include <SceneEvents/NewGameModeBase.h>
#include <SceneEvents/NewGameStateBase.h>

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
        if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Cyan, FString("Player reference obtained from Player."));
    }
    else
    {
        if (AEnemy* EnemyRef = Cast<AEnemy>(Entity))
        {
            PlayerCharacter = Cast<APlayerMain>(EnemyRef->GetPossessionComponent()->GetPossessedEntity());
            if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Magenta, FString("Player reference obtained from Enemy possessed."));
        }
        else
        {
            if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, FString("Player reference not obtained."));
        }
    }

    UInventoryComponent* PlayerInventory = IsValid(PlayerCharacter) ? PlayerCharacter->FindComponentByClass<UInventoryComponent>() : nullptr;

    if (IsValid(PlayerCharacter))
    {
        UMementoComponent* PlayerMemento = PlayerCharacter->FindComponentByClass<UMementoComponent>();
        if (PlayerMemento)
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

    ANewGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState<ANewGameStateBase>() : nullptr;
    if (GameState)
    {
        GameState->GetAllEnemyStates(SaveGameInstance->EnemiesData);

        TArray<FInteractedItemSaveData> AllWorldItemStates;
        GameState->GetAllInteractedItemStates(AllWorldItemStates);

        TSet<FName> InventoryItemIDs;
        if (PlayerInventory)
        {
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

    ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
    if (PlayerCharacter)
    {
        UMementoComponent* PlayerMemento = PlayerCharacter->FindComponentByClass<UMementoComponent>();
        if (PlayerMemento)
        {
            PlayerMemento->ApplyExternalState(PendingGameDataToLoad->PlayerState);
        }

        UInventoryComponent* PlayerInventory = PlayerCharacter->FindComponentByClass<UInventoryComponent>();
        if (PlayerInventory)
        {
            PlayerInventory->InventorySlots.Init(nullptr, PlayerInventory->MaxSlots);

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
                    AItem* NewItem = GetWorld()->SpawnActor<AItem>(SavedItemData.ItemClass, PlayerCharacter->GetActorLocation(), PlayerCharacter->GetActorRotation(), SpawnParams);
                    if (NewItem)
                    {
                        GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, FString("Item reconciliated..."));
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

    ANewGameStateBase* GameState = World->GetGameState<ANewGameStateBase>();
    if (GameState)
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
        APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
        if (PC)
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