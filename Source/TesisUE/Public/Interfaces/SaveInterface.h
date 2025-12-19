#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SaveInterface.generated.h"

UINTERFACE()
class USaveInterface : public UInterface
{
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct FEntitySaveData
{
	GENERATED_BODY()

	UPROPERTY()
	FName UniqueSaveID;

	UPROPERTY()
	FTransform Transform;

	UPROPERTY()
	TArray<uint8> ByteData;
};

class TESISUE_API ISaveInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SaveGameData|SaveInterface")
	void OnSaveGame(FEntitySaveData& OutData);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SaveGameData|SaveInterface")
	void OnLoadGame(const FEntitySaveData& InData);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SaveGameData|SaveInterface")
	FName GetUniqueSaveID();
};
