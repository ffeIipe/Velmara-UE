#include "Features/BossSystem/Core/BossEditorLibrary.h"

#include "Engine/StaticMesh.h"
#include "GeometryCollection/GeometryCollectionObject.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Dataflow/DataflowSelection.h"
#include "FractureEngineFracturing.h"
#include "GeometryCollection/GeometryCollection.h"
#include "GeometryCollection/GeometryCollectionClusteringUtility.h"
#include "GeometryCollection/GeometryCollectionEngineConversion.h"
#include "GeometryCollection/GeometryCollectionUtility.h"
#include "UObject/Package.h"
#endif

UGeometryCollection* UBossEditorLibrary::FractureMeshToGeometryCollection(UStaticMesh* SourceMesh, const FString& AssetPath, const int32 NumPieces,
	const int32 RandomSeed)
{
#if WITH_EDITOR
	if (!SourceMesh || AssetPath.IsEmpty() || NumPieces < 2) return nullptr;

	UPackage* Package = CreatePackage(*AssetPath);
	UGeometryCollection* Collection = NewObject<UGeometryCollection>(Package, FName(*FPackageName::GetLongPackageAssetName(AssetPath)),
		RF_Public | RF_Standalone | RF_Transactional);

	// Igual que el botón "New" de Fracture Mode.
	TArray<TObjectPtr<UMaterialInterface>> Materials;
	for (const FStaticMaterial& StaticMaterial : SourceMesh->GetStaticMaterials())
	{
		Materials.Add(StaticMaterial.MaterialInterface);
	}
	Collection->GeometrySource.Emplace(FSoftObjectPath(SourceMesh), FTransform::Identity, Materials);
	FGeometryCollectionEngineConversion::AppendStaticMesh(SourceMesh, ObjectPtrDecay(Materials), FTransform::Identity, Collection, false, true);
	Collection->InitializeMaterials(true);

	FGeometryCollection& GeometryCollection = *Collection->GetGeometryCollection();

	// Igual que "Uniform Voronoi": un sitio al azar por pieza dentro del bounding box. Sin separar islas, así no salen más de NumPieces.
	const FBox Bounds = SourceMesh->GetBoundingBox();
	FRandomStream Random(RandomSeed);
	TArray<FVector> Sites;
	for (int32 Index = 0; Index < NumPieces; ++Index)
	{
		Sites.Add(Bounds.Min + FVector(Random.FRand(), Random.FRand(), Random.FRand()) * Bounds.GetSize());
	}

	FDataflowTransformSelection Selection;
	Selection.Initialize(GeometryCollection.NumElements(FGeometryCollection::TransformGroup), false);
	Selection.SetSelected(0);

	FFractureEngineFracturing::VoronoiFracture(GeometryCollection, Selection, Sites, FTransform::Identity, RandomSeed,
		1.f, false, 0.f, 0.f, 0.1f, 0.5f, 2.f, 4, 10.f, false, 50.f);

	// Lo que Fracture Mode hace al terminar el corte. Sin convex hulls ni volúmenes, Chaos no tiene nada que simular.
	FGeometryCollectionClusteringUtility::UpdateHierarchyLevelOfChildren(&GeometryCollection, -1);
	::GeometryCollection::GenerateTemporaryGuids(&GeometryCollection, 0, true);
	Collection->UpdateGeometryDependentProperties();
	Collection->InvalidateCollection();
	Collection->RebuildRenderData();
	Collection->CreateSimulationData();

	FAssetRegistryModule::AssetCreated(Collection);
	Collection->MarkPackageDirty();

	return Collection;
#else
	return nullptr;
#endif
}
