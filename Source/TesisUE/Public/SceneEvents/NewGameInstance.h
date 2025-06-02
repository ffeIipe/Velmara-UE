#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "NewGameInstance.generated.h"

class USettingsSaveGame;
class UPlayerProgressSaveGame;
class ACharacter;
class AEnemy;

UCLASS()
class TESISUE_API UNewGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    UNewGameInstance();

    virtual void Init() override;
    virtual void Shutdown() override;

    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SetScreenResolution(FIntPoint NewResolution);

    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SetTextureQuality(int32 NewQuality);

    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SetShadowQuality(int32 NewQuality);

    UFUNCTION(BlueprintPure, Category = "Settings")
    FIntPoint GetScreenResolution() const;

    UFUNCTION(BlueprintPure, Category = "Settings")
    int32 GetTextureQuality() const;

    UFUNCTION(BlueprintPure, Category = "Settings")
    int32 GetShadowQuality() const;

    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SaveGameSettings();

    UFUNCTION(BlueprintCallable, Category = "Settings")
    void LoadGameSettings();

    UFUNCTION(BlueprintCallable, Category = "Settings")
    void ApplyGraphicsSettings();

    UPROPERTY(BlueprintReadOnly, Category = "Settings")
    USettingsSaveGame* CurrentSettings;

    UPROPERTY(BlueprintReadWrite, Category = "SaveLoad|Progress")
    int32 ActiveSaveSlotIndex;

    UPROPERTY()
    UPlayerProgressSaveGame* PendingGameDataToLoad;

    UFUNCTION(BlueprintCallable, Category = "SaveLoad|Progress")
    void CreateNewGame(int32 SlotIndex, FString StartLevelName);

    UFUNCTION(BlueprintCallable, Category = "SaveLoad|Progress")
    bool SavePlayerProgress(int32 SlotIndex, APawn* Entity);

    UFUNCTION(BlueprintCallable, Category = "SaveLoad|Progress")
    bool LoadPlayerProgress(int32 SlotIndex);

    void ApplyPendingLoadedDataToWorld();

    UFUNCTION(BlueprintPure, Category = "SaveLoad|Progress")
    bool DoesProgressSaveExist(int32 SlotIndex) const;

    UFUNCTION(BlueprintCallable, Category = "SaveLoad|Progress")
    UPlayerProgressSaveGame* GetSaveGameInfo(int32 SlotIndex) const;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Loading")
    TSubclassOf<UUserWidget> LoadingScreenWidgetClass;

    UFUNCTION(BlueprintCallable, Category = "UI|Loading")
    void ShowLoadingScreen();

    UFUNCTION(BlueprintCallable, Category = "UI|Loading")
    void HideLoadingScreen();

    UPROPERTY(BlueprintReadOnly, Category = "SaveLoad|Progress")
    bool bIsLoadingPlayerProgress;

private:
    const FString SettingsSlotName = TEXT("GameSettings");

    void SetDefaultGameSettings();

    const FString ProgressSaveSlotPrefix = TEXT("PlayerProgressSlot_");
    const int32 DefaultUserIndex = 0;
    
    UPROPERTY()
    UUserWidget* CurrentLoadingScreenInstance;

    FString PlayerCharacterGetNameSafe(ACharacter* Char) const;
};