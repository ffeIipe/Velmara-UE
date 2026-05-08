#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Features/SaveSystem/Core/PlayerProgressSaveGame.h"
#include "SaveGameSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameSaved, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameLoaded, bool, bSuccess);

UCLASS()
class TESISUE_API USaveGameSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// --- Public API ---
	UFUNCTION(BlueprintCallable, Category = "Save System")
	void SaveGame(FString SlotName);

	UFUNCTION(BlueprintCallable, Category = "Save System")
	void LoadGame(FString SlotName);

	UFUNCTION(BlueprintCallable, Category = "Save System")
	void RestoreCurrentLevelState() const;

	UPROPERTY(BlueprintAssignable)
	FOnGameSaved OnGameSaved;

	UPROPERTY(BlueprintAssignable)
	FOnGameLoaded OnGameLoaded;

protected:
	UPROPERTY()
	FString CurrentSlotName;

	UPROPERTY()
	UPlayerProgressSaveGame* CurrentSaveGame;

private:
	void SaveLevelActors(UPlayerProgressSaveGame* SaveObject) const;
	void LoadLevelActors(UPlayerProgressSaveGame* SaveObject) const;
};