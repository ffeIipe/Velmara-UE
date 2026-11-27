#include "Features/BossSystem/Core/BossEditorLibrary.h"

#include "Engine/StaticMesh.h"
#include "GeometryCollection/GeometryCollectionObject.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "ClothingAsset.h"
#include "ClothingAssetFactoryInterface.h"
#include "ClothingSystemEditorInterfaceModule.h"
#include "Engine/SkeletalMesh.h"
#include "Modules/ModuleManager.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PointWeightMap.h"
#include "Rendering/SkeletalMeshModel.h"
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

	TArray<TObjectPtr<UMaterialInterface>> Materials;
	for (const FStaticMaterial& StaticMaterial : SourceMesh->GetStaticMaterials())
	{
		Materials.Add(StaticMaterial.MaterialInterface);
	}
	Collection->GeometrySource.Emplace(FSoftObjectPath(SourceMesh), FTransform::Identity, Materials);
	FGeometryCollectionEngineConversion::AppendStaticMesh(SourceMesh, ObjectPtrDecay(Materials), FTransform::Identity, Collection, false, true);
	Collection->InitializeMaterials(true);

	FGeometryCollection& GeometryCollection = *Collection->GetGeometryCollection();

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

bool UBossEditorLibrary::SetupClothFromSections(USkeletalMesh* Mesh, const FName SimulationSlot, const FName RenderSlot,
	UPhysicsAsset* PhysicsAsset, const float PinnedHeight, const float FreeHeight, const float FreeMaxDistance)
{
#if WITH_EDITOR
	if (!Mesh || !Mesh->GetImportedModel() || !Mesh->GetImportedModel()->LODModels.IsValidIndex(0)) return false;

	FSkeletalMeshLODModel& LodModel = Mesh->GetImportedModel()->LODModels[0];
	const auto FindSection = [Mesh, &LodModel](const FName Slot)
	{
		return LodModel.Sections.IndexOfByPredicate([Mesh, Slot](const FSkelMeshSection& Section)
		{
			return Mesh->GetMaterials().IsValidIndex(Section.MaterialIndex)
				&& Mesh->GetMaterials()[Section.MaterialIndex].MaterialSlotName == Slot;
		});
	};

	const int32 SimulationSection = FindSection(SimulationSlot);
	const int32 RenderSection = FindSection(RenderSlot);
	if (SimulationSection == INDEX_NONE || RenderSection == INDEX_NONE || LodModel.Sections[RenderSection].HasClothingData()) return false;

	Mesh->Modify();
	FScopedSkeletalMeshPostEditChange ScopedPostEditChange(Mesh);

	FSkeletalMeshClothBuildParams Params;
	Params.LodIndex = 0;
	Params.SourceSection = SimulationSection;
	Params.AssetName = RenderSlot.ToString() + TEXT("Clothing");
	Params.PhysicsAsset = PhysicsAsset;
	Params.bRemoveFromMesh = true;

	FClothingSystemEditorInterfaceModule& ClothingModule =
		FModuleManager::LoadModuleChecked<FClothingSystemEditorInterfaceModule>("ClothingSystemEditorInterface");
	UClothingAssetCommon* Asset = Cast<UClothingAssetCommon>(ClothingModule.GetClothingAssetFactory()->CreateFromSkeletalMesh(Mesh, Params));
	if (!Asset || Asset->LodData.IsEmpty()) return false;

	Mesh->AddClothingAsset(Asset);

	FClothLODDataCommon& ClothLod = Asset->LodData[0];
	ClothLod.PointWeightMaps.RemoveAll([](const FPointWeightMap& Map)
	{
		return Map.CurrentTarget == static_cast<uint8>(EWeightMapTargetCommon::MaxDistance);
	});

	FPointWeightMap& MaxDistances = ClothLod.PointWeightMaps.AddDefaulted_GetRef();
	MaxDistances.Initialize(ClothLod.PhysicalMeshData.Vertices.Num());
	MaxDistances.CurrentTarget = static_cast<uint8>(EWeightMapTargetCommon::MaxDistance);
	MaxDistances.bEnabled = true;
	for (int32 Index = 0; Index < ClothLod.PhysicalMeshData.Vertices.Num(); ++Index)
	{
		const float Looseness = FMath::GetRangePct(PinnedHeight, FreeHeight, ClothLod.PhysicalMeshData.Vertices[Index].Z);
		MaxDistances.Values[Index] = FreeMaxDistance * FMath::Clamp(Looseness, 0.f, 1.f);
	}
	Asset->ApplyParameterMasks(true);

	if (!Asset->BindToSkeletalMesh(Mesh, 0, RenderSection, 0)) return false;

	FSkelMeshSourceSectionUserData& UserData =
		LodModel.UserSectionsData.FindOrAdd(LodModel.Sections[RenderSection].OriginalDataSectionIndex);
	UserData.CorrespondClothAssetIndex = static_cast<int16>(Mesh->GetMeshClothingAssets().Find(Asset));
	UserData.ClothingData.AssetGuid = Asset->GetAssetGuid();
	UserData.ClothingData.AssetLodIndex = 0;

	Mesh->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}
