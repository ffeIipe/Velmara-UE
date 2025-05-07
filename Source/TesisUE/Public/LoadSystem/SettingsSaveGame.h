#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SettingsSaveGame.generated.h"

UCLASS()
class TESISUE_API USettingsSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    USettingsSaveGame();

    UPROPERTY(VisibleAnywhere, Category = "Settings|Graphics")
    FIntPoint ScreenResolution;

    UPROPERTY(VisibleAnywhere, Category = "Settings|Graphics")
    int32 TextureQuality;

    UPROPERTY(VisibleAnywhere, Category = "Settings|Graphics")
    int32 ShadowQuality;

    UPROPERTY(VisibleAnywhere, Category = "Metadata")
    FString SaveSlotName;

    UPROPERTY(VisibleAnywhere, Category = "Metadata")
    uint32 UserIndex;
};
