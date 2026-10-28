#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Features/SaveSystem/Data/SaveTypes.h"
#include "GameplayTagContainer.h"
#include "PlayerProgressSaveGame.generated.h"

struct FEntitySaveData;
class AItem;

UCLASS()
class TESISUE_API UPlayerProgressSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPlayerProgressSaveGame();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SaveGameData|Level")
    FString CurrentLevelName;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SaveGameData|Metadata")
    FDateTime Timestamp;

    UPROPERTY(VisibleAnywhere, Category = "SaveGameData|Metadata")
    int32 SaveSlotIndex;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SaveGameData|Metadata")
    FString SaveSlotUserLabel;

    UPROPERTY(VisibleAnywhere, Category = "Save Data")
    TMap<FGuid, FEntitySaveData> SavedActors;

    // Niveles del árbol de atributos del jugador (UAttributeTreeComponent). Las ramas que no figuran están en Nivel I.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SaveGameData|Progression")
    TMap<FGameplayTag, int32> AttributeTreeLevels;

    // Combos de arma desbloqueados en el Altar de Sangre (UWeaponUpgradeComponent).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SaveGameData|Progression")
    FGameplayTagContainer UnlockedWeaponCombos;
};