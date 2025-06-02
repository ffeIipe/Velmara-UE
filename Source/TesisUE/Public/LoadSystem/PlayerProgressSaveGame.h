#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Components/MementoComponent.h"
#include "Enemy/Enemy.h"
#include "PlayerProgressSaveGame.generated.h"

class AItem;

USTRUCT(BlueprintType)
struct FInventoryItemSaveData
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, Category = "SaveGameData")
    TSubclassOf<AItem> ItemClass;

    FInventoryItemSaveData() : ItemClass(nullptr) {}
};

USTRUCT(BlueprintType)
struct FEnemySaveData
{
    GENERATED_BODY()

    UPROPERTY(SaveGame)
    FName UniqueSaveID;

    UPROPERTY(VisibleAnywhere, Category = "SaveGameData")
    TSubclassOf<AEnemy> EnemyClass;

    UPROPERTY(VisibleAnywhere, Category = "SaveGameData")
    FEntityMementoState EnemyState;

    UPROPERTY(VisibleAnywhere, Category = "SaveGameData")
    bool bIsAlive;

    FEnemySaveData()
        : UniqueSaveID(NAME_None)
        , EnemyClass(nullptr)
        , bIsAlive(true)
    {}
};

USTRUCT(BlueprintType)
struct FInteractedItemSaveData
{
    GENERATED_BODY()

    UPROPERTY(SaveGame)
    FName UniqueSaveID;

    UPROPERTY(SaveGame)
    bool bWasOpened;

    FInteractedItemSaveData()
        : UniqueSaveID(NAME_None)
        , bWasOpened(false)
    {}
};

UCLASS()
class TESISUE_API UPlayerProgressSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPlayerProgressSaveGame();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SaveGameData|Level")
    FString CurrentLevelName;

    UPROPERTY(VisibleAnywhere, Category = "SaveGameData|Player")
    FEntityMementoState PlayerState;

    UPROPERTY(VisibleAnywhere, Category = "SaveGameData|Inventory")
    TArray<FInventoryItemSaveData> InventorySlotsData;

    UPROPERTY(VisibleAnywhere, Category = "SaveGameData|Inventory")
    int32 EquippedSlotIndexInSave;

    UPROPERTY(VisibleAnywhere, Category = "SaveGameData|Enemies")
    TArray<FEnemySaveData> EnemiesData;

    UPROPERTY(VisibleAnywhere, Category = "SaveGameData|InteractedItems")
    TArray<FInteractedItemSaveData> InteractedItemsData;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SaveGameData|Metadata")
    FDateTime Timestamp;

    UPROPERTY(VisibleAnywhere, Category = "SaveGameData|Metadata")
    int32 SaveSlotIndex;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SaveGameData|Metadata")
    FString SaveSlotUserLabel;
};