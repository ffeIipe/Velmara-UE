#pragma once

#include "CoreMinimal.h"
#include "SaveTypes.generated.h"

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