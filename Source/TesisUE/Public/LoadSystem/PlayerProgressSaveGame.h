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

    UPROPERTY(VisibleAnywhere, Category = "SaveGameData")
    FName EnemyID;

    UPROPERTY(VisibleAnywhere, Category = "SaveGameData")
    TSubclassOf<AEnemy> EnemyClass;

    UPROPERTY(VisibleAnywhere, Category = "SaveGameData")
    FEntityMementoState EnemyState;

    UPROPERTY(VisibleAnywhere, Category = "SaveGameData")
    bool bIsAlive;

    FEnemySaveData()
        : EnemyID(NAME_None)
        , EnemyClass(nullptr)
        , bIsAlive(true)
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

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SaveGameData|Metadata")
    FDateTime Timestamp;

    UPROPERTY(VisibleAnywhere, Category = "SaveGameData|Metadata")
    int32 SaveSlotIndex;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SaveGameData|Metadata")
    FString SaveSlotUserLabel;
};