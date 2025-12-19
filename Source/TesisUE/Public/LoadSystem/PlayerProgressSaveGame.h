#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Interfaces/SaveInterface.h"
#include "PlayerProgressSaveGame.generated.h"

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

    UPROPERTY()
    TMap<FName, FEntitySaveData> SavedActors;
};