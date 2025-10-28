#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "VelmaraGameInstance.generated.h"

class USettingsSaveGame;
class UPlayerProgressSaveGame;
class ACharacter;
class AEnemy;

UCLASS()
class TESISUE_API UVelmaraGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    UVelmaraGameInstance();

    virtual void Init() override;
    virtual void Shutdown() override;

    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SetScreenResolution(FIntPoint NewResolution);

    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SetTextureQuality(int32 NewQuality);

    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SetShadowQuality(int32 NewQuality);

    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SetAntiAliasing(int32 NewQuality);

    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SetViewDistance(int32 NewQuality);
	
    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SetPostProcess(int32 NewQuality);
	
    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SetVisualEffect(int32 NewQuality);
	
    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SetGlobalIllumination(int32 NewQuality);

    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SetShadingQuality(int32 NewQuality);

    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SetFrameRateLimit(int32 NewFrameRateLimit);

    UFUNCTION(BlueprintPure, Category = "Settings")
    FIntPoint GetScreenResolution() const;

    UFUNCTION(BlueprintPure, Category = "Settings")
    int32 GetTextureQuality() const;

    UFUNCTION(BlueprintPure, Category = "Settings")
    int32 GetShadowQuality() const;
    
    UFUNCTION(BlueprintPure, Category = "Settings")
    int32 GetAntiAliasingQuality() const;

    UFUNCTION(BlueprintPure, Category = "Settings")
    int32 GetViewDistanceQuality() const;

    UFUNCTION(BlueprintPure, Category = "Settings")
    int32 GetPostProcessQuality() const;

    UFUNCTION(BlueprintPure, Category = "Settings")
    int32 GetGlobalIlluminationQuality() const;

    UFUNCTION(BlueprintPure, Category = "Settings")
    int32 GetShadingQuality() const;

    UFUNCTION(BlueprintPure, Category = "Settings")
    int32 GetVisualEffectsQuality() const;

    UFUNCTION(BlueprintPure, Category = "Settings")
    int32 GetFrameRateLimit() const;
    
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
    bool SavePlayerProgress(int32 SlotIndex, APawn* Pawn);

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