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
	// Lo mismo que Fracture Mode "New" + "Uniform Voronoi": crea un Geometry Collection desde el mesh y lo parte en NumPieces.
	// Solo editor (devuelve nullptr en el juego). Guardá el asset después (EditorAssetLibrary.save_asset).
	UFUNCTION(BlueprintCallable, Category = "Boss|Editor")
	static UGeometryCollection* FractureMeshToGeometryCollection(UStaticMesh* SourceMesh, const FString& AssetPath, int32 NumPieces = 5,
		int32 RandomSeed = 7);
};
