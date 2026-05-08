#pragma once

#include "CoreMinimal.h"
#include "SaveTypes.generated.h"

USTRUCT(BlueprintType)
struct FEntitySaveData
{
	GENERATED_BODY()

	UPROPERTY(SaveGame)
	FName UniqueSaveID;

	UPROPERTY(SaveGame)
	FTransform Transform;

	UPROPERTY(SaveGame)
	UClass* ActorClass;
	
	UPROPERTY(SaveGame)
	TArray<uint8> ByteData;
};