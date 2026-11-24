#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BossEditorLibrary.generated.h"

class UGeometryCollection;
class UStaticMesh;

UCLASS()
class TESISUE_API UBossEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Boss|Editor")
	static UGeometryCollection* FractureMeshToGeometryCollection(UStaticMesh* SourceMesh, const FString& AssetPath, int32 NumPieces = 5,
		int32 RandomSeed = 7);
};
