// Copyright (c) 2025 71 Studios. All rights reserved.

#include "StageZeroManifestBuilder.h"
#include "StageZeroManifestOverrideSerialization.h"
#include "StageZeroWarmupUtils.h"

#include "StageZeroWarmupManifest.h"
#include "StageZeroAssetRegistryCompat.h"
#include "StageZeroProjectSettings.h"
#include "StageZeroWPAnchor.h"
#include "StageZeroWPSplineWarmup.h"
#include "Algo/AllOf.h"
#include "Algo/AnyOf.h"
#include "Algo/Unique.h"
#include "GameFramework/Actor.h"
#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "Engine/AssetManager.h"
#include "Interfaces/IPluginManager.h"
#include "Settings/ProjectPackagingSettings.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Modules/ModuleManager.h"
#include "UObject/Package.h"
#include "UObject/PackageFileSummary.h"
#include "UObject/SavePackage.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/UObjectIterator.h"
#include "Misc/PackageName.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/ScopeExit.h"
#include "HAL/CriticalSection.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/IConsoleManager.h"
#include "Containers/Queue.h"
#include "Math/Sphere.h"
#include "RHI.h"
#include "EngineUtils.h"
#include "Engine/LevelStreaming.h"
#include "Engine/PostProcessVolume.h"
#include "LevelInstance/LevelInstanceActor.h"
#include "LevelInstance/LevelInstanceSubsystem.h"
#include "Components/PostProcessComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/ActorComponent.h"
#include "Components/SceneCaptureComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SceneCaptureComponentCube.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"
#include "PackedLevelActor/PackedLevelActor.h"
#include "Runtime/Launch/Resources/Version.h"
#include "UObject/UObjectGlobals.h"
#if WITH_EDITOR
#include "WorldPartition/WorldPartition.h"
#include "WorldPartition/WorldPartitionHelpers.h"
#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4)
#include "WorldPartition/WorldPartitionActorDescInstance.h"
#endif
#include "Components/SplineComponent.h"
#include "Containers/ScriptArray.h"
#include "UObject/UnrealType.h"
#endif

namespace
{
static FStageZeroManifestBuildResult GLastManifestBuildResult;
}

#if WITH_EDITOR
namespace
{
static TFunction<void(const FStageZeroManifestProgress&)> GManifestProgressCallback;
static TAutoConsoleVariable<int32> CVarStageZeroManifestAllowBroadDependencyFallback(
    TEXT("StageZero.Manifest.AllowBroadDependencyFallback"),
    0,
    TEXT("Allow broad asset-registry dependency closure fallback when loaded-world manifest collection finds no assets. 0=disabled (default), 1=enabled for compatibility."),
    ECVF_Default);

struct FStageZeroManifestWorldCollectionStats
{
    bool bWorldLoaded = false;
    bool bRecoveryAttempted = false;
    int32 PersistentLevelActorCount = 0;
    int32 StreamingLevelsDiscovered = 0;
    int32 StreamingLevelsLoaded = 0;
    int32 StreamingLevelLoadFailures = 0;
    int32 LevelInstancesTraversed = 0;
    int32 LevelInstanceLoadFailures = 0;
    int32 ComponentsInspected = 0;
};

struct FStageZeroManifestWorldCollectionResult
{
    UWorld* LoadedWorld = nullptr;
    TArray<FSoftObjectPath> Materials;
    TArray<FSoftObjectPath> Textures;
    TArray<FSoftObjectPath> NiagaraSystems;
    TArray<FSoftObjectPath> StaticMeshes;
    TArray<FSoftObjectPath> SkeletalMeshes;
    FStageZeroManifestWorldCollectionStats Stats;

    bool HasAnyAssets() const
    {
        return Materials.Num() > 0
            || Textures.Num() > 0
            || NiagaraSystems.Num() > 0
            || StaticMeshes.Num() > 0
            || SkeletalMeshes.Num() > 0;
    }

    int32 GetAssetCount() const
    {
        return Materials.Num()
            + Textures.Num()
            + NiagaraSystems.Num()
            + StaticMeshes.Num()
            + SkeletalMeshes.Num();
    }

    int32 GetLoadFailureCount() const
    {
        return (Stats.bWorldLoaded ? 0 : 1)
            + Stats.StreamingLevelLoadFailures
            + Stats.LevelInstanceLoadFailures;
    }
};

static TSet<TWeakObjectPtr<UWorld>> GStageZeroManifestScanWorlds;

static bool SZ_ManifestWorldHasEngineContext(const UWorld* World)
{
    if (!World || !GEngine)
    {
        return false;
    }

    for (const FWorldContext& Context : GEngine->GetWorldContexts())
    {
        if (Context.World() == World)
        {
            return true;
        }
    }

    return false;
}

static void SZ_ManifestTrackScanWorld(UWorld* World)
{
    if (!World || SZ_ManifestWorldHasEngineContext(World))
    {
        return;
    }

    // Manifest generation only inspects asset references. These worlds must not
    // look like live editor worlds, or UE 5.6 can fatal during the next map load.
    World->WorldType = EWorldType::Inactive;
    World->ClearFlags(RF_Standalone);
    GStageZeroManifestScanWorlds.Add(World);
}

static void SZ_ManifestCleanupScanWorlds()
{
    int32 PreparedWorldCount = 0;

    for (TSet<TWeakObjectPtr<UWorld>>::TIterator It(GStageZeroManifestScanWorlds); It; ++It)
    {
        UWorld* World = It->Get();
        if (!World || SZ_ManifestWorldHasEngineContext(World))
        {
            continue;
        }

        World->WorldType = EWorldType::Inactive;
        World->ClearFlags(RF_Standalone);
        ++PreparedWorldCount;
    }

    GStageZeroManifestScanWorlds.Reset();

    if (PreparedWorldCount > 0)
    {
        CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
        UE_LOG(LogStageZeroWarmupManifest, Display, TEXT("StageZero: Released %d inactive manifest scan world(s) after manifest build."), PreparedWorldCount);
    }
}

static ULevel* SZ_ManifestGetLevelInstanceLoadedLevel(ALevelInstance* LevelInstance)
{
    return LevelInstance ? LevelInstance->GetLoadedLevel() : nullptr;
}

static UWorld* SZ_ManifestGetLevelInstanceWorld(UWorld* SourceWorld, ALevelInstance* LevelInstance)
{
    if (!LevelInstance)
    {
        return nullptr;
    }

    if (ULevel* LoadedLevel = SZ_ManifestGetLevelInstanceLoadedLevel(LevelInstance))
    {
        return LoadedLevel->GetWorld();
    }

    if (SourceWorld)
    {
#if WITH_EDITOR
        if (ULevelInstanceSubsystem* Subsystem = SourceWorld->GetSubsystem<ULevelInstanceSubsystem>())
        {
            if (ULevel* InstanceLevel = Subsystem->GetLevelInstanceLevel(LevelInstance))
            {
                return InstanceLevel->GetWorld();
            }
        }
#endif
    }

    return nullptr;
}

static FName SZ_ManifestGetStreamingLevelPackageName(const ULevelStreaming* StreamingLevel)
{
    if (!StreamingLevel)
    {
        return NAME_None;
    }

#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
    const FName PackageName = StreamingLevel->GetWorldAssetPackageFName();
    if (PackageName != NAME_None)
    {
        return PackageName;
    }
#endif

    const FString PackageNameString = StreamingLevel->GetWorldAssetPackageName();
    return PackageNameString.IsEmpty() ? NAME_None : FName(*PackageNameString);
}

static UWorld* SZ_ManifestLoadWorldByPackageName(const FName& WorldPackageName)
{
    if (WorldPackageName == NAME_None)
    {
        return nullptr;
    }

    const FString PackageString = WorldPackageName.ToString();
    UPackage* Package = LoadPackage(nullptr, *PackageString, LOAD_None);

    if (Package)
    {
        Package->FullyLoad();
        if (UWorld* World = UWorld::FindWorldInPackage(Package))
        {
            SZ_ManifestTrackScanWorld(World);
            return World;
        }
    }

    const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *PackageString, *FPackageName::GetShortName(PackageString));
    UWorld* World = LoadObject<UWorld>(nullptr, *ObjectPath);
    SZ_ManifestTrackScanWorld(World);
    return World;
}

static void SZ_ManifestPrepareActor(AActor* Actor)
{
    if (!Actor)
    {
        return;
    }

    Actor->ConditionalPostLoad();

    TArray<UActorComponent*> Components;
    Actor->GetComponents(Components);
    for (UActorComponent* Component : Components)
    {
        if (Component)
        {
            Component->ConditionalPostLoad();
        }
    }
}

static void SZ_ManifestPrepareLevel(ULevel* Level)
{
    if (!Level)
    {
        return;
    }

    if (UPackage* LevelPackage = Level->GetOutermost())
    {
        LevelPackage->FullyLoad();
    }

    Level->ConditionalPostLoad();
    for (AActor* Actor : Level->Actors)
    {
        SZ_ManifestPrepareActor(Actor);
    }
}

static void SZ_ManifestPrepareWorld(UWorld* World)
{
    if (!World)
    {
        return;
    }

    if (UPackage* WorldPackage = World->GetOutermost())
    {
        WorldPackage->FullyLoad();
    }

    World->ConditionalPostLoad();
    SZ_ManifestPrepareLevel(World->PersistentLevel);

    for (ULevelStreaming* StreamingLevel : World->GetStreamingLevels())
    {
        if (StreamingLevel)
        {
            StreamingLevel->ConditionalPostLoad();
        }
    }
}

static void SZ_ManifestAppendTexture(
    UTexture* Texture,
    TSet<UTexture*>& SeenTextures,
    TArray<FSoftObjectPath>& OutTextures)
{
    if (!Texture
        || Texture->IsA<UTextureRenderTarget>()
        || SeenTextures.Contains(Texture)
        || !FStageZeroWarmupUtils::IsRuntimeWarmupAssetAllowed(Texture))
    {
        return;
    }

    SeenTextures.Add(Texture);
    const FSoftObjectPath TexturePath(Texture);
    if (FStageZeroWarmupUtils::IsRuntimeWarmupAssetPathAllowed(TexturePath))
    {
        OutTextures.Add(TexturePath);
    }
}

static bool SZ_ManifestIsLoadableAssetPath(const FSoftObjectPath& Path)
{
    return FStageZeroWarmupUtils::IsRuntimeWarmupAssetPathAllowed(Path);
}

static void SZ_ManifestAppendPath(
    const FSoftObjectPath& Path,
    TSet<FSoftObjectPath>& SeenPaths,
    TArray<FSoftObjectPath>& OutPaths)
{
    if (!SZ_ManifestIsLoadableAssetPath(Path) || SeenPaths.Contains(Path))
    {
        return;
    }

    SeenPaths.Add(Path);
    OutPaths.Add(Path);
}

static void SZ_ManifestAppendStaticMesh(
    UStaticMesh* Mesh,
    TSet<FSoftObjectPath>& SeenStaticMeshes,
    TArray<FSoftObjectPath>& OutStaticMeshes)
{
    if (Mesh)
    {
        SZ_ManifestAppendPath(FSoftObjectPath(Mesh), SeenStaticMeshes, OutStaticMeshes);
    }
}

static void SZ_ManifestAppendSkeletalMesh(
    USkeletalMesh* Mesh,
    TSet<FSoftObjectPath>& SeenSkeletalMeshes,
    TArray<FSoftObjectPath>& OutSkeletalMeshes)
{
    if (Mesh)
    {
        SZ_ManifestAppendPath(FSoftObjectPath(Mesh), SeenSkeletalMeshes, OutSkeletalMeshes);
    }
}

static UMaterialInterface* SZ_ManifestResolveStableMaterialAsset(UMaterialInterface* Material)
{
    TSet<UMaterialInterface*> Visited;
    UMaterialInterface* Current = Material;

    while (Current && !Visited.Contains(Current))
    {
        Visited.Add(Current);

        const FSoftObjectPath CurrentPath(Current);
        if (FStageZeroWarmupUtils::IsRuntimeWarmupAssetAllowed(Current)
            && FStageZeroWarmupUtils::IsRuntimeWarmupAssetPathAllowed(CurrentPath))
        {
            return Current;
        }

        if (UMaterialInstance* MaterialInstance = Cast<UMaterialInstance>(Current))
        {
            if (MaterialInstance->Parent && MaterialInstance->Parent != Current)
            {
                Current = MaterialInstance->Parent;
                continue;
            }
        }

        UMaterial* BaseMaterial = Current->GetMaterial();
        if (BaseMaterial && BaseMaterial != Current)
        {
            Current = BaseMaterial;
            continue;
        }

        break;
    }

    return nullptr;
}

static void SZ_ManifestAppendMaterial(
    UMaterialInterface* Material,
    TSet<UMaterialInterface*>& SeenMaterials,
    TSet<UTexture*>& SeenTextures,
    TArray<FSoftObjectPath>& OutMaterials,
    TArray<FSoftObjectPath>& OutTextures,
    bool bIncludeTextures)
{
    if (!Material || SeenMaterials.Contains(Material))
    {
        return;
    }

    UMaterialInterface* StableMaterial = SZ_ManifestResolveStableMaterialAsset(Material);
    if (!StableMaterial || SeenMaterials.Contains(StableMaterial))
    {
        return;
    }

    SeenMaterials.Add(Material);
    SeenMaterials.Add(StableMaterial);
    OutMaterials.Add(FSoftObjectPath(StableMaterial));

    if (!bIncludeTextures)
    {
        return;
    }

    TArray<UTexture*> UsedTextures;
#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7)
    StableMaterial->GetUsedTextures(UsedTextures);
#else
    StableMaterial->GetUsedTextures(UsedTextures, EMaterialQualityLevel::Num, true, GMaxRHIFeatureLevel, true);
#endif

    for (UTexture* Texture : UsedTextures)
    {
        SZ_ManifestAppendTexture(Texture, SeenTextures, OutTextures);
    }
}

static void SZ_ManifestAppendPostProcessSettings(
    const FPostProcessSettings& Settings,
    TSet<UMaterialInterface*>& SeenMaterials,
    TSet<UTexture*>& SeenTextures,
    TArray<FSoftObjectPath>& OutMaterials,
    TArray<FSoftObjectPath>& OutTextures,
    bool bIncludeTextures)
{
    for (const FWeightedBlendable& Blendable : Settings.WeightedBlendables.Array)
    {
        if (UMaterialInterface* Material = Cast<UMaterialInterface>(Blendable.Object))
        {
            SZ_ManifestAppendMaterial(Material, SeenMaterials, SeenTextures, OutMaterials, OutTextures, bIncludeTextures);
            continue;
        }

        if (bIncludeTextures)
        {
            if (UTexture* Texture = Cast<UTexture>(Blendable.Object))
            {
                SZ_ManifestAppendTexture(Texture, SeenTextures, OutTextures);
            }
        }
    }
}

static void SZ_ManifestAppendSceneCaptureAssets(
    USceneCaptureComponent* CaptureComponent,
    TSet<UMaterialInterface*>& SeenMaterials,
    TSet<UTexture*>& SeenTextures,
    TArray<FSoftObjectPath>& OutMaterials,
    TArray<FSoftObjectPath>& OutTextures,
    bool bIncludeTextures)
{
    if (!CaptureComponent)
    {
        return;
    }

    if (USceneCaptureComponent2D* Capture2D = Cast<USceneCaptureComponent2D>(CaptureComponent))
    {
        SZ_ManifestAppendPostProcessSettings(Capture2D->PostProcessSettings, SeenMaterials, SeenTextures, OutMaterials, OutTextures, bIncludeTextures);
        if (bIncludeTextures)
        {
            SZ_ManifestAppendTexture(static_cast<UTexture*>(Capture2D->TextureTarget), SeenTextures, OutTextures);
        }
    }
    else if (USceneCaptureComponentCube* CaptureCube = Cast<USceneCaptureComponentCube>(CaptureComponent))
    {
#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5)
        SZ_ManifestAppendPostProcessSettings(CaptureCube->PostProcessSettings, SeenMaterials, SeenTextures, OutMaterials, OutTextures, bIncludeTextures);
#endif
        if (bIncludeTextures)
        {
            SZ_ManifestAppendTexture(static_cast<UTexture*>(CaptureCube->TextureTarget), SeenTextures, OutTextures);
        }
    }
}

static void SZ_ManifestAppendNiagaraPath(
    const FSoftObjectPath& NiagaraPath,
    TSet<FSoftObjectPath>& SeenNiagaraSystems,
    TArray<FSoftObjectPath>& OutNiagaraSystems)
{
    if (SZ_ManifestIsLoadableAssetPath(NiagaraPath) && !SeenNiagaraSystems.Contains(NiagaraPath))
    {
        SeenNiagaraSystems.Add(NiagaraPath);
        OutNiagaraSystems.Add(NiagaraPath);
    }
}

static void SZ_ManifestAppendObjectReference(
    UObject* ReferencedObject,
    bool bIncludeTextures,
    TSet<UMaterialInterface*>& SeenMaterials,
    TSet<UTexture*>& SeenTextures,
    TSet<FSoftObjectPath>& SeenNiagaraSystems,
    TSet<FSoftObjectPath>& SeenStaticMeshes,
    TSet<FSoftObjectPath>& SeenSkeletalMeshes,
    TArray<FSoftObjectPath>& OutMaterials,
    TArray<FSoftObjectPath>& OutTextures,
    TArray<FSoftObjectPath>& OutNiagaraSystems,
    TArray<FSoftObjectPath>& OutStaticMeshes,
    TArray<FSoftObjectPath>& OutSkeletalMeshes)
{
    if (!ReferencedObject)
    {
        return;
    }

    if (UMaterialInterface* Material = Cast<UMaterialInterface>(ReferencedObject))
    {
        SZ_ManifestAppendMaterial(Material, SeenMaterials, SeenTextures, OutMaterials, OutTextures, bIncludeTextures);
        return;
    }

    if (bIncludeTextures)
    {
        if (UTexture* Texture = Cast<UTexture>(ReferencedObject))
        {
            SZ_ManifestAppendTexture(Texture, SeenTextures, OutTextures);
            return;
        }
    }

    if (UNiagaraSystem* NiagaraSystem = Cast<UNiagaraSystem>(ReferencedObject))
    {
        SZ_ManifestAppendNiagaraPath(FSoftObjectPath(NiagaraSystem), SeenNiagaraSystems, OutNiagaraSystems);
        return;
    }

    if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(ReferencedObject))
    {
        SZ_ManifestAppendStaticMesh(StaticMesh, SeenStaticMeshes, OutStaticMeshes);
        return;
    }

    if (USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(ReferencedObject))
    {
        SZ_ManifestAppendSkeletalMesh(SkeletalMesh, SeenSkeletalMeshes, OutSkeletalMeshes);
    }
}

static void SZ_ManifestAppendSoftReferenceByClass(
    const FSoftObjectPath& Path,
    UClass* PropertyClass,
    bool bIncludeTextures,
    TSet<FSoftObjectPath>& SeenMaterialsByPath,
    TSet<FSoftObjectPath>& SeenTexturesByPath,
    TSet<FSoftObjectPath>& SeenNiagaraSystems,
    TSet<FSoftObjectPath>& SeenStaticMeshes,
    TSet<FSoftObjectPath>& SeenSkeletalMeshes,
    TArray<FSoftObjectPath>& OutMaterials,
    TArray<FSoftObjectPath>& OutTextures,
    TArray<FSoftObjectPath>& OutNiagaraSystems,
    TArray<FSoftObjectPath>& OutStaticMeshes,
    TArray<FSoftObjectPath>& OutSkeletalMeshes)
{
    if (!PropertyClass || !SZ_ManifestIsLoadableAssetPath(Path))
    {
        return;
    }

    if (PropertyClass->IsChildOf(UMaterialInterface::StaticClass()))
    {
        SZ_ManifestAppendPath(Path, SeenMaterialsByPath, OutMaterials);
    }
    else if (bIncludeTextures && PropertyClass->IsChildOf(UTexture::StaticClass()))
    {
        SZ_ManifestAppendPath(Path, SeenTexturesByPath, OutTextures);
    }
    else if (PropertyClass->IsChildOf(UNiagaraSystem::StaticClass()))
    {
        SZ_ManifestAppendNiagaraPath(Path, SeenNiagaraSystems, OutNiagaraSystems);
    }
    else if (PropertyClass->IsChildOf(UStaticMesh::StaticClass()))
    {
        SZ_ManifestAppendPath(Path, SeenStaticMeshes, OutStaticMeshes);
    }
    else if (PropertyClass->IsChildOf(USkeletalMesh::StaticClass()))
    {
        SZ_ManifestAppendPath(Path, SeenSkeletalMeshes, OutSkeletalMeshes);
    }
}

static void SZ_ManifestAppendSoftReferenceByRegistryClass(
    const FSoftObjectPath& Path,
    bool bIncludeTextures,
    TSet<FSoftObjectPath>& SeenMaterialsByPath,
    TSet<FSoftObjectPath>& SeenTexturesByPath,
    TSet<FSoftObjectPath>& SeenNiagaraSystems,
    TSet<FSoftObjectPath>& SeenStaticMeshes,
    TSet<FSoftObjectPath>& SeenSkeletalMeshes,
    TArray<FSoftObjectPath>& OutMaterials,
    TArray<FSoftObjectPath>& OutTextures,
    TArray<FSoftObjectPath>& OutNiagaraSystems,
    TArray<FSoftObjectPath>& OutStaticMeshes,
    TArray<FSoftObjectPath>& OutSkeletalMeshes)
{
    if (!SZ_ManifestIsLoadableAssetPath(Path))
    {
        return;
    }

    FAssetRegistryModule* AssetRegistryModule = FModuleManager::GetModulePtr<FAssetRegistryModule>(TEXT("AssetRegistry"));
    if (!AssetRegistryModule)
    {
        return;
    }

    const FAssetData Asset = StageZeroAssetRegistryCompat::GetAssetByObjectPath(AssetRegistryModule->Get(), Path);
    if (!Asset.IsValid() || (Asset.PackageFlags & PKG_EditorOnly) != 0 || Asset.IsRedirector())
    {
        return;
    }

    if (Asset.IsInstanceOf(UMaterialInterface::StaticClass()))
    {
        SZ_ManifestAppendPath(Path, SeenMaterialsByPath, OutMaterials);
    }
    else if (bIncludeTextures && Asset.IsInstanceOf(UTexture::StaticClass()))
    {
        SZ_ManifestAppendPath(Path, SeenTexturesByPath, OutTextures);
    }
    else if (Asset.IsInstanceOf(UNiagaraSystem::StaticClass()))
    {
        SZ_ManifestAppendNiagaraPath(Path, SeenNiagaraSystems, OutNiagaraSystems);
    }
    else if (Asset.IsInstanceOf(UStaticMesh::StaticClass()))
    {
        SZ_ManifestAppendPath(Path, SeenStaticMeshes, OutStaticMeshes);
    }
    else if (Asset.IsInstanceOf(USkeletalMesh::StaticClass()))
    {
        SZ_ManifestAppendPath(Path, SeenSkeletalMeshes, OutSkeletalMeshes);
    }
}

static void SZ_ManifestAppendReflectedAssetReferences(
    UObject* Source,
    bool bIncludeTextures,
    TSet<UMaterialInterface*>& SeenMaterials,
    TSet<UTexture*>& SeenTextures,
    TSet<FSoftObjectPath>& SeenMaterialsByPath,
    TSet<FSoftObjectPath>& SeenTexturesByPath,
    TSet<FSoftObjectPath>& SeenNiagaraSystems,
    TSet<FSoftObjectPath>& SeenStaticMeshes,
    TSet<FSoftObjectPath>& SeenSkeletalMeshes,
    TArray<FSoftObjectPath>& OutMaterials,
    TArray<FSoftObjectPath>& OutTextures,
    TArray<FSoftObjectPath>& OutNiagaraSystems,
    TArray<FSoftObjectPath>& OutStaticMeshes,
    TArray<FSoftObjectPath>& OutSkeletalMeshes)
{
    if (!Source)
    {
        return;
    }

    constexpr int32 MaxPropertyNesting = 32;
    TSet<const UObject*> VisitedInstancedObjects;
    TFunction<void(FProperty*, void*, int32)> VisitPropertyValue;
    TFunction<void(UObject*, int32)> VisitObject;

    VisitObject = [&](UObject* Object, int32 Depth)
    {
        if (!Object || Depth > MaxPropertyNesting || VisitedInstancedObjects.Contains(Object))
        {
            return;
        }

        VisitedInstancedObjects.Add(Object);
        for (TFieldIterator<FProperty> PropertyIt(Object->GetClass()); PropertyIt; ++PropertyIt)
        {
            FProperty* Property = *PropertyIt;
            if (Property)
            {
                VisitPropertyValue(Property, Property->ContainerPtrToValuePtr<void>(Object), Depth + 1);
            }
        }
    };

    VisitPropertyValue = [&](FProperty* Property, void* ValuePtr, int32 Depth)
    {
        if (!Property || !ValuePtr || Depth > MaxPropertyNesting)
        {
            return;
        }

        // FSoftObjectProperty derives from FObjectPropertyBase. Inspect it first so an
        // unloaded TSoftObjectPtr is classified from its path instead of being discarded
        // when GetObjectPropertyValue() correctly returns nullptr.
        if (FSoftObjectProperty* SoftObjectProperty = CastField<FSoftObjectProperty>(Property))
        {
            const FSoftObjectPtr SoftObjectPtr = SoftObjectProperty->GetPropertyValue(ValuePtr);
            SZ_ManifestAppendSoftReferenceByClass(
                SoftObjectPtr.ToSoftObjectPath(),
                SoftObjectProperty->PropertyClass,
                bIncludeTextures,
                SeenMaterialsByPath,
                SeenTexturesByPath,
                SeenNiagaraSystems,
                SeenStaticMeshes,
                SeenSkeletalMeshes,
                OutMaterials,
                OutTextures,
                OutNiagaraSystems,
                OutStaticMeshes,
                OutSkeletalMeshes);
            return;
        }

        if (FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
        {
            UObject* ReferencedObject = ObjectProperty->GetObjectPropertyValue(ValuePtr);
            SZ_ManifestAppendObjectReference(
                ReferencedObject,
                bIncludeTextures,
                SeenMaterials,
                SeenTextures,
                SeenNiagaraSystems,
                SeenStaticMeshes,
                SeenSkeletalMeshes,
                OutMaterials,
                OutTextures,
                OutNiagaraSystems,
                OutStaticMeshes,
                OutSkeletalMeshes);

            if (ReferencedObject && ObjectProperty->HasAnyPropertyFlags(CPF_InstancedReference))
            {
                VisitObject(ReferencedObject, Depth + 1);
            }
            return;
        }

        if (FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
        {
            FScriptArrayHelper ArrayHelper(ArrayProperty, ValuePtr);
            for (int32 Index = 0; Index < ArrayHelper.Num(); ++Index)
            {
                VisitPropertyValue(ArrayProperty->Inner, ArrayHelper.GetRawPtr(Index), Depth + 1);
            }
            return;
        }

        if (FSetProperty* SetProperty = CastField<FSetProperty>(Property))
        {
            FScriptSetHelper SetHelper(SetProperty, ValuePtr);
            for (int32 Index = 0; Index < SetHelper.GetMaxIndex(); ++Index)
            {
                if (SetHelper.IsValidIndex(Index))
                {
                    VisitPropertyValue(SetProperty->ElementProp, SetHelper.GetElementPtr(Index), Depth + 1);
                }
            }
            return;
        }

        if (FMapProperty* MapProperty = CastField<FMapProperty>(Property))
        {
            FScriptMapHelper MapHelper(MapProperty, ValuePtr);
            for (int32 Index = 0; Index < MapHelper.GetMaxIndex(); ++Index)
            {
                if (MapHelper.IsValidIndex(Index))
                {
                    VisitPropertyValue(MapProperty->KeyProp, MapHelper.GetKeyPtr(Index), Depth + 1);
                    VisitPropertyValue(MapProperty->ValueProp, MapHelper.GetValuePtr(Index), Depth + 1);
                }
            }
            return;
        }

        if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
        {
            if (StructProperty->Struct == TBaseStructure<FSoftObjectPath>::Get())
            {
                const FSoftObjectPath& Path = *static_cast<FSoftObjectPath*>(ValuePtr);
                SZ_ManifestAppendSoftReferenceByRegistryClass(
                    Path,
                    bIncludeTextures,
                    SeenMaterialsByPath,
                    SeenTexturesByPath,
                    SeenNiagaraSystems,
                    SeenStaticMeshes,
                    SeenSkeletalMeshes,
                    OutMaterials,
                    OutTextures,
                    OutNiagaraSystems,
                    OutStaticMeshes,
                    OutSkeletalMeshes);
                return;
            }

            for (TFieldIterator<FProperty> StructPropertyIt(StructProperty->Struct); StructPropertyIt; ++StructPropertyIt)
            {
                FProperty* InnerProperty = *StructPropertyIt;
                if (InnerProperty)
                {
                    VisitPropertyValue(InnerProperty, InnerProperty->ContainerPtrToValuePtr<void>(ValuePtr), Depth + 1);
                }
            }
        }
    };

    VisitObject(Source, 0);
}

static UWorld* SZ_ManifestResolveLevelInstanceWorld(
    UWorld* SourceWorld,
    ALevelInstance* LevelInstance,
    bool bForcePackageLoad)
{
    if (!LevelInstance)
    {
        return nullptr;
    }

    if (UWorld* InstanceWorld = SZ_ManifestGetLevelInstanceWorld(SourceWorld, LevelInstance))
    {
        return InstanceWorld;
    }

    if (bForcePackageLoad)
    {
        const FSoftObjectPath WorldAssetPath = LevelInstance->GetWorldAsset().ToSoftObjectPath();
        const FString LongPackageName = WorldAssetPath.GetLongPackageName();
        if (!LongPackageName.IsEmpty())
        {
            return SZ_ManifestLoadWorldByPackageName(FName(*LongPackageName));
        }
    }

    return nullptr;
}

static void SZ_ManifestCollectActorAssetReferences(
    AActor* Actor,
    bool bIncludeTextures,
    TSet<UMaterialInterface*>& SeenMaterials,
    TSet<UTexture*>& SeenTextures,
    TSet<FSoftObjectPath>& SeenMaterialsByPath,
    TSet<FSoftObjectPath>& SeenTexturesByPath,
    TSet<FSoftObjectPath>& SeenNiagaraSystems,
    TSet<FSoftObjectPath>& SeenStaticMeshes,
    TSet<FSoftObjectPath>& SeenSkeletalMeshes,
    TArray<FSoftObjectPath>& OutMaterials,
    TArray<FSoftObjectPath>& OutTextures,
    TArray<FSoftObjectPath>& OutNiagaraSystems,
    TArray<FSoftObjectPath>& OutStaticMeshes,
    TArray<FSoftObjectPath>& OutSkeletalMeshes,
    TSet<const AActor*>& VisitedActors,
    FStageZeroManifestWorldCollectionStats& InOutStats,
    int32 Depth = 0)
{
    // The visited set is the primary cycle guard; the generous cap only protects against a corrupt pathological graph.
    if (!Actor || VisitedActors.Contains(Actor) || Depth > 64)
    {
        return;
    }

    VisitedActors.Add(Actor);
    Actor->ConditionalPostLoad();
    SZ_ManifestAppendReflectedAssetReferences(
        Actor,
        bIncludeTextures,
        SeenMaterials,
        SeenTextures,
        SeenMaterialsByPath,
        SeenTexturesByPath,
        SeenNiagaraSystems,
        SeenStaticMeshes,
        SeenSkeletalMeshes,
        OutMaterials,
        OutTextures,
        OutNiagaraSystems,
        OutStaticMeshes,
        OutSkeletalMeshes);

    TArray<UActorComponent*> Components;
    Actor->GetComponents(Components);
    for (UActorComponent* Component : Components)
    {
        if (!Component)
        {
            continue;
        }

        Component->ConditionalPostLoad();
        ++InOutStats.ComponentsInspected;
        SZ_ManifestAppendReflectedAssetReferences(
            Component,
            bIncludeTextures,
            SeenMaterials,
            SeenTextures,
            SeenMaterialsByPath,
            SeenTexturesByPath,
            SeenNiagaraSystems,
            SeenStaticMeshes,
            SeenSkeletalMeshes,
            OutMaterials,
            OutTextures,
            OutNiagaraSystems,
            OutStaticMeshes,
            OutSkeletalMeshes);

        if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component))
        {
            const int32 NumMaterials = Primitive->GetNumMaterials();
            for (int32 MaterialIndex = 0; MaterialIndex < NumMaterials; ++MaterialIndex)
            {
                SZ_ManifestAppendMaterial(
                    Primitive->GetMaterial(MaterialIndex),
                    SeenMaterials,
                    SeenTextures,
                    OutMaterials,
                    OutTextures,
                    bIncludeTextures);
            }
        }

        if (UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(Component))
        {
            SZ_ManifestAppendStaticMesh(StaticMeshComponent->GetStaticMesh(), SeenStaticMeshes, OutStaticMeshes);
        }

        if (UPostProcessComponent* PostProcessComponent = Cast<UPostProcessComponent>(Component))
        {
            SZ_ManifestAppendPostProcessSettings(
                PostProcessComponent->Settings,
                SeenMaterials,
                SeenTextures,
                OutMaterials,
                OutTextures,
                bIncludeTextures);
        }

        if (USceneCaptureComponent* CaptureComponent = Cast<USceneCaptureComponent>(Component))
        {
            SZ_ManifestAppendSceneCaptureAssets(
                CaptureComponent,
                SeenMaterials,
                SeenTextures,
                OutMaterials,
                OutTextures,
                bIncludeTextures);
        }

        if (UNiagaraComponent* NiagaraComponent = Cast<UNiagaraComponent>(Component))
        {
            if (UNiagaraSystem* NiagaraSystem = NiagaraComponent->GetAsset())
            {
                SZ_ManifestAppendNiagaraPath(FSoftObjectPath(NiagaraSystem), SeenNiagaraSystems, OutNiagaraSystems);
            }
        }

        if (UChildActorComponent* ChildActorComponent = Cast<UChildActorComponent>(Component))
        {
            if (AActor* ChildActor = ChildActorComponent->GetChildActor())
            {
                SZ_ManifestCollectActorAssetReferences(
                    ChildActor,
                    bIncludeTextures,
                    SeenMaterials,
                    SeenTextures,
                    SeenMaterialsByPath,
                    SeenTexturesByPath,
                    SeenNiagaraSystems,
                    SeenStaticMeshes,
                    SeenSkeletalMeshes,
                    OutMaterials,
                    OutTextures,
                    OutNiagaraSystems,
                    OutStaticMeshes,
                    OutSkeletalMeshes,
                    VisitedActors,
                    InOutStats,
                    Depth + 1);
            }
            else if (UClass* ChildActorClass = ChildActorComponent->GetChildActorClass())
            {
                if (AActor* ChildCDO = Cast<AActor>(ChildActorClass->GetDefaultObject()))
                {
                    SZ_ManifestCollectActorAssetReferences(
                        ChildCDO,
                        bIncludeTextures,
                        SeenMaterials,
                        SeenTextures,
                        SeenMaterialsByPath,
                        SeenTexturesByPath,
                        SeenNiagaraSystems,
                        SeenStaticMeshes,
                        SeenSkeletalMeshes,
                        OutMaterials,
                        OutTextures,
                        OutNiagaraSystems,
                        OutStaticMeshes,
                        OutSkeletalMeshes,
                        VisitedActors,
                        InOutStats,
                        Depth + 1);
                }
            }
        }
    }

    if (APostProcessVolume* PostProcessVolume = Cast<APostProcessVolume>(Actor))
    {
        SZ_ManifestAppendPostProcessSettings(
            PostProcessVolume->Settings,
            SeenMaterials,
            SeenTextures,
            OutMaterials,
            OutTextures,
            bIncludeTextures);
    }
}

static void SZ_ManifestTraverseWorld(
    UWorld* World,
    bool bIncludeLevelInstances,
    bool bIncludePackedLevelActors,
    bool bIncludeTextures,
    bool bForcePackageLoad,
    TSet<UMaterialInterface*>& SeenMaterials,
    TSet<UTexture*>& SeenTextures,
    TSet<FSoftObjectPath>& SeenMaterialsByPath,
    TSet<FSoftObjectPath>& SeenTexturesByPath,
    TSet<FSoftObjectPath>& SeenNiagaraSystems,
    TSet<FSoftObjectPath>& SeenStaticMeshes,
    TSet<FSoftObjectPath>& SeenSkeletalMeshes,
    TArray<FSoftObjectPath>& OutMaterials,
    TArray<FSoftObjectPath>& OutTextures,
    TArray<FSoftObjectPath>& OutNiagaraSystems,
    TArray<FSoftObjectPath>& OutStaticMeshes,
    TArray<FSoftObjectPath>& OutSkeletalMeshes,
    TSet<TWeakObjectPtr<UWorld>>& VisitedWorlds,
    TSet<FName>& VisitedStreamingPackages,
    FStageZeroManifestWorldCollectionStats& InOutStats)
{
    if (!World || VisitedWorlds.Contains(World))
    {
        return;
    }

    SZ_ManifestPrepareWorld(World);
    VisitedWorlds.Add(World);
    InOutStats.bWorldLoaded = true;

    if (VisitedWorlds.Num() == 1 && World->PersistentLevel)
    {
        InOutStats.PersistentLevelActorCount = World->PersistentLevel->Actors.Num();
    }

    ULevel* PersistentLevel = World->PersistentLevel;
    if (!PersistentLevel)
    {
        return;
    }

    TSet<const AActor*> VisitedActors;
    for (AActor* Actor : PersistentLevel->Actors)
    {
        if (!Actor)
        {
            continue;
        }

        if (!bIncludePackedLevelActors && Actor->IsA<APackedLevelActor>())
        {
            continue;
        }

        SZ_ManifestCollectActorAssetReferences(
            Actor,
            bIncludeTextures,
            SeenMaterials,
            SeenTextures,
            SeenMaterialsByPath,
            SeenTexturesByPath,
            SeenNiagaraSystems,
            SeenStaticMeshes,
            SeenSkeletalMeshes,
            OutMaterials,
            OutTextures,
            OutNiagaraSystems,
            OutStaticMeshes,
            OutSkeletalMeshes,
            VisitedActors,
            InOutStats);

        if (bIncludeLevelInstances)
        {
            if (ALevelInstance* LevelInstance = Cast<ALevelInstance>(Actor))
            {
                if (UWorld* InstanceWorld = SZ_ManifestResolveLevelInstanceWorld(World, LevelInstance, bForcePackageLoad))
                {
                    ++InOutStats.LevelInstancesTraversed;
                    SZ_ManifestTraverseWorld(
                        InstanceWorld,
                        bIncludeLevelInstances,
                        bIncludePackedLevelActors,
                        bIncludeTextures,
                        bForcePackageLoad,
                        SeenMaterials,
                        SeenTextures,
                        SeenMaterialsByPath,
                        SeenTexturesByPath,
                        SeenNiagaraSystems,
                        SeenStaticMeshes,
                        SeenSkeletalMeshes,
                        OutMaterials,
                        OutTextures,
                        OutNiagaraSystems,
                        OutStaticMeshes,
                        OutSkeletalMeshes,
                        VisitedWorlds,
                        VisitedStreamingPackages,
                        InOutStats);
                }
                else
                {
                    ++InOutStats.LevelInstanceLoadFailures;
                }
            }
        }
    }

    for (ULevelStreaming* StreamingLevel : World->GetStreamingLevels())
    {
        if (!StreamingLevel)
        {
            continue;
        }

        ++InOutStats.StreamingLevelsDiscovered;
        const FName StreamingPackage = SZ_ManifestGetStreamingLevelPackageName(StreamingLevel);
        if (StreamingPackage == NAME_None || VisitedStreamingPackages.Contains(StreamingPackage))
        {
            continue;
        }

        VisitedStreamingPackages.Add(StreamingPackage);
        UWorld* StreamingWorld = nullptr;
        if (ULevel* LoadedStreamingLevel = StreamingLevel->GetLoadedLevel())
        {
            StreamingWorld = LoadedStreamingLevel->GetWorld();
        }
        if (!StreamingWorld && bForcePackageLoad)
        {
            StreamingWorld = SZ_ManifestLoadWorldByPackageName(StreamingPackage);
        }
        if (!StreamingWorld)
        {
            const FString PackageString = StreamingPackage.ToString();
            const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *PackageString, *FPackageName::GetShortName(PackageString));
            StreamingWorld = LoadObject<UWorld>(nullptr, *ObjectPath);
        }

        if (StreamingWorld)
        {
            ++InOutStats.StreamingLevelsLoaded;
            SZ_ManifestTraverseWorld(
                StreamingWorld,
                bIncludeLevelInstances,
                bIncludePackedLevelActors,
                bIncludeTextures,
                bForcePackageLoad,
                SeenMaterials,
                SeenTextures,
                SeenMaterialsByPath,
                SeenTexturesByPath,
                SeenNiagaraSystems,
                SeenStaticMeshes,
                SeenSkeletalMeshes,
                OutMaterials,
                OutTextures,
                OutNiagaraSystems,
                OutStaticMeshes,
                OutSkeletalMeshes,
                VisitedWorlds,
                VisitedStreamingPackages,
                InOutStats);
        }
        else
        {
            ++InOutStats.StreamingLevelLoadFailures;
        }
    }
}

static FStageZeroManifestWorldCollectionResult SZ_ManifestCollectAssetsFromWorld(
    const FName& WorldPackageName,
    UWorld* InitialWorld,
    bool bIncludeTextures,
    bool bIncludeLevelInstances,
    bool bIncludePackedLevelActors,
    bool bForcePackageLoad,
    bool bMarkRecoveryAttempt)
{
    FStageZeroManifestWorldCollectionResult Result;
    Result.LoadedWorld = InitialWorld;
    Result.Stats.bRecoveryAttempted = bMarkRecoveryAttempt;

    if (!Result.LoadedWorld)
    {
        Result.LoadedWorld = SZ_ManifestLoadWorldByPackageName(WorldPackageName);
    }

    if (!Result.LoadedWorld)
    {
        return Result;
    }

    TSet<UMaterialInterface*> SeenMaterials;
    TSet<UTexture*> SeenTextures;
    TSet<FSoftObjectPath> SeenMaterialsByPath;
    TSet<FSoftObjectPath> SeenTexturesByPath;
    TSet<FSoftObjectPath> SeenNiagaraSystems;
    TSet<FSoftObjectPath> SeenStaticMeshes;
    TSet<FSoftObjectPath> SeenSkeletalMeshes;
    TSet<TWeakObjectPtr<UWorld>> VisitedWorlds;
    TSet<FName> VisitedStreamingPackages;

    SZ_ManifestTraverseWorld(
        Result.LoadedWorld,
        bIncludeLevelInstances,
        bIncludePackedLevelActors,
        bIncludeTextures,
        bForcePackageLoad,
        SeenMaterials,
        SeenTextures,
        SeenMaterialsByPath,
        SeenTexturesByPath,
        SeenNiagaraSystems,
        SeenStaticMeshes,
        SeenSkeletalMeshes,
        Result.Materials,
        Result.Textures,
        Result.NiagaraSystems,
        Result.StaticMeshes,
        Result.SkeletalMeshes,
        VisitedWorlds,
        VisitedStreamingPackages,
        Result.Stats);

    return Result;
}
}
#endif

namespace
{
static const TCHAR* StageZeroManifestAssetName = TEXT("StageZeroWarmupManifest");
}

const FStageZeroManifestBuildResult& FStageZeroManifestBuilder::GetLastBuildResult()
{
    return GLastManifestBuildResult;
}

#if WITH_EDITOR
void FStageZeroManifestBuilder::SetProgressCallback(TFunction<void(const FStageZeroManifestProgress&)> InCallback)
{
    GManifestProgressCallback = MoveTemp(InCallback);
}

void FStageZeroManifestBuilder::ClearProgressCallback()
{
    GManifestProgressCallback = nullptr;
}
#endif

#if WITH_DEV_AUTOMATION_TESTS
FStageZeroManifestReflectedAssetReferences FStageZeroManifestBuilder::CollectReflectedAssetReferencesForTesting(
    UObject* Source,
    bool bIncludeTextures)
{
    TSet<UMaterialInterface*> SeenMaterials;
    TSet<UTexture*> SeenTextures;
    TSet<FSoftObjectPath> SeenMaterialsByPath;
    TSet<FSoftObjectPath> SeenTexturesByPath;
    TSet<FSoftObjectPath> SeenNiagaraSystems;
    TSet<FSoftObjectPath> SeenStaticMeshes;
    TSet<FSoftObjectPath> SeenSkeletalMeshes;
    FStageZeroManifestReflectedAssetReferences References;

    SZ_ManifestAppendReflectedAssetReferences(
        Source,
        bIncludeTextures,
        SeenMaterials,
        SeenTextures,
        SeenMaterialsByPath,
        SeenTexturesByPath,
        SeenNiagaraSystems,
        SeenStaticMeshes,
        SeenSkeletalMeshes,
        References.Materials,
        References.Textures,
        References.NiagaraSystems,
        References.StaticMeshes,
        References.SkeletalMeshes);

    return References;
}
#endif

#if WITH_EDITOR
namespace StageZeroManifestConfig
{
static const TCHAR* Section = TEXT("StageZero.ManifestBuildOptions");
static const TCHAR* KeyUseAllMaps = TEXT("UseAllMaps");
static const TCHAR* KeySelectedMaps = TEXT("SelectedMaps");
static const TCHAR* KeyIncludeWorldPartition = TEXT("IncludeWorldPartition");
static const TCHAR* KeyIncludeAnchorAndSplineRoutes = TEXT("IncludeAnchorAndSplineRoutes");
static const TCHAR* KeyIncludeTextures = TEXT("IncludeTextures");
static const TCHAR* KeyIncludeAllProjectMaterials = TEXT("IncludeAllProjectMaterials");
static const TCHAR* KeyIncludeAllProjectTextures = TEXT("IncludeAllProjectTextures");
static const TCHAR* KeyIncludeEngineContent = TEXT("IncludeEngineContent");
static const TCHAR* KeyGlobalAdditionalAssets = TEXT("GlobalAdditionalAssets");
static const TCHAR* KeyGlobalExcludedAssets = TEXT("GlobalExcludedAssets");
static const TCHAR* KeyPerMapAssetOverrides = TEXT("PerMapAssetOverrides");
static const TCHAR* KeyAllowShortNameMapFallback = TEXT("AllowShortNameMapFallback");

static bool LoadFromConfigFile(FStageZeroManifestBuildOptions& Options, const FString& ConfigFile)
{
    if (!GConfig || ConfigFile.IsEmpty())
    {
        return false;
    }

    bool bLoadedAny = false;
    bLoadedAny |= GConfig->GetBool(Section, KeyUseAllMaps, Options.bUseAllMaps, ConfigFile);
    bLoadedAny |= GConfig->GetBool(Section, KeyIncludeWorldPartition, Options.bIncludeWorldPartition, ConfigFile);
    bLoadedAny |= GConfig->GetBool(Section, KeyIncludeAnchorAndSplineRoutes, Options.bIncludeAnchorAndSplineRoutes, ConfigFile);
    bLoadedAny |= GConfig->GetBool(Section, KeyIncludeTextures, Options.bIncludeTextures, ConfigFile);
    bLoadedAny |= GConfig->GetBool(Section, KeyIncludeAllProjectMaterials, Options.bIncludeAllProjectMaterials, ConfigFile);
    bLoadedAny |= GConfig->GetBool(Section, KeyIncludeAllProjectTextures, Options.bIncludeAllProjectTextures, ConfigFile);
    bLoadedAny |= GConfig->GetBool(Section, KeyIncludeEngineContent, Options.bIncludeEngineContent, ConfigFile);

    TArray<FString> SelectedMaps;
    const int32 NumSelectedMaps = GConfig->GetArray(Section, KeySelectedMaps, SelectedMaps, ConfigFile);
    if (bLoadedAny || NumSelectedMaps > 0)
    {
        Options.SelectedMaps.Reset();
        for (const FString& PathString : SelectedMaps)
        {
            const FString Trimmed = PathString.TrimStartAndEnd();
            if (!Trimmed.IsEmpty())
            {
                Options.SelectedMaps.Add(FSoftObjectPath(Trimmed));
            }
        }
    }

    TArray<FString> GlobalAdditionalAssets;
    const int32 NumGlobalAdditionalAssets = GConfig->GetArray(
        Section, KeyGlobalAdditionalAssets, GlobalAdditionalAssets, ConfigFile);
    if (bLoadedAny || NumGlobalAdditionalAssets > 0)
    {
        Options.GlobalAdditionalAssets.Reset();
        for (const FString& PathString : GlobalAdditionalAssets)
        {
            const FString Trimmed = PathString.TrimStartAndEnd();
            if (!Trimmed.IsEmpty())
            {
                Options.GlobalAdditionalAssets.Add(FSoftObjectPath(Trimmed));
            }
        }
    }

    TArray<FString> GlobalExcludedAssets;
    const int32 NumGlobalExcludedAssets = GConfig->GetArray(
        Section, KeyGlobalExcludedAssets, GlobalExcludedAssets, ConfigFile);
    if (bLoadedAny || NumGlobalExcludedAssets > 0)
    {
        Options.GlobalExcludedAssets.Reset();
        for (const FString& PathString : GlobalExcludedAssets)
        {
            const FString Trimmed = PathString.TrimStartAndEnd();
            if (!Trimmed.IsEmpty())
            {
                Options.GlobalExcludedAssets.Add(FSoftObjectPath(Trimmed));
            }
        }
    }

    TArray<FString> PerMapOverrides;
    const int32 NumPerMapOverrides = GConfig->GetArray(
        Section, KeyPerMapAssetOverrides, PerMapOverrides, ConfigFile);
    if (bLoadedAny || NumPerMapOverrides > 0)
    {
        Options.PerMapAssetOverrides.Reset();
        for (const FString& SerializedOverride : PerMapOverrides)
        {
            FStageZeroManifestMapAssetOverrides ParsedOverride;
            FString ParseError;
            if (StageZeroManifestOverrideSerialization::DecodeMapOverride(
                SerializedOverride, ParsedOverride, ParseError))
            {
                Options.PerMapAssetOverrides.Add(MoveTemp(ParsedOverride));
            }
            else
            {
                UE_LOG(LogStageZeroWarmupManifest, Warning,
                    TEXT("StageZero: Ignoring malformed persisted per-map override '%s': %s"),
                    *SerializedOverride,
                    *ParseError);
            }
        }
    }

    Options.bAllowShortNameMapFallback = false;
    StageZeroManifestOverrideSerialization::NormalizeOptions(Options);
    return bLoadedAny
        || NumSelectedMaps > 0
        || NumGlobalAdditionalAssets > 0
        || NumGlobalExcludedAssets > 0
        || NumPerMapOverrides > 0;
}

static void SaveToConfigFile(const FStageZeroManifestBuildOptions& Options, const FString& ConfigFile)
{
    if (!GConfig || ConfigFile.IsEmpty())
    {
        return;
    }

    GConfig->SetBool(Section, KeyUseAllMaps, Options.bUseAllMaps, ConfigFile);
    GConfig->SetBool(Section, KeyIncludeWorldPartition, Options.bIncludeWorldPartition, ConfigFile);
    GConfig->SetBool(Section, KeyIncludeAnchorAndSplineRoutes, Options.bIncludeAnchorAndSplineRoutes, ConfigFile);
    GConfig->SetBool(Section, KeyIncludeTextures, Options.bIncludeTextures, ConfigFile);
    GConfig->SetBool(Section, KeyIncludeAllProjectMaterials, Options.bIncludeAllProjectMaterials, ConfigFile);
    GConfig->SetBool(Section, KeyIncludeAllProjectTextures, Options.bIncludeAllProjectTextures, ConfigFile);
    GConfig->SetBool(Section, KeyIncludeEngineContent, Options.bIncludeEngineContent, ConfigFile);
    GConfig->RemoveKey(Section, KeyAllowShortNameMapFallback, ConfigFile);

    TArray<FString> SelectedMaps;
    SelectedMaps.Reserve(Options.SelectedMaps.Num());
    for (const FSoftObjectPath& Path : Options.SelectedMaps)
    {
        if (!Path.IsNull())
        {
            SelectedMaps.Add(Path.ToString());
        }
    }
    GConfig->SetArray(Section, KeySelectedMaps, SelectedMaps, ConfigFile);

    TArray<FString> GlobalAdditionalAssets;
    GlobalAdditionalAssets.Reserve(Options.GlobalAdditionalAssets.Num());
    for (const FSoftObjectPath& Path : Options.GlobalAdditionalAssets)
    {
        if (!Path.IsNull())
        {
            GlobalAdditionalAssets.Add(Path.ToString());
        }
    }
    GlobalAdditionalAssets.Sort();
    GlobalAdditionalAssets.SetNum(Algo::Unique(GlobalAdditionalAssets));
    GConfig->SetArray(Section, KeyGlobalAdditionalAssets, GlobalAdditionalAssets, ConfigFile);

    TArray<FString> GlobalExcludedAssets;
    GlobalExcludedAssets.Reserve(Options.GlobalExcludedAssets.Num());
    for (const FSoftObjectPath& Path : Options.GlobalExcludedAssets)
    {
        if (!Path.IsNull())
        {
            GlobalExcludedAssets.Add(Path.ToString());
        }
    }
    GlobalExcludedAssets.Sort();
    GlobalExcludedAssets.SetNum(Algo::Unique(GlobalExcludedAssets));
    GConfig->SetArray(Section, KeyGlobalExcludedAssets, GlobalExcludedAssets, ConfigFile);

    TArray<FString> PerMapOverrides;
    PerMapOverrides.Reserve(Options.PerMapAssetOverrides.Num());
    for (const FStageZeroManifestMapAssetOverrides& Override : Options.PerMapAssetOverrides)
    {
        FString SerializedOverride;
        FString SerializationError;
        if (StageZeroManifestOverrideSerialization::EncodeMapOverride(
            Override, SerializedOverride, SerializationError))
        {
            PerMapOverrides.Add(MoveTemp(SerializedOverride));
        }
        else
        {
            UE_LOG(LogStageZeroWarmupManifest, Warning,
                TEXT("StageZero: Per-map override was not persisted because it is incomplete or malformed: %s"),
                *SerializationError);
        }
    }
    PerMapOverrides.Sort();
    PerMapOverrides.SetNum(Algo::Unique(PerMapOverrides));
    GConfig->SetArray(Section, KeyPerMapAssetOverrides, PerMapOverrides, ConfigFile);
    GConfig->Flush(false, ConfigFile);
}
}

FStageZeroManifestBuildOptions FStageZeroManifestBuilder::LoadManifestBuildOptions()
{
    FStageZeroManifestBuildOptions Options;

    if (!GConfig)
    {
        return Options;
    }

    StageZeroManifestConfig::LoadFromConfigFile(Options, GGameIni);
    if (!GEditorPerProjectIni.IsEmpty() && GEditorPerProjectIni != GGameIni)
    {
        StageZeroManifestConfig::LoadFromConfigFile(Options, GEditorPerProjectIni);
    }

    return Options;
}

void FStageZeroManifestBuilder::SaveManifestBuildOptions(const FStageZeroManifestBuildOptions& Options)
{
    if (!GConfig)
    {
        return;
    }

    StageZeroManifestConfig::SaveToConfigFile(Options, GGameIni);
    if (!GEditorPerProjectIni.IsEmpty() && GEditorPerProjectIni != GGameIni)
    {
        StageZeroManifestConfig::SaveToConfigFile(Options, GEditorPerProjectIni);
    }
}

namespace
{
static void SZ_ResetManifestData(UStageZeroWarmupManifest* Manifest)
{
    if (!Manifest)
    {
        return;
    }

    Manifest->AllMaterials.Reset();
    Manifest->AllTextures.Reset();
    Manifest->AllNiagaraSystems.Reset();
    Manifest->AllStaticMeshes.Reset();
    Manifest->AllSkeletalMeshes.Reset();
    Manifest->MapEntries.Reset();
    Manifest->WPCells.Reset();
    Manifest->WPAnchorGroups.Reset();
    Manifest->WPSplineGroups.Reset();
}

static void SZ_CopyManifestData(UStageZeroWarmupManifest* Destination, const UStageZeroWarmupManifest* Source)
{
    if (!Destination || !Source)
    {
        return;
    }

    Destination->AllMaterials = Source->AllMaterials;
    Destination->AllTextures = Source->AllTextures;
    Destination->AllNiagaraSystems = Source->AllNiagaraSystems;
    Destination->AllStaticMeshes = Source->AllStaticMeshes;
    Destination->AllSkeletalMeshes = Source->AllSkeletalMeshes;
    Destination->MapEntries = Source->MapEntries;
    Destination->WPCells = Source->WPCells;
    Destination->WPAnchorGroups = Source->WPAnchorGroups;
    Destination->WPSplineGroups = Source->WPSplineGroups;
}

static FString SZ_SanitizeSummaryValue(const FString& Value)
{
    FString Sanitized = Value;
    Sanitized.ReplaceInline(TEXT("\""), TEXT(""));
    Sanitized.ReplaceInline(TEXT("\r"), TEXT("_"));
    Sanitized.ReplaceInline(TEXT("\n"), TEXT("_"));
    Sanitized.ReplaceInline(TEXT("\t"), TEXT("_"));
    Sanitized.ReplaceInline(TEXT(" "), TEXT("_"));
    return Sanitized.IsEmpty() ? TEXT("None") : Sanitized;
}

static void SZ_LogManifestBuildSummary(const FStageZeroManifestBuildResult& Result)
{
    UE_LOG(
        LogStageZeroWarmupManifest,
        Display,
        TEXT("StageZeroBuildManifestSummary: Schema=2 Requested=%d Resolved=%d Missing=%d Processed=%d MapsWithAssets=%d ZeroAssetMaps=%d WorldLoadFailures=%d Saved=%d Preserved=%d ForcedAllZeroSave=%d Succeeded=%d MissingMaps=%s MissingReasons=%s Reason=%s Complete=1"),
        Result.NumRequestedMaps,
        Result.NumResolvedMaps,
        Result.NumMissingRequestedMaps,
        Result.NumProcessedMaps,
        Result.NumMapsWithAssets,
        Result.NumZeroAssetMaps,
        Result.NumWorldLoadFailures,
        Result.bManifestSaved ? 1 : 0,
        Result.bExistingManifestPreserved ? 1 : 0,
        Result.bForcedAllZeroSave ? 1 : 0,
        Result.bSucceeded ? 1 : 0,
        *SZ_SanitizeSummaryValue(FString::Join(Result.MissingRequestedMaps, TEXT(";"))),
        *SZ_SanitizeSummaryValue(FString::Join(Result.MissingRequestedMapReasons, TEXT(";"))),
        *SZ_SanitizeSummaryValue(Result.FailureReason));
}

static FString SZ_MakeUniqueManifestFilePath(const FString& Directory, const TCHAR* Purpose, bool bTemporary = false)
{
    const FString FileName = FString::Printf(
        TEXT("%s_%s_%s_P%u_%s%s"),
        StageZeroManifestAssetName,
        Purpose,
        *FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S_%s")),
        FPlatformProcess::GetCurrentProcessId(),
        *FGuid::NewGuid().ToString(EGuidFormats::Digits),
        bTemporary ? TEXT(".uasset.tmp") : TEXT(".uasset"));
    return Directory / FileName;
}

static bool SZ_ValidateStagedManifestFile(
    const FString& StagedFilePath,
    FString& OutFailureReason,
    int64 ExpectedFileSize = -1)
{
    const int64 StagedSize = IFileManager::Get().FileSize(*StagedFilePath);
    if (StagedSize <= 0)
    {
        OutFailureReason = FString::Printf(TEXT("Staged manifest was missing or empty: %s"), *StagedFilePath);
        return false;
    }

    if (ExpectedFileSize >= 0 && StagedSize != ExpectedFileSize)
    {
        OutFailureReason = FString::Printf(
            TEXT("Manifest file size changed during the transaction (Expected=%lld Actual=%lld): %s"),
            static_cast<long long>(ExpectedFileSize),
            static_cast<long long>(StagedSize),
            *StagedFilePath);
        return false;
    }

    TUniquePtr<FArchive> Reader(IFileManager::Get().CreateFileReader(*StagedFilePath));
    if (!Reader)
    {
        OutFailureReason = FString::Printf(TEXT("Could not reopen staged manifest for validation: %s"), *StagedFilePath);
        return false;
    }

    FPackageFileSummary Summary;
    *Reader << Summary;
    if (Reader->IsError() || Summary.Tag != PACKAGE_FILE_TAG)
    {
        OutFailureReason = FString::Printf(TEXT("Staged manifest did not contain a valid Unreal package header: %s"), *StagedFilePath);
        return false;
    }


    const int64 SummaryEnd = Reader->Tell();
    const auto IsOffsetInsideFile = [StagedSize](int64 Offset)
    {
        return Offset > 0 && Offset < StagedSize;
    };
    const auto IsTableInsideHeader = [StagedSize, SummaryEnd, &Summary](int32 Count, int64 Offset)
    {
        return Count >= 0
            && (Count == 0 || (Offset >= SummaryEnd && Offset < Summary.TotalHeaderSize && Offset < StagedSize));
    };

    const bool bSummaryLayoutValid = SummaryEnd > 0
        && Summary.TotalHeaderSize >= SummaryEnd
        && Summary.TotalHeaderSize <= StagedSize
        && Summary.NameCount > 0
        && IsTableInsideHeader(Summary.NameCount, Summary.NameOffset)
        && Summary.ExportCount > 0
        && IsTableInsideHeader(Summary.ExportCount, Summary.ExportOffset)
        && IsTableInsideHeader(Summary.ImportCount, Summary.ImportOffset)
        && Summary.NameOffset >= SummaryEnd
        && Summary.NameOffset <= Summary.ImportOffset
        && Summary.ImportOffset <= Summary.ExportOffset
        && Summary.ExportOffset <= Summary.DependsOffset
        && Summary.DependsOffset <= Summary.AssetRegistryDataOffset
        && Summary.DependsOffset >= SummaryEnd
        && Summary.AssetRegistryDataOffset >= SummaryEnd
        && Summary.AssetRegistryDataOffset < Summary.TotalHeaderSize
        && IsOffsetInsideFile(Summary.AssetRegistryDataOffset)
        && Summary.BulkDataStartOffset >= Summary.TotalHeaderSize
        && Summary.BulkDataStartOffset <= StagedSize;
    if (!bSummaryLayoutValid)
    {
        OutFailureReason = FString::Printf(
            TEXT("Manifest package tables or offsets were structurally invalid (Size=%lld SummaryEnd=%lld Header=%d Names=%d@%d Imports=%d@%d Exports=%d@%d Depends=%d AssetRegistry=%d Bulk=%lld): %s"),
            static_cast<long long>(StagedSize),
            static_cast<long long>(SummaryEnd),
            Summary.TotalHeaderSize,
            Summary.NameCount,
            Summary.NameOffset,
            Summary.ImportCount,
            Summary.ImportOffset,
            Summary.ExportCount,
            Summary.ExportOffset,
            Summary.DependsOffset,
            Summary.AssetRegistryDataOffset,
            static_cast<long long>(Summary.BulkDataStartOffset),
            *StagedFilePath);
        return false;
    }
    return true;
}

static bool SZ_ValidateManifestContents(
    const UStageZeroWarmupManifest* Manifest,
    int32 ExpectedMapCount,
    FString& OutFailureReason)
{
    if (!Manifest)
    {
        OutFailureReason = TEXT("Manifest candidate was null");
        return false;
    }

    if (ExpectedMapCount <= 0 || Manifest->MapEntries.Num() != ExpectedMapCount)
    {
        OutFailureReason = FString::Printf(
            TEXT("Manifest candidate map count was inconsistent (Expected=%d Actual=%d)"),
            ExpectedMapCount,
            Manifest->MapEntries.Num());
        return false;
    }

    TSet<FName> SeenMaps;
    TSet<FSoftObjectPath> AllMaterials;
    TSet<FSoftObjectPath> AllTextures;
    TSet<FSoftObjectPath> AllNiagara;
    TSet<FSoftObjectPath> AllStaticMeshes;
    TSet<FSoftObjectPath> AllSkeletalMeshes;
    AllMaterials.Append(Manifest->AllMaterials);
    AllTextures.Append(Manifest->AllTextures);
    AllNiagara.Append(Manifest->AllNiagaraSystems);
    AllStaticMeshes.Append(Manifest->AllStaticMeshes);
    AllSkeletalMeshes.Append(Manifest->AllSkeletalMeshes);

    const auto IsValidUniquePathArray = [](const TArray<FSoftObjectPath>& Paths, const TSet<FSoftObjectPath>& UniquePaths)
    {
        return UniquePaths.Num() == Paths.Num() && Algo::AllOf(Paths, [](const FSoftObjectPath& Path)
        {
            return Path.IsValid();
        });
    };
    if (!IsValidUniquePathArray(Manifest->AllMaterials, AllMaterials)
        || !IsValidUniquePathArray(Manifest->AllTextures, AllTextures)
        || !IsValidUniquePathArray(Manifest->AllNiagaraSystems, AllNiagara)
        || !IsValidUniquePathArray(Manifest->AllStaticMeshes, AllStaticMeshes)
        || !IsValidUniquePathArray(Manifest->AllSkeletalMeshes, AllSkeletalMeshes))
    {
        OutFailureReason = TEXT("Manifest candidate global asset arrays contained an invalid or duplicate path");
        return false;
    }

    const auto IsSubset = [](const TArray<FSoftObjectPath>& Paths, const TSet<FSoftObjectPath>& Union)
    {
        return Algo::AllOf(Paths, [&Union](const FSoftObjectPath& Path)
        {
            return Path.IsValid() && Union.Contains(Path);
        });
    };
    const auto HasValidTypedAssets = [&](const TArray<FSoftObjectPath>& Materials,
        const TArray<FSoftObjectPath>& Textures,
        const TArray<FSoftObjectPath>& Niagara,
        const TArray<FSoftObjectPath>& StaticMeshes,
        const TArray<FSoftObjectPath>& SkeletalMeshes)
    {
        return IsValidUniquePathArray(Materials, TSet<FSoftObjectPath>(Materials))
            && IsValidUniquePathArray(Textures, TSet<FSoftObjectPath>(Textures))
            && IsValidUniquePathArray(Niagara, TSet<FSoftObjectPath>(Niagara))
            && IsValidUniquePathArray(StaticMeshes, TSet<FSoftObjectPath>(StaticMeshes))
            && IsValidUniquePathArray(SkeletalMeshes, TSet<FSoftObjectPath>(SkeletalMeshes))
            && IsSubset(Materials, AllMaterials)
            && IsSubset(Textures, AllTextures)
            && IsSubset(Niagara, AllNiagara)
            && IsSubset(StaticMeshes, AllStaticMeshes)
            && IsSubset(SkeletalMeshes, AllSkeletalMeshes);
    };
    const auto HasAnyTypedAssets = [](const TArray<FSoftObjectPath>& Materials,
        const TArray<FSoftObjectPath>& Textures,
        const TArray<FSoftObjectPath>& Niagara,
        const TArray<FSoftObjectPath>& StaticMeshes,
        const TArray<FSoftObjectPath>& SkeletalMeshes)
    {
        return Materials.Num() > 0
            || Textures.Num() > 0
            || Niagara.Num() > 0
            || StaticMeshes.Num() > 0
            || SkeletalMeshes.Num() > 0;
    };

    for (const FStageZeroMapMatTexEntry& Entry : Manifest->MapEntries)
    {
        if (Entry.MapPackageName.IsNone() || SeenMaps.Contains(Entry.MapPackageName))
        {
            OutFailureReason = FString::Printf(
                TEXT("Manifest candidate contained an empty or duplicate map entry: %s"),
                *Entry.MapPackageName.ToString());
            return false;
        }
        SeenMaps.Add(Entry.MapPackageName);

        if (!HasValidTypedAssets(
            Entry.Materials,
            Entry.Textures,
            Entry.NiagaraSystems,
            Entry.StaticMeshes,
            Entry.SkeletalMeshes))
        {
            OutFailureReason = FString::Printf(
                TEXT("Manifest candidate contained invalid, duplicate, or globally uncovered assets for map entry %s"),
                *Entry.MapPackageName.ToString());
            return false;
        }
    }

    TSet<FString> SeenCellIds;
    for (const FStageZeroWPCellEntry& Cell : Manifest->WPCells)
    {
        const FString CellKey = Cell.MapPackageName.ToString() + TEXT("|") + Cell.CellId;
        if (!SeenMaps.Contains(Cell.MapPackageName)
            || Cell.CellId.IsEmpty()
            || !Cell.Bounds.IsValid
            || Cell.Bounds.Min.ContainsNaN()
            || Cell.Bounds.Max.ContainsNaN()
            || SeenCellIds.Contains(CellKey)
            || !HasAnyTypedAssets(Cell.Materials, Cell.Textures, Cell.NiagaraSystems, Cell.StaticMeshes, Cell.SkeletalMeshes)
            || !HasValidTypedAssets(Cell.Materials, Cell.Textures, Cell.NiagaraSystems, Cell.StaticMeshes, Cell.SkeletalMeshes))
        {
            OutFailureReason = FString::Printf(TEXT("Manifest candidate contained an invalid or duplicate World Partition cell: %s"), *CellKey);
            return false;
        }
        SeenCellIds.Add(CellKey);
    }

    TSet<FString> SeenAnchorIds;
    for (const FStageZeroWPAnchorGroup& Anchor : Manifest->WPAnchorGroups)
    {
        const FString AnchorKey = Anchor.MapPackageName.ToString() + TEXT("|") + Anchor.AnchorId;
        if (!SeenMaps.Contains(Anchor.MapPackageName)
            || Anchor.AnchorId.IsEmpty()
            || Anchor.Location.ContainsNaN()
            || !FMath::IsFinite(Anchor.Radius)
            || Anchor.Radius < 0.0f
            || SeenAnchorIds.Contains(AnchorKey)
            || !HasAnyTypedAssets(Anchor.Materials, Anchor.Textures, Anchor.NiagaraSystems, Anchor.StaticMeshes, Anchor.SkeletalMeshes)
            || !HasValidTypedAssets(Anchor.Materials, Anchor.Textures, Anchor.NiagaraSystems, Anchor.StaticMeshes, Anchor.SkeletalMeshes))
        {
            OutFailureReason = FString::Printf(TEXT("Manifest candidate contained an invalid or duplicate World Partition anchor group: %s"), *AnchorKey);
            return false;
        }
        SeenAnchorIds.Add(AnchorKey);
    }

    TSet<FString> SeenRouteIds;
    for (const FStageZeroWPSplineGroup& Route : Manifest->WPSplineGroups)
    {
        const FString RouteKey = Route.MapPackageName.ToString() + TEXT("|") + Route.RouteId;
        if (!SeenMaps.Contains(Route.MapPackageName)
            || Route.RouteId.IsEmpty()
            || !FMath::IsFinite(Route.SampleRadius)
            || Route.SampleRadius < 1.0f
            || SeenRouteIds.Contains(RouteKey)
            || !HasAnyTypedAssets(Route.Materials, Route.Textures, Route.NiagaraSystems, Route.StaticMeshes, Route.SkeletalMeshes)
            || !HasValidTypedAssets(Route.Materials, Route.Textures, Route.NiagaraSystems, Route.StaticMeshes, Route.SkeletalMeshes))
        {
            OutFailureReason = FString::Printf(TEXT("Manifest candidate contained an invalid or duplicate World Partition spline group: %s"), *RouteKey);
            return false;
        }
        SeenRouteIds.Add(RouteKey);
    }

    if (Manifest->AllMaterials.Num() == 0
        && Manifest->AllTextures.Num() == 0
        && Manifest->AllNiagaraSystems.Num() == 0
        && Manifest->AllStaticMeshes.Num() == 0
        && Manifest->AllSkeletalMeshes.Num() == 0)
    {
        OutFailureReason = TEXT("Manifest candidate contained no assets");
        return false;
    }

    return true;
}

static bool SZ_RestoreValidatedManifestFile(
    const FString& SourceFilePath,
    const FString& DestinationFilePath,
    FString& OutFailureReason)
{
    FString SourceValidationFailure;
    if (!SZ_ValidateStagedManifestFile(SourceFilePath, SourceValidationFailure))
    {
        OutFailureReason = FString::Printf(TEXT("Restore source was invalid: %s"), *SourceValidationFailure);
        return false;
    }

    IFileManager& FileManager = IFileManager::Get();
    const int64 SourceFileSize = FileManager.FileSize(*SourceFilePath);
    const FString DestinationDirectory = FPaths::GetPath(DestinationFilePath);
    const FString RestoreStagedPath = SZ_MakeUniqueManifestFilePath(DestinationDirectory, TEXT("RestoreStaged"), true);
    ON_SCOPE_EXIT
    {
        FileManager.Delete(*RestoreStagedPath, false, true, true);
    };

    if (FileManager.Copy(*RestoreStagedPath, *SourceFilePath, true, false) != COPY_OK)
    {
        OutFailureReason = FString::Printf(TEXT("Could not stage restore source %s"), *SourceFilePath);
        return false;
    }

    FString StagedValidationFailure;
    if (!SZ_ValidateStagedManifestFile(RestoreStagedPath, StagedValidationFailure, SourceFileSize))
    {
        OutFailureReason = FString::Printf(TEXT("Staged restore copy failed verification: %s"), *StagedValidationFailure);
        return false;
    }

    if (!FileManager.Move(*DestinationFilePath, *RestoreStagedPath, true, false, true, false))
    {
        OutFailureReason = FString::Printf(
            TEXT("Could not transactionally restore %s from %s"),
            *DestinationFilePath,
            *SourceFilePath);
        return false;
    }

    FString DestinationValidationFailure;
    if (!SZ_ValidateStagedManifestFile(DestinationFilePath, DestinationValidationFailure, SourceFileSize))
    {
        OutFailureReason = FString::Printf(TEXT("Restored destination failed verification: %s"), *DestinationValidationFailure);
        return false;
    }

    return true;
}

static bool SZ_RecoverLatestLastGoodManifestIfNeeded(const FString& PackageFilePath, FString& OutFailureReason)
{
    IFileManager& FileManager = IFileManager::Get();
    const bool bDestinationExists = FPaths::FileExists(PackageFilePath);
    FString DestinationValidationFailure;
    if (bDestinationExists && SZ_ValidateStagedManifestFile(PackageFilePath, DestinationValidationFailure))
    {
        return true;
    }

    const FString BackupDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("StageZeroManifestBackups"));
    TArray<FString> BackupFiles;
    FileManager.FindFiles(BackupFiles, *(BackupDir / TEXT("StageZeroWarmupManifest_LastGood_*.uasset")), true, false);
    BackupFiles.Sort([](const FString& Left, const FString& Right)
    {
        return Left > Right;
    });

    FString RecoverySource;
    for (const FString& BackupFile : BackupFiles)
    {
        const FString CandidatePath = BackupDir / BackupFile;
        FString BackupValidationFailure;
        if (SZ_ValidateStagedManifestFile(CandidatePath, BackupValidationFailure))
        {
            RecoverySource = CandidatePath;
            break;
        }
        UE_LOG(LogStageZeroWarmupManifest, Warning, TEXT("StageZero: Ignoring invalid last-good manifest backup: %s"), *BackupValidationFailure);
    }

    if (RecoverySource.IsEmpty())
    {
        if (!bDestinationExists)
        {
            // A first-time build has nothing to recover. Invalid historical backups are
            // retained for diagnosis but do not prevent authoring a fresh manifest.
            return true;
        }

        if (!FileManager.MakeDirectory(*BackupDir, true))
        {
            OutFailureReason = FString::Printf(TEXT("Failed to create manifest recovery directory %s"), *BackupDir);
            return false;
        }
        const FString CorruptCopyPath = SZ_MakeUniqueManifestFilePath(BackupDir, TEXT("Corrupt"));
        if (FileManager.Copy(*CorruptCopyPath, *PackageFilePath, true, true) != COPY_OK)
        {
            OutFailureReason = FString::Printf(TEXT("Invalid manifest could not be preserved for diagnosis at %s"), *CorruptCopyPath);
            return false;
        }
        if (!FileManager.Delete(*PackageFilePath, false, false, true))
        {
            OutFailureReason = FString::Printf(TEXT("Invalid manifest was preserved at %s but could not be removed before replacement"), *CorruptCopyPath);
            return false;
        }
        UE_LOG(LogStageZeroWarmupManifest, Warning,
            TEXT("StageZero: Preserved invalid manifest at %s and cleared the destination for a fresh validated replacement. Prior validation: %s"),
            *CorruptCopyPath,
            DestinationValidationFailure.IsEmpty() ? TEXT("package validation failed") : *DestinationValidationFailure);
        return true;
    }

    if (bDestinationExists)
    {
        if (!FileManager.MakeDirectory(*BackupDir, true))
        {
            OutFailureReason = FString::Printf(TEXT("Failed to create manifest recovery directory %s"), *BackupDir);
            return false;
        }

        const FString CorruptCopyPath = SZ_MakeUniqueManifestFilePath(BackupDir, TEXT("Corrupt"));
        if (FileManager.Copy(*CorruptCopyPath, *PackageFilePath, true, true) != COPY_OK)
        {
            OutFailureReason = FString::Printf(TEXT("Existing manifest is invalid and could not be preserved for diagnosis at %s"), *CorruptCopyPath);
            return false;
        }
    }

    const FString PackageDirectory = FPaths::GetPath(PackageFilePath);
    FileManager.MakeDirectory(*PackageDirectory, true);
    const FString RecoveryStagedPath = SZ_MakeUniqueManifestFilePath(PackageDirectory, TEXT("RecoveryStaged"), true);
    ON_SCOPE_EXIT
    {
        FileManager.Delete(*RecoveryStagedPath, false, true, true);
    };

    if (FileManager.Copy(*RecoveryStagedPath, *RecoverySource, true, true) != COPY_OK)
    {
        OutFailureReason = FString::Printf(TEXT("Failed to stage manifest recovery from %s"), *RecoverySource);
        return false;
    }

    FString RecoveryValidationFailure;
    if (!SZ_ValidateStagedManifestFile(RecoveryStagedPath, RecoveryValidationFailure))
    {
        OutFailureReason = FString::Printf(TEXT("Staged manifest recovery failed validation: %s"), *RecoveryValidationFailure);
        return false;
    }

    const int64 RecoveryStagedSize = FileManager.FileSize(*RecoveryStagedPath);

    if (!FileManager.Move(*PackageFilePath, *RecoveryStagedPath, true, false, true, false))
    {
        OutFailureReason = FString::Printf(TEXT("Failed to commit manifest recovery from %s to %s"), *RecoverySource, *PackageFilePath);
        return false;
    }


    FString CommittedRecoveryValidationFailure;
    if (!SZ_ValidateStagedManifestFile(PackageFilePath, CommittedRecoveryValidationFailure, RecoveryStagedSize))
    {
        FString DirectRestoreValidationFailure;
        if (!SZ_RestoreValidatedManifestFile(RecoverySource, PackageFilePath, DirectRestoreValidationFailure))
        {
            OutFailureReason = FString::Printf(
                TEXT("Committed manifest recovery failed verification and direct restoration failed (Commit=%s Restore=%s)"),
                *CommittedRecoveryValidationFailure,
                DirectRestoreValidationFailure.IsEmpty() ? TEXT("transactional restore failed") : *DirectRestoreValidationFailure);
            return false;
        }
    }

    UE_LOG(LogStageZeroWarmupManifest, Warning, TEXT("StageZero: Recovered %s manifest destination from last-good backup %s"),
        bDestinationExists ? TEXT("invalid") : TEXT("missing"),
        *RecoverySource);
    return true;
}

static bool SZ_CommitStagedManifestFile(
    const FString& StagedFilePath,
    const FString& PackageFilePath,
    bool bHadExistingManifest,
    FString& OutBackupPath,
    bool& bOutExistingManifestPreservedOnFailure,
    FString& OutFailureReason)
{
    IFileManager& FileManager = IFileManager::Get();
    OutBackupPath.Reset();
    bOutExistingManifestPreservedOnFailure = false;
    const int64 StagedFileSize = FileManager.FileSize(*StagedFilePath);
    if (StagedFileSize <= 0)
    {
        OutFailureReason = FString::Printf(TEXT("Staged manifest was unavailable before commit: %s"), *StagedFilePath);
        return false;
    }

    if (bHadExistingManifest)
    {
        FString ExistingValidationFailure;
        if (!SZ_ValidateStagedManifestFile(PackageFilePath, ExistingValidationFailure))
        {
            OutFailureReason = FString::Printf(
                TEXT("Existing manifest failed validation immediately before backup: %s"),
                *ExistingValidationFailure);
            return false;
        }
        bOutExistingManifestPreservedOnFailure = true;

        const int64 ExistingFileSize = FileManager.FileSize(*PackageFilePath);
        const FString BackupDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("StageZeroManifestBackups"));
        if (!FileManager.MakeDirectory(*BackupDir, true))
        {
            OutFailureReason = FString::Printf(TEXT("Failed to create manifest backup directory %s"), *BackupDir);
            return false;
        }

        OutBackupPath = SZ_MakeUniqueManifestFilePath(BackupDir, TEXT("LastGood"));
        if (FileManager.Copy(*OutBackupPath, *PackageFilePath, true, true) != COPY_OK)
        {
            OutFailureReason = FString::Printf(TEXT("Failed to preserve the existing manifest at %s"), *OutBackupPath);
            OutBackupPath.Reset();
            return false;
        }

        FString BackupValidationFailure;
        if (!SZ_ValidateStagedManifestFile(OutBackupPath, BackupValidationFailure, ExistingFileSize))
        {
            OutFailureReason = FString::Printf(
                TEXT("Last-good manifest backup failed verification: %s"),
                *BackupValidationFailure);
            OutBackupPath.Reset();
            return false;
        }
    }

    if (!FileManager.Move(*PackageFilePath, *StagedFilePath, true, false, true, false))
    {
        if (bHadExistingManifest)
        {
            FString ExistingAfterMoveFailureValidation;
            bOutExistingManifestPreservedOnFailure = SZ_ValidateStagedManifestFile(
                PackageFilePath,
                ExistingAfterMoveFailureValidation);
            if (!bOutExistingManifestPreservedOnFailure && !OutBackupPath.IsEmpty())
            {
                FString RestoreFailure;
                bOutExistingManifestPreservedOnFailure = SZ_RestoreValidatedManifestFile(
                    OutBackupPath,
                    PackageFilePath,
                    RestoreFailure);
                if (!bOutExistingManifestPreservedOnFailure && !RestoreFailure.IsEmpty())
                {
                    OutFailureReason = FString::Printf(
                        TEXT("Failed to replace manifest and failed to restore last-good state: %s"),
                        *RestoreFailure);
                    return false;
                }
            }
        }
        OutFailureReason = FString::Printf(TEXT("Failed to replace manifest %s with staged file %s; last-good backup remains at %s"),
            *PackageFilePath,
            *StagedFilePath,
            OutBackupPath.IsEmpty() ? TEXT("none") : *OutBackupPath);
        return false;
    }

    bOutExistingManifestPreservedOnFailure = false;


    FString CommittedValidationFailure;
    if (!SZ_ValidateStagedManifestFile(PackageFilePath, CommittedValidationFailure, StagedFileSize))
    {
        bool bRestoredLastGood = false;
        if (bHadExistingManifest && !OutBackupPath.IsEmpty())
        {
            FString RestoredValidationFailure;
            bRestoredLastGood = SZ_RestoreValidatedManifestFile(
                OutBackupPath,
                PackageFilePath,
                RestoredValidationFailure);
            if (!bRestoredLastGood)
            {
                CommittedValidationFailure += FString::Printf(
                    TEXT("; transactional last-good restore failed: %s"),
                    *RestoredValidationFailure);
            }
        }
        else
        {
            FileManager.Delete(*PackageFilePath, false, false, true);
        }
        bOutExistingManifestPreservedOnFailure = bRestoredLastGood;

        OutFailureReason = FString::Printf(
            TEXT("Committed manifest failed post-replacement verification and %s: %s"),
            bRestoredLastGood ? TEXT("the last-good manifest was restored") : TEXT("automatic restoration was unavailable or failed"),
            *CommittedValidationFailure);
        return false;
    }

    return true;
}
}

#if WITH_DEV_AUTOMATION_TESTS
bool FStageZeroManifestBuilder::ValidateManifestContentsForTesting(
    const UStageZeroWarmupManifest* Manifest,
    int32 ExpectedMapCount,
    FString& OutFailureReason)
{
    return SZ_ValidateManifestContents(Manifest, ExpectedMapCount, OutFailureReason);
}
#endif

UStageZeroWarmupManifest* FStageZeroManifestBuilder::BuildManifestWithOptions(const FStageZeroManifestBuildOptions& Options, FStageZeroManifestBuildResult* OutResult)
{
    FStageZeroManifestBuildOptions EffectiveOptions = Options;
    EffectiveOptions.bAllowShortNameMapFallback = false;
    StageZeroManifestOverrideSerialization::NormalizeOptions(EffectiveOptions);
    UE_LOG(LogStageZeroWarmupManifest, Display, TEXT("StageZero: Building warmup manifest (UseAllMaps=%s, SelectedMaps=%d, WP=%s, Anchors/Splines=%s, Textures=%s, AllMaterials=%s, AllTextures=%s, IncludeEngine=%s, GlobalIncludes=%d, GlobalExcludes=%d, PerMapOverrides=%d)"),
        EffectiveOptions.bUseAllMaps ? TEXT("true") : TEXT("false"),
        EffectiveOptions.SelectedMaps.Num(),
        EffectiveOptions.bIncludeWorldPartition ? TEXT("true") : TEXT("false"),
        EffectiveOptions.bIncludeAnchorAndSplineRoutes ? TEXT("true") : TEXT("false"),
        EffectiveOptions.bIncludeTextures ? TEXT("true") : TEXT("false"),
        EffectiveOptions.bIncludeAllProjectMaterials ? TEXT("true") : TEXT("false"),
        EffectiveOptions.bIncludeAllProjectTextures ? TEXT("true") : TEXT("false"),
        EffectiveOptions.bIncludeEngineContent ? TEXT("true") : TEXT("false"),
        EffectiveOptions.GlobalAdditionalAssets.Num(),
        EffectiveOptions.GlobalExcludedAssets.Num(),
        EffectiveOptions.PerMapAssetOverrides.Num());

    FStageZeroManifestBuildResult BuildResult;
    const FString ProjectIdentity = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()).ToLower();
    const FString BuildLockName = FString::Printf(TEXT("StageZeroManifestBuild_%08x"), GetTypeHash(ProjectIdentity));
    FSystemWideCriticalSection ManifestBuildLock(BuildLockName, FTimespan::FromSeconds(1.0));
    if (!ManifestBuildLock.IsValid())
    {
        BuildResult.FailureReason = TEXT("Another Stage Zero manifest build already owns the project build lock");
        GLastManifestBuildResult = BuildResult;
        SZ_LogManifestBuildSummary(BuildResult);
        if (OutResult)
        {
            *OutResult = BuildResult;
        }
        return nullptr;
    }
    ON_SCOPE_EXIT
    {
        SZ_ManifestCleanupScanWorlds();
    };

    auto PublishFailure = [&BuildResult, OutResult](const FString& Reason, bool bExistingManifestPreserved) -> UStageZeroWarmupManifest*
    {
        BuildResult.bSucceeded = false;
        BuildResult.bManifestSaved = false;
        BuildResult.bExistingManifestPreserved = bExistingManifestPreserved;
        if (!Reason.IsEmpty())
        {
            BuildResult.FailureReason = Reason;
        }
        GLastManifestBuildResult = BuildResult;
        SZ_LogManifestBuildSummary(BuildResult);
        if (!BuildResult.FailureReason.IsEmpty())
        {
            UE_LOG(LogStageZeroWarmupManifest, Warning, TEXT("StageZero: Manifest build failed before save. %s"), *BuildResult.FailureReason);
        }
        if (OutResult)
        {
            *OutResult = BuildResult;
        }
        return nullptr;
    };

    const FString PackageFilePath = FPackageName::LongPackageNameToFilename(
        FStageZeroWarmupUtils::GetManifestPackagePath(),
        FPackageName::GetAssetPackageExtension());
    const bool bDestinationExistedAtStart = FPaths::FileExists(PackageFilePath);

    if (UPackage* LoadedManifestPackage = FindPackage(nullptr, FStageZeroWarmupUtils::GetManifestPackagePath()))
    {
        if (LoadedManifestPackage->IsDirty())
        {
            return PublishFailure(
                TEXT("The loaded Stage Zero manifest package has unsaved changes. Save or discard them explicitly before rebuilding."),
                bDestinationExistedAtStart);
        }
    }

    if (GIsEditor && !IsRunningCommandlet())
    {
        TArray<FString> DirtySourcePackages;
        for (TObjectIterator<UPackage> It; It; ++It)
        {
            UPackage* DirtyPackage = *It;
            if (!DirtyPackage
                || DirtyPackage == GetTransientPackage()
                || !DirtyPackage->IsDirty()
                || DirtyPackage->HasAnyFlags(RF_Transient)
                || DirtyPackage->HasAnyPackageFlags(PKG_CompiledIn | PKG_PlayInEditor))
            {
                continue;
            }

            const FString PackageName = DirtyPackage->GetName();
            if (PackageName == FStageZeroWarmupUtils::GetManifestPackagePath()
                || PackageName.StartsWith(TEXT("/Engine/"))
                || PackageName.StartsWith(TEXT("/Script/"))
                || FPackageName::IsMemoryPackage(PackageName)
                || FPackageName::IsTempPackage(PackageName)
                || !FPackageName::IsValidLongPackageName(PackageName))
            {
                continue;
            }

            DirtySourcePackages.Add(PackageName);
        }

        if (DirtySourcePackages.Num() > 0)
        {
            DirtySourcePackages.Sort();
            const int32 DisplayCount = FMath::Min(DirtySourcePackages.Num(), 8);
            TArray<FString> DisplayPackages;
            DisplayPackages.Append(DirtySourcePackages.GetData(), DisplayCount);
            const FString MoreSuffix = DirtySourcePackages.Num() > DisplayCount
                ? FString::Printf(TEXT("; ... and %d more"), DirtySourcePackages.Num() - DisplayCount)
                : FString();
            return PublishFailure(
                FString::Printf(
                    TEXT("Manifest generation requires saved source state. Save or discard these dirty project/plugin packages first: %s%s"),
                    *FString::Join(DisplayPackages, TEXT("; ")),
                    *MoreSuffix),
                bDestinationExistedAtStart);
        }
    }

    if (bDestinationExistedAtStart && IFileManager::Get().IsReadOnly(*PackageFilePath))
    {
        return PublishFailure(
            FString::Printf(TEXT("Existing manifest is read-only; check it out before building: %s"), *PackageFilePath),
            true);
    }

    // A force-terminated commandlet cannot execute scope-exit cleanup. Temporary
    // files use a non-asset extension and are removed under the project build lock.
    {
        TArray<FString> StaleTemporaryFiles;
        const FString PackageDirectory = FPaths::GetPath(PackageFilePath);
        IFileManager::Get().FindFiles(StaleTemporaryFiles, *(PackageDirectory / TEXT("StageZeroWarmupManifest_*.uasset.tmp")), true, false);
        for (const FString& StaleFile : StaleTemporaryFiles)
        {
            IFileManager::Get().Delete(*(PackageDirectory / StaleFile), false, false, true);
        }
    }

    UStageZeroWarmupManifest* CandidateManifest = NewObject<UStageZeroWarmupManifest>(GetTransientPackage(), NAME_None, RF_Transient);
    BuildResult = BuildManifestContents(CandidateManifest, EffectiveOptions);
    if (!BuildResult.bSucceeded)
    {
        return PublishFailure(BuildResult.FailureReason, bDestinationExistedAtStart);
    }

    FString CandidateValidationFailure;
    if (!SZ_ValidateManifestContents(CandidateManifest, BuildResult.NumProcessedMaps, CandidateValidationFailure))
    {
        return PublishFailure(CandidateValidationFailure, bDestinationExistedAtStart);
    }

    FString RecoveryFailureReason;
    if (!SZ_RecoverLatestLastGoodManifestIfNeeded(PackageFilePath, RecoveryFailureReason))
    {
        return PublishFailure(RecoveryFailureReason, FPaths::FileExists(PackageFilePath));
    }
    const bool bHadExistingManifest = FPaths::FileExists(PackageFilePath);

    UPackage* Package = CreatePackage(*FString(FStageZeroWarmupUtils::GetManifestPackagePath()));
    if (!Package)
    {
        BuildResult.bSucceeded = false;
        BuildResult.bExistingManifestPreserved = bHadExistingManifest;
        BuildResult.FailureReason = TEXT("Failed to create manifest package");
        GLastManifestBuildResult = BuildResult;
        SZ_LogManifestBuildSummary(BuildResult);
        if (OutResult)
        {
            *OutResult = BuildResult;
        }
        return nullptr;
    }

    Package->FullyLoad();

    UStageZeroWarmupManifest* Manifest = FindObject<UStageZeroWarmupManifest>(Package, StageZeroManifestAssetName);
    const bool bCreatedNewAsset = (Manifest == nullptr);
    if (!Manifest)
    {
        Manifest = NewObject<UStageZeroWarmupManifest>(
            Package,
            UStageZeroWarmupManifest::StaticClass(),
            FName(StageZeroManifestAssetName),
            RF_Public | RF_Standalone);
    }

    if (!Manifest)
    {
        BuildResult.bSucceeded = false;
        BuildResult.bExistingManifestPreserved = bHadExistingManifest;
        BuildResult.FailureReason = TEXT("Failed to create manifest object");
        GLastManifestBuildResult = BuildResult;
        SZ_LogManifestBuildSummary(BuildResult);
        if (OutResult)
        {
            *OutResult = BuildResult;
        }
        return nullptr;
    }

    UStageZeroWarmupManifest* PreviousManifestData = nullptr;
    if (!bCreatedNewAsset)
    {
        PreviousManifestData = NewObject<UStageZeroWarmupManifest>(GetTransientPackage(), NAME_None, RF_Transient);
        SZ_CopyManifestData(PreviousManifestData, Manifest);
    }

    SZ_ResetManifestData(Manifest);
    SZ_CopyManifestData(Manifest, CandidateManifest);
    Manifest->MarkPackageDirty();

    const FString PackageDirectory = FPaths::GetPath(PackageFilePath);
    IFileManager::Get().MakeDirectory(*PackageDirectory, true);
    const FString StagedFilePath = SZ_MakeUniqueManifestFilePath(PackageDirectory, TEXT("Staged"), true);
    ON_SCOPE_EXIT
    {
        IFileManager::Get().Delete(*StagedFilePath, false, true, true);
    };

    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;
    FString SaveFailureReason;
    FString ManifestBackupPath;
    bool bExistingManifestPreservedOnFailure = bHadExistingManifest;
    const FSavePackageResultStruct SaveResult = UPackage::Save(Package, Manifest, *StagedFilePath, SaveArgs);
    const bool bSavedExpectedAsset = Algo::AnyOf(SaveResult.SavedAssets, [](const FAssetData& SavedAsset)
    {
        return SavedAsset.AssetName == FName(StageZeroManifestAssetName)
            && SavedAsset.AssetClassPath == UStageZeroWarmupManifest::StaticClass()->GetClassPathName();
    });
    const bool bSavedStagedFile = SaveResult.Result == ESavePackageResult::Success
        && SaveResult.TotalFileSize > 0
        && bSavedExpectedAsset;
    if (!bSavedStagedFile)
    {
        SaveFailureReason = FString::Printf(
            TEXT("Staged manifest save was not a complete success (Result=%d TotalFileSize=%lld SavedExpectedAsset=%s)"),
            static_cast<int32>(SaveResult.Result),
            static_cast<long long>(SaveResult.TotalFileSize),
            bSavedExpectedAsset ? TEXT("true") : TEXT("false"));
    }
    const bool bValidatedStagedFile = bSavedStagedFile && SZ_ValidateStagedManifestFile(StagedFilePath, SaveFailureReason);
    const bool bCommittedStagedFile = bValidatedStagedFile && SZ_CommitStagedManifestFile(
        StagedFilePath,
        PackageFilePath,
        bHadExistingManifest,
        ManifestBackupPath,
        bExistingManifestPreservedOnFailure,
        SaveFailureReason);

    if (!bCommittedStagedFile)
    {
        if (!bSavedStagedFile && SaveFailureReason.IsEmpty())
        {
            SaveFailureReason = FString::Printf(TEXT("Failed to save staged warmup manifest to %s"), *StagedFilePath);
        }
        if (PreviousManifestData)
        {
            SZ_ResetManifestData(Manifest);
            SZ_CopyManifestData(Manifest, PreviousManifestData);
            Package->SetDirtyFlag(false);
        }
        BuildResult.bSucceeded = false;
        BuildResult.bExistingManifestPreserved = bHadExistingManifest && bExistingManifestPreservedOnFailure;
        BuildResult.FailureReason = SaveFailureReason;
        UE_LOG(LogStageZeroWarmupManifest, Warning, TEXT("StageZero: %s"), *BuildResult.FailureReason);
    }
    else
    {
        BuildResult.bManifestSaved = true;
        BuildResult.bExistingManifestPreserved = bHadExistingManifest && !ManifestBackupPath.IsEmpty();
        Package->SetDirtyFlag(false);
        if (bCreatedNewAsset)
        {
            FAssetRegistryModule::AssetCreated(Manifest);
        }
        UE_LOG(LogStageZeroWarmupManifest, Display, TEXT("StageZero: Transactionally committed warmup manifest to %s (last-good backup: %s)"),
            *PackageFilePath,
            ManifestBackupPath.IsEmpty() ? TEXT("none") : *ManifestBackupPath);
    }

    UE_LOG(LogStageZeroWarmupManifest, Display, TEXT("StageZero: Manifest summary - Maps=%d Materials=%d Textures=%d Niagara=%d StaticMeshes=%d SkeletalMeshes=%d WPCells=%d WPAnchors=%d WPSplines=%d"),
        Manifest->MapEntries.Num(),
        Manifest->AllMaterials.Num(),
        Manifest->AllTextures.Num(),
        Manifest->AllNiagaraSystems.Num(),
        Manifest->AllStaticMeshes.Num(),
        Manifest->AllSkeletalMeshes.Num(),
        Manifest->WPCells.Num(),
        Manifest->WPAnchorGroups.Num(),
        Manifest->WPSplineGroups.Num());

    GLastManifestBuildResult = BuildResult;
    SZ_LogManifestBuildSummary(BuildResult);
    if (OutResult)
    {
        *OutResult = BuildResult;
    }

    if (!BuildResult.bSucceeded)
    {
        if (bCreatedNewAsset)
        {
            // A failed first save must not leave an unsaved public asset in the
            // editor package that Save All could persist outside this transaction.
            Manifest->ClearFlags(RF_Public | RF_Standalone);
            Manifest->MarkAsGarbage();
            Package->SetDirtyFlag(false);
        }
        return nullptr;
    }

    return Manifest;
}
#endif

FStageZeroManifestBuildResult FStageZeroManifestBuilder::BuildManifestContents(UStageZeroWarmupManifest* Manifest, const FStageZeroManifestBuildOptions& Options)
{
    FStageZeroManifestBuildResult Result;
    if (!Manifest)
    {
        Result.FailureReason = TEXT("Manifest target was null");
        return Result;
    }

#if !WITH_EDITOR
    UE_LOG(LogStageZeroWarmupManifest, Warning, TEXT("StageZero: BuildMatTexManifestContents called outside editor; no-op"));
    (void)Options;
    Result.FailureReason = TEXT("BuildMatTexManifestContents called outside editor");
    return Result;
#else
    SZ_ResetManifestData(Manifest);

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
    AssetRegistry.SearchAllAssets(true);
    AssetRegistry.WaitForCompletion();

    struct FStageZeroPackageAssets
    {
        TArray<FSoftObjectPath> Materials;
        TArray<FSoftObjectPath> Textures;
        TArray<FSoftObjectPath> NiagaraSystems;
        TArray<FSoftObjectPath> StaticMeshes;
        TArray<FSoftObjectPath> SkeletalMeshes;
        TArray<FAssetData> AnchorAssets;
        TArray<FAssetData> SplineAssets;

        bool HasTypedAssets() const
        {
            return Materials.Num() > 0
                || Textures.Num() > 0
                || NiagaraSystems.Num() > 0
                || StaticMeshes.Num() > 0
                || SkeletalMeshes.Num() > 0;
        }
    };

    enum class EStageZeroExplicitAssetType : uint8
    {
        Material,
        Texture,
        NiagaraSystem,
        StaticMesh,
        SkeletalMesh
    };

    struct FStageZeroExplicitAssetSet
    {
        TArray<FSoftObjectPath> Materials;
        TArray<FSoftObjectPath> Textures;
        TArray<FSoftObjectPath> NiagaraSystems;
        TArray<FSoftObjectPath> StaticMeshes;
        TArray<FSoftObjectPath> SkeletalMeshes;

        void Add(const EStageZeroExplicitAssetType Type, const FSoftObjectPath& Path)
        {
            switch (Type)
            {
                case EStageZeroExplicitAssetType::Material: Materials.AddUnique(Path); break;
                case EStageZeroExplicitAssetType::Texture: Textures.AddUnique(Path); break;
                case EStageZeroExplicitAssetType::NiagaraSystem: NiagaraSystems.AddUnique(Path); break;
                case EStageZeroExplicitAssetType::StaticMesh: StaticMeshes.AddUnique(Path); break;
                case EStageZeroExplicitAssetType::SkeletalMesh: SkeletalMeshes.AddUnique(Path); break;
            }
        }

        void AppendTo(
            TArray<FSoftObjectPath>& OutMaterials,
            TArray<FSoftObjectPath>& OutTextures,
            TArray<FSoftObjectPath>& OutNiagaraSystems,
            TArray<FSoftObjectPath>& OutStaticMeshes,
            TArray<FSoftObjectPath>& OutSkeletalMeshes) const
        {
            OutMaterials.Append(Materials);
            OutTextures.Append(Textures);
            OutNiagaraSystems.Append(NiagaraSystems);
            OutStaticMeshes.Append(StaticMeshes);
            OutSkeletalMeshes.Append(SkeletalMeshes);
        }

        void RemoveFrom(
            TArray<FSoftObjectPath>& OutMaterials,
            TArray<FSoftObjectPath>& OutTextures,
            TArray<FSoftObjectPath>& OutNiagaraSystems,
            TArray<FSoftObjectPath>& OutStaticMeshes,
            TArray<FSoftObjectPath>& OutSkeletalMeshes) const
        {
            for (const FSoftObjectPath& Path : Materials) { OutMaterials.Remove(Path); }
            for (const FSoftObjectPath& Path : Textures) { OutTextures.Remove(Path); }
            for (const FSoftObjectPath& Path : NiagaraSystems) { OutNiagaraSystems.Remove(Path); }
            for (const FSoftObjectPath& Path : StaticMeshes) { OutStaticMeshes.Remove(Path); }
            for (const FSoftObjectPath& Path : SkeletalMeshes) { OutSkeletalMeshes.Remove(Path); }
        }

        bool HasTextures() const
        {
            return Textures.Num() > 0;
        }
    };

    auto DedupPaths = [](TArray<FSoftObjectPath>& Paths)
    {
        TSet<FSoftObjectPath> PathSet(Paths);
        Paths = PathSet.Array();
        Paths.Sort([](const FSoftObjectPath& Left, const FSoftObjectPath& Right)
        {
            return Left.ToString() < Right.ToString();
        });
    };

    TArray<FString> NeverCookPackageRoots;
    if (const UProjectPackagingSettings* PackagingSettings = GetDefault<UProjectPackagingSettings>())
    {
        for (const FDirectoryPath& Directory : PackagingSettings->DirectoriesToNeverCook)
        {
            FString Root = Directory.Path;
            Root.TrimStartAndEndInline();
            Root.ReplaceInline(TEXT("\\"), TEXT("/"));
            Root.RemoveFromEnd(TEXT("/"));
            if (FPackageName::IsValidLongPackageName(Root))
            {
                NeverCookPackageRoots.AddUnique(Root);
            }
        }
    }
    NeverCookPackageRoots.Sort();

    UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
    TMap<FName, bool> CookPolicyCache;
    auto IsCookPolicyAllowed = [&](const FName& PackageName)
    {
        if (const bool* Cached = CookPolicyCache.Find(PackageName))
        {
            return *Cached;
        }

        const FString PackageString = PackageName.ToString();
        bool bAllowed = !NeverCookPackageRoots.ContainsByPredicate([&PackageString](const FString& Root)
        {
            return PackageString.Equals(Root, ESearchCase::IgnoreCase)
                || PackageString.StartsWith(Root + TEXT("/"), ESearchCase::IgnoreCase);
        });
        if (bAllowed && AssetManager)
        {
            bAllowed = AssetManager->VerifyCanCookPackage(nullptr, PackageName, false);
        }
        CookPolicyCache.Add(PackageName, bAllowed);
        return bAllowed;
    };

    TMap<FName, FStageZeroPackageAssets> PackageAssetCache;

    auto CollectPackageAssets = [&](const FName& PackageName) -> const FStageZeroPackageAssets&
    {
        if (const FStageZeroPackageAssets* Found = PackageAssetCache.Find(PackageName))
        {
            return *Found;
        }

        FStageZeroPackageAssets& CacheEntry = PackageAssetCache.Add(PackageName);
        if (!FStageZeroWarmupUtils::IsRuntimeWarmupPackageAllowed(PackageName)
            || !IsCookPolicyAllowed(PackageName))
        {
            UE_LOG(LogStageZeroWarmupManifest, Verbose, TEXT("StageZero: Skipping non-runtime package %s"), *PackageName.ToString());
            return CacheEntry;
        }

        TArray<FAssetData> PackageAssets;
        AssetRegistry.GetAssetsByPackageName(PackageName, PackageAssets);

        for (const FAssetData& Asset : PackageAssets)
        {
            if ((Asset.PackageFlags & PKG_EditorOnly) != 0 || Asset.IsRedirector())
            {
                continue;
            }
            if (Asset.IsInstanceOf(UMaterialInterface::StaticClass()))
            {
                CacheEntry.Materials.Add(Asset.ToSoftObjectPath());
                continue;
            }

            if (Asset.IsInstanceOf(UTextureRenderTarget::StaticClass()))
            {
                // Render targets are writable GPU resources, not content textures.
                // Loading them during runtime warmup can allocate RHI memory without
                // contributing shader or texture-streaming coverage.
                continue;
            }

            if (Asset.IsInstanceOf(UTexture::StaticClass()))
            {
                CacheEntry.Textures.Add(Asset.ToSoftObjectPath());
                continue;
            }

            if (Asset.IsInstanceOf(UNiagaraSystem::StaticClass()))
            {
                CacheEntry.NiagaraSystems.Add(Asset.ToSoftObjectPath());
                continue;
            }

            if (Asset.IsInstanceOf(UStaticMesh::StaticClass()))
            {
                CacheEntry.StaticMeshes.Add(Asset.ToSoftObjectPath());
                continue;
            }

            if (Asset.IsInstanceOf(USkeletalMesh::StaticClass()))
            {
                CacheEntry.SkeletalMeshes.Add(Asset.ToSoftObjectPath());
                continue;
            }

            if (Asset.IsInstanceOf(AStageZeroWPAnchor::StaticClass()))
            {
                CacheEntry.AnchorAssets.Add(Asset);
                continue;
            }

            if (Asset.IsInstanceOf(AStageZeroWPSplineWarmup::StaticClass()))
            {
                CacheEntry.SplineAssets.Add(Asset);
                continue;
            }
        }

        DedupPaths(CacheEntry.Materials);
        DedupPaths(CacheEntry.Textures);
        DedupPaths(CacheEntry.NiagaraSystems);
        DedupPaths(CacheEntry.StaticMeshes);
        DedupPaths(CacheEntry.SkeletalMeshes);
        return CacheEntry;
    };

    // Stage Zero's anchor and spline actors are editor-only authoring markers.
    // Their packages can legitimately be marked PKG_EditorOnly or NeverCook, so
    // discover them through a separate cache that never feeds runtime asset paths.
    TMap<FName, FStageZeroPackageAssets> EditorMarkerAssetCache;
    auto CollectEditorMarkerAssets = [&](const FName& PackageName) -> const FStageZeroPackageAssets&
    {
        if (const FStageZeroPackageAssets* Found = EditorMarkerAssetCache.Find(PackageName))
        {
            return *Found;
        }

        FStageZeroPackageAssets& CacheEntry = EditorMarkerAssetCache.Add(PackageName);
        if (PackageName == NAME_None)
        {
            return CacheEntry;
        }

        TArray<FAssetData> PackageAssets;
        AssetRegistry.GetAssetsByPackageName(PackageName, PackageAssets);
        for (const FAssetData& Asset : PackageAssets)
        {
            if (Asset.IsRedirector())
            {
                continue;
            }

            if (Asset.IsInstanceOf(AStageZeroWPAnchor::StaticClass()))
            {
                CacheEntry.AnchorAssets.Add(Asset);
            }
            else if (Asset.IsInstanceOf(AStageZeroWPSplineWarmup::StaticClass()))
            {
                CacheEntry.SplineAssets.Add(Asset);
            }
        }

        CacheEntry.AnchorAssets.Sort([](const FAssetData& Left, const FAssetData& Right)
        {
            return Left.ToSoftObjectPath().ToString() < Right.ToSoftObjectPath().ToString();
        });
        CacheEntry.SplineAssets.Sort([](const FAssetData& Left, const FAssetData& Right)
        {
            return Left.ToSoftObjectPath().ToString() < Right.ToSoftObjectPath().ToString();
        });
        return CacheEntry;
    };

    const bool bIncludeEngineContent = Options.bIncludeEngineContent;

    TArray<FName> ManifestRootPaths;
    ManifestRootPaths.Add(FName(TEXT("/Game")));
    for (const TSharedRef<IPlugin>& Plugin : IPluginManager::Get().GetEnabledPluginsWithContent())
    {
        if (Plugin->GetName().Equals(TEXT("StageZero"), ESearchCase::IgnoreCase))
        {
            // Loading-screen/editor presentation assets are not gameplay warmup targets.
            continue;
        }

        const EPluginType PluginType = Plugin->GetType();
        const bool bProjectContentPlugin = PluginType == EPluginType::Project
            || PluginType == EPluginType::External
            || PluginType == EPluginType::Mod;
        if (!bProjectContentPlugin && !bIncludeEngineContent)
        {
            continue;
        }

        FString MountedAssetPath = Plugin->GetMountedAssetPath();
        MountedAssetPath.RemoveFromEnd(TEXT("/"));
        if (MountedAssetPath.StartsWith(TEXT("/")) && MountedAssetPath.Len() > 1)
        {
            ManifestRootPaths.AddUnique(FName(*MountedAssetPath));
        }
    }
    if (bIncludeEngineContent)
    {
        ManifestRootPaths.AddUnique(FName(TEXT("/Engine")));
    }
    ManifestRootPaths.Sort(FNameLexicalLess());

    auto IsEnginePackage = [](const FName& PackageName)
    {
        return PackageName.ToString().StartsWith(TEXT("/Engine/"));
    };

    auto IsPackageUnderManifestRoots = [&ManifestRootPaths](const FName& PackageName)
    {
        const FString PackageString = PackageName.ToString();
        return ManifestRootPaths.ContainsByPredicate([&PackageString](const FName& RootName)
        {
            const FString Root = RootName.ToString();
            return PackageString.Equals(Root, ESearchCase::IgnoreCase)
                || PackageString.StartsWith(Root + TEXT("/"), ESearchCase::IgnoreCase);
        });
    };

    auto ShouldIncludePackage = [&](const FName& PackageName)
    {
        return FStageZeroWarmupUtils::IsRuntimeWarmupPackageAllowed(PackageName)
            && IsCookPolicyAllowed(PackageName);
    };

    auto ShouldDiscoverMapPackage = [&](const FName& PackageName)
    {
        return ShouldIncludePackage(PackageName) && IsPackageUnderManifestRoots(PackageName);
    };

    auto FilterRuntimePaths = [&ShouldIncludePackage, &AssetRegistry](TArray<FSoftObjectPath>& Paths, const TCHAR* AssetLabel, const FName& ContextPackage)
    {
        const bool bFilteringTextures = AssetLabel && FCString::Stricmp(AssetLabel, TEXT("texture")) == 0;
        const int32 Before = Paths.Num();
        Paths.RemoveAll([&ShouldIncludePackage, &AssetRegistry, bFilteringTextures](const FSoftObjectPath& Path)
        {
            if (!FStageZeroWarmupUtils::IsRuntimeWarmupAssetPathAllowed(Path))
            {
                return true;
            }

            const FString PackageName = Path.GetLongPackageName();
            if (PackageName.IsEmpty() || !ShouldIncludePackage(FName(*PackageName)))
            {
                return true;
            }

            if (bFilteringTextures)
            {
                const FAssetData TextureAsset = StageZeroAssetRegistryCompat::GetAssetByObjectPath(AssetRegistry, Path);
                if (TextureAsset.IsValid() && TextureAsset.IsInstanceOf(UTextureRenderTarget::StaticClass()))
                {
                    return true;
                }
            }

            return false;
        });

        const int32 Removed = Before - Paths.Num();
        if (Removed > 0)
        {
            UE_LOG(LogStageZeroWarmupManifest, Warning, TEXT("StageZero: Filtered %d non-runtime or non-cookable %s reference(s) while building manifest for %s."),
                Removed,
                AssetLabel ? AssetLabel : TEXT("asset"),
                *ContextPackage.ToString());
        }
    };

    auto DedupAndUnion = [&DedupPaths, &FilterRuntimePaths, &Options](
        TArray<FSoftObjectPath>& Materials,
        TArray<FSoftObjectPath>& Textures,
        TArray<FSoftObjectPath>& NiagaraSystems,
        TArray<FSoftObjectPath>& StaticMeshes,
        TArray<FSoftObjectPath>& SkeletalMeshes,
        TArray<FSoftObjectPath>* OutAssets = nullptr,
        const FName& ContextPackage = NAME_None,
        const bool bPreserveExplicitTextures = false)
    {
        // Reflection-discovered hard and soft references do not necessarily pass through
        // the Asset Registry dependency closure. Apply the final runtime/cooker policy to
        // every emitted path so DirectoriesToNeverCook and Asset Manager exclusions cannot
        // leak into the manifest through direct object properties.
        FilterRuntimePaths(Materials, TEXT("material"), ContextPackage);
        FilterRuntimePaths(Textures, TEXT("texture"), ContextPackage);
        FilterRuntimePaths(NiagaraSystems, TEXT("Niagara"), ContextPackage);
        FilterRuntimePaths(StaticMeshes, TEXT("static mesh"), ContextPackage);
        FilterRuntimePaths(SkeletalMeshes, TEXT("skeletal mesh"), ContextPackage);

        DedupPaths(Materials);
        if (Options.bIncludeTextures || bPreserveExplicitTextures)
        {
            DedupPaths(Textures);
        }
        else
        {
            Textures.Reset();
        }
        DedupPaths(NiagaraSystems);
        DedupPaths(StaticMeshes);
        DedupPaths(SkeletalMeshes);
        if (OutAssets)
        {
            TSet<FSoftObjectPath> AllAssets;
            AllAssets.Append(Materials);
            AllAssets.Append(Textures);
            AllAssets.Append(NiagaraSystems);
            AllAssets.Append(StaticMeshes);
            AllAssets.Append(SkeletalMeshes);
            *OutAssets = AllAssets.Array();
        }
    };

    auto ApplyManifestRootFilter = [&](FARFilter& Filter)
    {
        for (const FName& RootPath : ManifestRootPaths)
        {
            Filter.PackagePaths.Add(RootPath);
        }
        Filter.bRecursivePaths = true;
    };

    auto CountRegistryAssetsByClass = [&](const UClass* AssetClass, bool bRecursiveClasses)
    {
        FARFilter Filter;
        ApplyManifestRootFilter(Filter);
        StageZeroAssetRegistryCompat::AddClassToFilter(Filter, AssetClass);
        Filter.bRecursiveClasses = bRecursiveClasses;

        TArray<FAssetData> Assets;
        StageZeroAssetRegistryCompat::GetAssets(AssetRegistry, Filter, Assets, true);
        int32 RuntimeAssetCount = 0;
        for (const FAssetData& Asset : Assets)
        {
            if ((Asset.PackageFlags & PKG_EditorOnly) == 0
                && !Asset.IsRedirector()
                && !Asset.IsInstanceOf(UTextureRenderTarget::StaticClass())
                && ShouldIncludePackage(Asset.PackageName))
            {
                ++RuntimeAssetCount;
            }
        }
        return RuntimeAssetCount;
    };

    UE_LOG(
        LogStageZeroWarmupManifest,
        Display,
        TEXT("StageZeroManifestInventory: Roots=%s Maps=%d Materials=%d Textures=%d Niagara=%d StaticMeshes=%d SkeletalMeshes=%d"),
        *FString::JoinBy(ManifestRootPaths, TEXT(","), [](const FName& RootPath) { return RootPath.ToString(); }),
        CountRegistryAssetsByClass(UWorld::StaticClass(), true),
        CountRegistryAssetsByClass(UMaterialInterface::StaticClass(), true),
        CountRegistryAssetsByClass(UTexture::StaticClass(), true),
        CountRegistryAssetsByClass(UNiagaraSystem::StaticClass(), true),
        CountRegistryAssetsByClass(UStaticMesh::StaticClass(), true),
        CountRegistryAssetsByClass(USkeletalMesh::StaticClass(), true));

    auto AppendRegistryAssetsByClass = [&](const UClass* AssetClass, bool bRecursiveClasses, TSet<FSoftObjectPath>& OutPaths)
    {
        FARFilter Filter;
        ApplyManifestRootFilter(Filter);
        StageZeroAssetRegistryCompat::AddClassToFilter(Filter, AssetClass);
        Filter.bRecursiveClasses = bRecursiveClasses;

        TArray<FAssetData> Assets;
        StageZeroAssetRegistryCompat::GetAssets(AssetRegistry, Filter, Assets, true);
        for (const FAssetData& Asset : Assets)
        {
            if ((Asset.PackageFlags & PKG_EditorOnly) == 0
                && !Asset.IsRedirector()
                && !Asset.IsInstanceOf(UTextureRenderTarget::StaticClass())
                && ShouldIncludePackage(Asset.PackageName))
            {
                OutPaths.Add(Asset.ToSoftObjectPath());
            }
        }
    };

    TMap<FName, TArray<FName>> SortedDependencyAdjacencyCache;
    auto GetSortedPackageDependencies = [&AssetRegistry, &SortedDependencyAdjacencyCache](const FName& PackageName) -> const TArray<FName>&
    {
        if (const TArray<FName>* Cached = SortedDependencyAdjacencyCache.Find(PackageName))
        {
            return *Cached;
        }

        TArray<FName> Dependencies;
        {
            using namespace UE::AssetRegistry;
            FDependencyQuery DependencyQuery;
            AssetRegistry.GetDependencies(PackageName, Dependencies, EDependencyCategory::Package, DependencyQuery);
        }
        Dependencies.Sort(FNameLexicalLess());
        return SortedDependencyAdjacencyCache.Add(PackageName, MoveTemp(Dependencies));
    };

    bool bDependencyTraversalFailed = false;
    FString DependencyTraversalFailureReason;
    auto RecordDependencyTraversalFailure = [&bDependencyTraversalFailed, &DependencyTraversalFailureReason](const FString& Reason)
    {
        bDependencyTraversalFailed = true;
        if (DependencyTraversalFailureReason.IsEmpty())
        {
            DependencyTraversalFailureReason = Reason;
        }
    };

    constexpr int32 MaxGeneralDependencyPackages = 16384;
    auto ExpandDependencyClosure = [&](const TSet<FName>& Roots, TSet<FName>& OutPackages)
    {
        OutPackages.Reset();
        TQueue<FName> Worklist;
        TArray<FName> SortedRoots = Roots.Array();
        SortedRoots.Sort(FNameLexicalLess());
        for (const FName& Root : SortedRoots)
        {
            if (!ShouldIncludePackage(Root))
            {
                continue;
            }

            if (OutPackages.Num() >= MaxGeneralDependencyPackages)
            {
                RecordDependencyTraversalFailure(FString::Printf(
                    TEXT("Dependency traversal exceeded its %d-package safety bound while seeding root %s"),
                    MaxGeneralDependencyPackages,
                    *Root.ToString()));
                return false;
            }

            OutPackages.Add(Root);
            Worklist.Enqueue(Root);
        }

        while (!Worklist.IsEmpty())
        {
            FName PackageName;
            Worklist.Dequeue(PackageName);
            for (const FName& Dep : GetSortedPackageDependencies(PackageName))
            {
                if (!ShouldIncludePackage(Dep))
                {
                    continue;
                }

                if (!OutPackages.Contains(Dep))
                {
                    if (OutPackages.Num() >= MaxGeneralDependencyPackages)
                    {
                        RecordDependencyTraversalFailure(FString::Printf(
                            TEXT("Dependency traversal from %s exceeded its %d-package safety bound"),
                            *PackageName.ToString(),
                            MaxGeneralDependencyPackages));
                        return false;
                    }

                    OutPackages.Add(Dep);
                    Worklist.Enqueue(Dep);
                }
            }
        }
        return true;
    };

    auto AppendAssetsFromPackages = [&](const TSet<FName>& Packages, FStageZeroMapMatTexEntry& OutEntry)
    {
        for (const FName& PackageName : Packages)
        {
            if (!ShouldIncludePackage(PackageName))
            {
                continue;
            }

            const FStageZeroPackageAssets& PackageAssets = CollectPackageAssets(PackageName);
            OutEntry.Materials.Append(PackageAssets.Materials);
            if (Options.bIncludeTextures)
            {
                OutEntry.Textures.Append(PackageAssets.Textures);
            }
            OutEntry.NiagaraSystems.Append(PackageAssets.NiagaraSystems);
            OutEntry.StaticMeshes.Append(PackageAssets.StaticMeshes);
            OutEntry.SkeletalMeshes.Append(PackageAssets.SkeletalMeshes);
        }
    };

    auto AppendTypedPackageAssets = [&Options](FStageZeroPackageAssets& Destination, const FStageZeroPackageAssets& Source)
    {
        Destination.Materials.Append(Source.Materials);
        if (Options.bIncludeTextures)
        {
            Destination.Textures.Append(Source.Textures);
        }
        Destination.NiagaraSystems.Append(Source.NiagaraSystems);
        Destination.StaticMeshes.Append(Source.StaticMeshes);
        Destination.SkeletalMeshes.Append(Source.SkeletalMeshes);
    };

    auto FinalizeTypedPackageAssets = [&DedupPaths, &Options](FStageZeroPackageAssets& Assets)
    {
        DedupPaths(Assets.Materials);
        if (Options.bIncludeTextures)
        {
            DedupPaths(Assets.Textures);
        }
        else
        {
            Assets.Textures.Reset();
        }
        DedupPaths(Assets.NiagaraSystems);
        DedupPaths(Assets.StaticMeshes);
        DedupPaths(Assets.SkeletalMeshes);
    };

    struct FStageZeroDependencyAssetCacheEntry
    {
        FStageZeroPackageAssets Assets;
        bool bComplete = true;
    };

    constexpr int32 MaxActorDependencyPackages = 512;
    constexpr int32 MaxActorDependencyDepth = 8;
    TMap<FName, FStageZeroDependencyAssetCacheEntry> ActorDependencyAssetCache;
    TMap<FName, bool> WorldPackageCache;

    auto IsWorldAssetPackage = [&AssetRegistry, &WorldPackageCache](const FName& PackageName)
    {
        if (const bool* Cached = WorldPackageCache.Find(PackageName))
        {
            return *Cached;
        }

        TArray<FAssetData> PackageAssets;
        AssetRegistry.GetAssetsByPackageName(PackageName, PackageAssets);
        const bool bContainsWorld = PackageAssets.ContainsByPredicate([](const FAssetData& Asset)
        {
            return StageZeroAssetRegistryCompat::IsAssetClassExact(Asset, UWorld::StaticClass());
        });
        WorldPackageCache.Add(PackageName, bContainsWorld);
        return bContainsWorld;
    };

    auto CollectActorDependencyAssets = [&](const FName& RootPackage) -> const FStageZeroPackageAssets&
    {
        if (const FStageZeroDependencyAssetCacheEntry* Found = ActorDependencyAssetCache.Find(RootPackage))
        {
            return Found->Assets;
        }

        FStageZeroDependencyAssetCacheEntry& CacheEntry = ActorDependencyAssetCache.Add(RootPackage);
        if (bDependencyTraversalFailed || !ShouldIncludePackage(RootPackage))
        {
            return CacheEntry.Assets;
        }

        struct FDependencyWorkItem
        {
            FName PackageName = NAME_None;
            int32 Depth = 0;
        };

        TSet<FName> VisitedPackages;
        TQueue<FDependencyWorkItem> Worklist;
        VisitedPackages.Add(RootPackage);
        Worklist.Enqueue(FDependencyWorkItem{ RootPackage, 0 });

        while (!Worklist.IsEmpty())
        {
            FDependencyWorkItem WorkItem;
            Worklist.Dequeue(WorkItem);

            const FStageZeroPackageAssets& DirectAssets = CollectPackageAssets(WorkItem.PackageName);
            AppendTypedPackageAssets(CacheEntry.Assets, DirectAssets);

            const TArray<FName>& Dependencies = GetSortedPackageDependencies(WorkItem.PackageName);

            if (WorkItem.Depth >= MaxActorDependencyDepth)
            {
                CacheEntry.bComplete = !Dependencies.ContainsByPredicate([&](const FName& Dependency)
                {
                    return ShouldIncludePackage(Dependency)
                        && !VisitedPackages.Contains(Dependency)
                        && !IsWorldAssetPackage(Dependency);
                });
                if (!CacheEntry.bComplete)
                {
                    break;
                }
                continue;
            }

            for (const FName& Dependency : Dependencies)
            {
                if (!ShouldIncludePackage(Dependency)
                    || VisitedPackages.Contains(Dependency)
                    || IsWorldAssetPackage(Dependency))
                {
                    continue;
                }

                if (VisitedPackages.Num() >= MaxActorDependencyPackages)
                {
                    CacheEntry.bComplete = false;
                    break;
                }

                VisitedPackages.Add(Dependency);
                Worklist.Enqueue(FDependencyWorkItem{ Dependency, WorkItem.Depth + 1 });
            }

            if (!CacheEntry.bComplete)
            {
                break;
            }
        }

        FinalizeTypedPackageAssets(CacheEntry.Assets);
        if (!CacheEntry.bComplete)
        {
            RecordDependencyTraversalFailure(FString::Printf(
                TEXT("External-actor dependency traversal exceeded its safety bound for %s (MaxPackages=%d MaxDepth=%d)"),
                *RootPackage.ToString(),
                MaxActorDependencyPackages,
                MaxActorDependencyDepth));
        }
        return CacheEntry.Assets;
    };

    TMap<FName, TArray<FSoftObjectPath>> MaterialTextureDependencyCache;
    auto AppendTextureDependenciesForMaterials = [&](const TArray<FSoftObjectPath>& Materials, TArray<FSoftObjectPath>& OutTextures, const FName& ContextPackage)
    {
        if (!Options.bIncludeTextures || Materials.Num() == 0)
        {
            return;
        }

        TSet<FName> RootPackages;
        for (const FSoftObjectPath& MaterialPath : Materials)
        {
            const FString MaterialPackageName = MaterialPath.GetLongPackageName();
            if (!MaterialPackageName.IsEmpty())
            {
                const FName MaterialPackage(*MaterialPackageName);
                if (ShouldIncludePackage(MaterialPackage))
                {
                    RootPackages.Add(MaterialPackage);
                }
            }
        }

        if (RootPackages.Num() == 0)
        {
            return;
        }

        TSet<FSoftObjectPath> ExistingTextures;
        ExistingTextures.Append(OutTextures);

        int32 AddedTextures = 0;
        TArray<FName> SortedRootPackages = RootPackages.Array();
        SortedRootPackages.Sort(FNameLexicalLess());
        for (const FName& RootPackage : SortedRootPackages)
        {
            if (bDependencyTraversalFailed)
            {
                return;
            }

            TArray<FSoftObjectPath>* CachedTextures = MaterialTextureDependencyCache.Find(RootPackage);
            if (!CachedTextures)
            {
                TSet<FName> SingleRoot;
                SingleRoot.Add(RootPackage);
                TSet<FName> DependencyPackages;
                ExpandDependencyClosure(SingleRoot, DependencyPackages);

                TArray<FSoftObjectPath>& NewCacheEntry = MaterialTextureDependencyCache.Add(RootPackage);
                for (const FName& PackageName : DependencyPackages)
                {
                    NewCacheEntry.Append(CollectPackageAssets(PackageName).Textures);
                }
                DedupPaths(NewCacheEntry);
                CachedTextures = &NewCacheEntry;
            }

            for (const FSoftObjectPath& TexturePath : *CachedTextures)
            {
                if (!ExistingTextures.Contains(TexturePath))
                {
                    ExistingTextures.Add(TexturePath);
                    OutTextures.Add(TexturePath);
                    ++AddedTextures;
                }
            }
        }

        if (AddedTextures > 0)
        {
            UE_LOG(LogStageZeroWarmupManifest, Display, TEXT("StageZero: Material dependency texture fallback for %s added %d texture(s) from %d material package(s)."),
                *ContextPackage.ToString(),
                AddedTextures,
                RootPackages.Num());
        }
    };

    // If user selected specific maps, force-scan their root paths and filenames so the registry sees them.
    if (!Options.bUseAllMaps && Options.SelectedMaps.Num() > 0)
    {
        TArray<FString> SelectedRootPaths;
        TArray<FString> SelectedFilenames;
        SelectedRootPaths.Reserve(Options.SelectedMaps.Num());
        SelectedFilenames.Reserve(Options.SelectedMaps.Num());
        for (const FSoftObjectPath& Path : Options.SelectedMaps)
        {
            const FString PackageName = Path.GetLongPackageName();
            const FString RootPath = FPackageName::GetLongPackagePath(PackageName);
            if (!RootPath.IsEmpty())
            {
                SelectedRootPaths.AddUnique(RootPath);
            }

            FString Filename;
            if (FPackageName::TryConvertLongPackageNameToFilename(PackageName, Filename, FPackageName::GetMapPackageExtension()))
            {
                SelectedFilenames.AddUnique(Filename);
            }
        }

        if (SelectedRootPaths.Num() > 0)
        {
            AssetRegistry.ScanPathsSynchronous(SelectedRootPaths, /*bForceRescan*/ true);
        }
        if (SelectedFilenames.Num() > 0)
        {
            AssetRegistry.ScanFilesSynchronous(SelectedFilenames, /*bForceRescan*/ true);
        }
    }

    TArray<FAssetData> WorldAssetsRaw;
    {
        FARFilter WorldFilter;
        ApplyManifestRootFilter(WorldFilter);
        StageZeroAssetRegistryCompat::AddClassToFilter(WorldFilter, UWorld::StaticClass());
        WorldFilter.bRecursiveClasses = true;
        StageZeroAssetRegistryCompat::GetAssets(AssetRegistry, WorldFilter, WorldAssetsRaw, true);
    }

    TArray<FAssetData> WorldAssets;
    WorldAssets.Reserve(WorldAssetsRaw.Num());
    for (const FAssetData& Asset : WorldAssetsRaw)
    {
        if (!ShouldDiscoverMapPackage(Asset.PackageName))
        {
            continue;
        }
        WorldAssets.Add(Asset);
    }

    TArray<FAssetData> TargetWorldAssets;

    auto AddTargetWorldAsset = [&](const FAssetData& InAsset)
    {
        if (!StageZeroAssetRegistryCompat::IsAssetClassExact(InAsset, UWorld::StaticClass()))
        {
            return false;
        }

        if (!ShouldDiscoverMapPackage(InAsset.PackageName))
        {
            return false;
        }

        if (TargetWorldAssets.ContainsByPredicate([&](const FAssetData& Data)
        {
            return Data.PackageName == InAsset.PackageName;
        }))
        {
            return false;
        }

        TargetWorldAssets.Add(InAsset);
        return true;
    };

    if (Options.bUseAllMaps)
    {
        for (const FAssetData& WorldAsset : WorldAssets)
        {
            AddTargetWorldAsset(WorldAsset);
            Result.RequestedMapPackages.Add(WorldAsset.PackageName.ToString());
        }
        Result.NumRequestedMaps = Result.RequestedMapPackages.Num();
    }
    else
    {
        TMap<FName, FString> SelectedDisplayPaths;
        TMap<FName, FSoftObjectPath> SelectedObjectPaths;
        TSet<FName> SelectedPackages;
        TMap<FName, FString> UnresolvedReasons;

        auto AddMissingRequestedMap = [&Result](const FString& Map, const FString& Reason)
        {
            if (!Result.MissingRequestedMaps.Contains(Map))
            {
                Result.MissingRequestedMaps.Add(Map);
                Result.MissingRequestedMapReasons.Add(FString::Printf(TEXT("%s:%s"), *Map, *Reason));
            }
        };

        for (const FSoftObjectPath& Path : Options.SelectedMaps)
        {
            const FString LongName = Path.GetLongPackageName();
            const FString DisplayPath = Path.ToString().TrimStartAndEnd();
            if (LongName.IsEmpty() || !FPackageName::IsValidLongPackageName(LongName))
            {
                AddMissingRequestedMap(DisplayPath.IsEmpty() ? TEXT("<empty>") : DisplayPath, TEXT("InvalidPackageName"));
                continue;
            }

            const FName PackageName(*LongName);
            SelectedPackages.Add(PackageName);
            SelectedDisplayPaths.Add(PackageName, DisplayPath);
            SelectedObjectPaths.Add(PackageName, Path);
        }

        TArray<FName> RequestedPackages = SelectedPackages.Array();
        RequestedPackages.Sort([](const FName& Left, const FName& Right)
        {
            return Left.LexicalLess(Right);
        });
        for (const FName& PackageName : RequestedPackages)
        {
            Result.RequestedMapPackages.Add(PackageName.ToString());
        }
        Result.NumRequestedMaps = Result.RequestedMapPackages.Num() + Result.MissingRequestedMaps.Num();

        TSet<FName> MatchedRequestedPackages;

        // Ensure explicitly selected maps are added even if they weren't present in the initial scan.
        for (const FName& PackageName : RequestedPackages)
        {
            if (!ShouldDiscoverMapPackage(PackageName))
            {
                const bool bEngineMapDisabled = !bIncludeEngineContent && IsEnginePackage(PackageName);
                UnresolvedReasons.Add(PackageName, bEngineMapDisabled ? TEXT("EngineContentDisabled") : TEXT("OutsideManifestRootsOrNonRuntime"));
                continue;
            }

            FAssetData FoundAsset;
            if (const FAssetData* ExistingWorldAsset = WorldAssets.FindByPredicate([&](const FAssetData& Candidate)
            {
                return Candidate.PackageName == PackageName;
            }))
            {
                FoundAsset = *ExistingWorldAsset;
            }
            else if (const FSoftObjectPath* RequestedPath = SelectedObjectPaths.Find(PackageName))
            {
                FoundAsset = StageZeroAssetRegistryCompat::GetAssetByObjectPath(AssetRegistry, *RequestedPath);
            }

            if (!FoundAsset.IsValid())
            {
                if (UWorld* LoadedWorld = SZ_ManifestLoadWorldByPackageName(PackageName))
                {
                    FoundAsset = FAssetData(LoadedWorld, /*bAllowBlueprintClass*/ true);
                }
            }

            if (StageZeroAssetRegistryCompat::IsAssetClassExact(FoundAsset, UWorld::StaticClass()) && ShouldDiscoverMapPackage(FoundAsset.PackageName))
            {
                AddTargetWorldAsset(FoundAsset);
                MatchedRequestedPackages.Add(PackageName);
                continue;
            }

            UnresolvedReasons.Add(PackageName, TEXT("NotFoundOrNotWorld"));
        }

        if (MatchedRequestedPackages.Num() < RequestedPackages.Num())
        {
            UE_LOG(LogStageZeroWarmupManifest, Display, TEXT("StageZero: Exact package matching is required; unmatched selected maps will be reported as missing."));
        }

        for (const FName& PackageName : RequestedPackages)
        {
            if (MatchedRequestedPackages.Contains(PackageName))
            {
                continue;
            }

            const FString PackageString = PackageName.ToString();
            const FString* Display = SelectedDisplayPaths.Find(PackageName);
            const FString* Reason = UnresolvedReasons.Find(PackageName);
            AddMissingRequestedMap(Display ? *Display : PackageString, Reason ? *Reason : TEXT("Unresolved"));
        }

        Result.NumMissingRequestedMaps = Result.MissingRequestedMaps.Num();
    }

    // Safety dedup: keep a single world entry per package.
    if (TargetWorldAssets.Num() > 1)
    {
        TSet<FName> SeenPackages;
        TArray<FAssetData> DedupedWorldAssets;
        DedupedWorldAssets.Reserve(TargetWorldAssets.Num());
        for (const FAssetData& WorldAsset : TargetWorldAssets)
        {
            if (SeenPackages.Contains(WorldAsset.PackageName))
            {
                continue;
            }

            SeenPackages.Add(WorldAsset.PackageName);
            DedupedWorldAssets.Add(WorldAsset);
        }

        if (DedupedWorldAssets.Num() != TargetWorldAssets.Num())
        {
            UE_LOG(LogStageZeroWarmupManifest, Warning, TEXT("StageZero: Removed %d duplicate map selection(s) before manifest build."),
                TargetWorldAssets.Num() - DedupedWorldAssets.Num());
        }

        TargetWorldAssets = MoveTemp(DedupedWorldAssets);
    }

    TargetWorldAssets.Sort([](const FAssetData& Left, const FAssetData& Right)
    {
        return Left.PackageName.LexicalLess(Right.PackageName);
    });

    Result.NumResolvedMaps = TargetWorldAssets.Num();
    Result.RequestedMapPackages.Sort();
    for (const FAssetData& WorldAsset : TargetWorldAssets)
    {
        Result.ResolvedMapPackages.Add(WorldAsset.PackageName.ToString());
    }

    if (!Options.bUseAllMaps && Result.NumMissingRequestedMaps > 0)
    {
        Result.FailureReason = FString::Printf(
            TEXT("Strict map selection failed: %d of %d requested maps were unresolved"),
            Result.NumMissingRequestedMaps,
            Result.NumRequestedMaps);
        UE_LOG(LogStageZeroWarmupManifest, Error, TEXT("StageZero: %s. Missing=%s Reasons=%s"),
            *Result.FailureReason,
            *FString::Join(Result.MissingRequestedMaps, TEXT(", ")),
            *FString::Join(Result.MissingRequestedMapReasons, TEXT(", ")));
        return Result;
    }

    if (TargetWorldAssets.Num() == 0)
    {
        UE_LOG(LogStageZeroWarmupManifest, Warning, TEXT("StageZero: No maps selected for manifest build."));
        Result.FailureReason = TEXT("No target maps were resolved for manifest build");
        return Result;
    }

    FStageZeroExplicitAssetSet GlobalAdditionalAssets;
    FStageZeroExplicitAssetSet GlobalExcludedAssets;
    TMap<FName, FStageZeroExplicitAssetSet> PerMapAdditionalAssets;
    TMap<FName, FStageZeroExplicitAssetSet> PerMapExcludedAssets;

    auto DescribeExplicitAssetRejection = [](const FSoftObjectPath& Path, const TCHAR* Reason)
    {
        return FString::Printf(TEXT("'%s' (%s)"), *Path.ToString(), Reason);
    };

    auto ClassifyExplicitAsset = [&](
        const FSoftObjectPath& RequestedPath,
        EStageZeroExplicitAssetType& OutType,
        FSoftObjectPath& OutCanonicalPath,
        FString& OutRejectionReason)
    {
        if (RequestedPath.IsNull() || RequestedPath.GetLongPackageName().IsEmpty())
        {
            OutRejectionReason = TEXT("the asset path is empty or malformed");
            return false;
        }

        const FAssetData Asset = StageZeroAssetRegistryCompat::GetAssetByObjectPath(AssetRegistry, RequestedPath);
        if (!Asset.IsValid())
        {
            OutRejectionReason = TEXT("the Asset Registry could not resolve the exact object path");
            return false;
        }
        if (Asset.IsRedirector())
        {
            OutRejectionReason = TEXT("redirectors are not runtime warmup assets; fix up redirectors and select the destination asset");
            return false;
        }
        if ((Asset.PackageFlags & PKG_EditorOnly) != 0)
        {
            OutRejectionReason = TEXT("the asset is editor-only");
            return false;
        }

        OutCanonicalPath = Asset.ToSoftObjectPath();
        if (!FStageZeroWarmupUtils::IsRuntimeWarmupAssetPathAllowed(OutCanonicalPath)
            || !ShouldIncludePackage(Asset.PackageName))
        {
            OutRejectionReason = TEXT("the package is not runtime-loadable or cookable under the current project policy");
            return false;
        }

        if (Asset.IsInstanceOf(UTextureRenderTarget::StaticClass()))
        {
            OutRejectionReason = TEXT("render targets are writable runtime GPU resources and are intentionally unsupported");
            return false;
        }
        if (Asset.IsInstanceOf(UMaterialInterface::StaticClass()))
        {
            OutType = EStageZeroExplicitAssetType::Material;
            return true;
        }
        if (Asset.IsInstanceOf(UTexture::StaticClass()))
        {
            OutType = EStageZeroExplicitAssetType::Texture;
            return true;
        }
        if (Asset.IsInstanceOf(UNiagaraSystem::StaticClass()))
        {
            OutType = EStageZeroExplicitAssetType::NiagaraSystem;
            return true;
        }
        if (Asset.IsInstanceOf(UStaticMesh::StaticClass()))
        {
            OutType = EStageZeroExplicitAssetType::StaticMesh;
            return true;
        }
        if (Asset.IsInstanceOf(USkeletalMesh::StaticClass()))
        {
            OutType = EStageZeroExplicitAssetType::SkeletalMesh;
            return true;
        }

        OutRejectionReason = TEXT("the asset type is unsupported; supported types are materials, textures, Niagara systems, static meshes, and skeletal meshes");
        return false;
    };

    auto ResolveExplicitAssets = [&](
        const TArray<FSoftObjectPath>& RequestedPaths,
        const FString& Context,
        const bool bRequiredInclude,
        FStageZeroExplicitAssetSet& OutAssets)
    {
        for (const FSoftObjectPath& RequestedPath : RequestedPaths)
        {
            EStageZeroExplicitAssetType Type = EStageZeroExplicitAssetType::Material;
            FSoftObjectPath CanonicalPath;
            FString RejectionReason;
            if (!ClassifyExplicitAsset(RequestedPath, Type, CanonicalPath, RejectionReason))
            {
                const FString Rejected = DescribeExplicitAssetRejection(RequestedPath, *RejectionReason);
                if (bRequiredInclude)
                {
                    Result.FailureReason = FString::Printf(
                        TEXT("Invalid explicit include in %s: %s. The existing manifest was preserved."),
                        *Context,
                        *Rejected);
                    return false;
                }

                Result.IgnoredExplicitExclusions.Add(FString::Printf(TEXT("%s: %s"), *Context, *Rejected));
                UE_LOG(LogStageZeroWarmupManifest, Warning,
                    TEXT("StageZero: Ignoring explicit exclusion in %s because it cannot match a supported runtime manifest asset: %s"),
                    *Context,
                    *Rejected);
                continue;
            }

            OutAssets.Add(Type, CanonicalPath);
            if (bRequiredInclude)
            {
                ++Result.NumExplicitIncludesResolved;
            }
            else
            {
                ++Result.NumExplicitExclusionsResolved;
            }
        }
        return true;
    };

    if (!ResolveExplicitAssets(
            Options.GlobalAdditionalAssets,
            TEXT("global additional assets"),
            true,
            GlobalAdditionalAssets)
        || !ResolveExplicitAssets(
            Options.GlobalExcludedAssets,
            TEXT("global excluded assets"),
            false,
            GlobalExcludedAssets))
    {
        return Result;
    }

    TSet<FName> ResolvedTargetMapPackages;
    for (const FAssetData& TargetWorldAsset : TargetWorldAssets)
    {
        ResolvedTargetMapPackages.Add(TargetWorldAsset.PackageName);
    }

    for (const FStageZeroManifestMapAssetOverrides& Override : Options.PerMapAssetOverrides)
    {
        const FString MapPackageString = Override.Map.GetLongPackageName();
        const FAssetData OverrideMapAsset = StageZeroAssetRegistryCompat::GetAssetByObjectPath(AssetRegistry, Override.Map);
        if (MapPackageString.IsEmpty()
            || !FPackageName::IsValidLongPackageName(MapPackageString)
            || !StageZeroAssetRegistryCompat::IsAssetClassExact(OverrideMapAsset, UWorld::StaticClass()))
        {
            Result.FailureReason = FString::Printf(
                TEXT("Per-map asset override has an invalid or unresolved map key '%s'. Use an exact UWorld object path. The existing manifest was preserved."),
                *Override.Map.ToString());
            return Result;
        }

        const FName MapPackage = OverrideMapAsset.PackageName;
        if (!ResolvedTargetMapPackages.Contains(MapPackage))
        {
            Result.FailureReason = FString::Printf(
                TEXT("Per-map asset override key '%s' did not resolve to a map in this build. Select that map or remove the override. The existing manifest was preserved."),
                *Override.Map.ToString());
            return Result;
        }

        FStageZeroExplicitAssetSet& Additional = PerMapAdditionalAssets.FindOrAdd(MapPackage);
        FStageZeroExplicitAssetSet& Excluded = PerMapExcludedAssets.FindOrAdd(MapPackage);
        const FString Context = FString::Printf(TEXT("map override %s"), *MapPackageString);
        if (!ResolveExplicitAssets(Override.AdditionalAssets, Context, true, Additional)
            || !ResolveExplicitAssets(Override.ExcludedAssets, Context, false, Excluded))
        {
            return Result;
        }
    }

    Result.NumExplicitOverrideMaps = PerMapAdditionalAssets.Num();
    Result.IgnoredExplicitExclusions.Sort();
    Result.IgnoredExplicitExclusions.SetNum(Algo::Unique(Result.IgnoredExplicitExclusions));
    Result.NumIgnoredExplicitExclusions = Result.IgnoredExplicitExclusions.Num();

    auto ApplyExplicitOverrides = [&](
        const FName& MapPackage,
        TArray<FSoftObjectPath>& Materials,
        TArray<FSoftObjectPath>& Textures,
        TArray<FSoftObjectPath>& NiagaraSystems,
        TArray<FSoftObjectPath>& StaticMeshes,
        TArray<FSoftObjectPath>& SkeletalMeshes,
        TArray<FSoftObjectPath>* OutAssets = nullptr)
    {
        GlobalAdditionalAssets.AppendTo(Materials, Textures, NiagaraSystems, StaticMeshes, SkeletalMeshes);
        const FStageZeroExplicitAssetSet* MapAdditional = PerMapAdditionalAssets.Find(MapPackage);
        if (MapAdditional)
        {
            MapAdditional->AppendTo(Materials, Textures, NiagaraSystems, StaticMeshes, SkeletalMeshes);
        }

        // Explicit material includes participate in the same safe, registry-only
        // texture dependency expansion as automatically discovered materials.
        AppendTextureDependenciesForMaterials(Materials, Textures, MapPackage);

        // Apply both exclusion scopes last. This precedence is deterministic even
        // when an asset appears in generated data and one or more include scopes.
        GlobalExcludedAssets.RemoveFrom(Materials, Textures, NiagaraSystems, StaticMeshes, SkeletalMeshes);
        const FStageZeroExplicitAssetSet* MapExcluded = PerMapExcludedAssets.Find(MapPackage);
        if (MapExcluded)
        {
            MapExcluded->RemoveFrom(Materials, Textures, NiagaraSystems, StaticMeshes, SkeletalMeshes);
        }

        const bool bPreserveExplicitTextures = GlobalAdditionalAssets.HasTextures()
            || (MapAdditional && MapAdditional->HasTextures());
        DedupAndUnion(
            Materials,
            Textures,
            NiagaraSystems,
            StaticMeshes,
            SkeletalMeshes,
            OutAssets,
            MapPackage,
            bPreserveExplicitTextures);
    };

    UE_LOG(LogStageZeroWarmupManifest, Display, TEXT("StageZero: Building manifest for %d map(s). WP=%s Anchors/Splines=%s Textures=%s AllProjectMaterials=%s AllProjectTextures=%s"),
        TargetWorldAssets.Num(),
        Options.bIncludeWorldPartition ? TEXT("true") : TEXT("false"),
        Options.bIncludeAnchorAndSplineRoutes ? TEXT("true") : TEXT("false"),
        Options.bIncludeTextures ? TEXT("true") : TEXT("false"),
        Options.bIncludeAllProjectMaterials ? TEXT("true") : TEXT("false"),
        Options.bIncludeAllProjectTextures ? TEXT("true") : TEXT("false"));

    const UStageZeroProjectSettings* ManifestSettings = GetDefault<UStageZeroProjectSettings>();
    const bool bIncludeLevelInstancesForWorld = !ManifestSettings || ManifestSettings->bWarmUpLevelInstances;
    const bool bIncludePackedLevelActorsForWorld = !ManifestSettings || ManifestSettings->bWarmUpPackedLevelActors;

    int32 MapIndex = 0;
    for (const FAssetData& WorldAsset : TargetWorldAssets)
    {
        ++MapIndex;
        ++Result.NumProcessedMaps;
        Result.ProcessedMapPackages.Add(WorldAsset.PackageName.ToString());
        UE_LOG(LogStageZeroWarmupManifest, Display, TEXT("StageZero: Processing map %d/%d: %s"),
            MapIndex,
            TargetWorldAssets.Num(),
            *WorldAsset.PackageName.ToString());

        FStageZeroMapMatTexEntry Entry;
        Entry.MapPackageName = WorldAsset.PackageName;

        UWorld* LoadedWorld = SZ_ManifestLoadWorldByPackageName(WorldAsset.PackageName);

#if WITH_EDITOR
        FStageZeroManifestWorldCollectionResult BaseCollection = SZ_ManifestCollectAssetsFromWorld(
            WorldAsset.PackageName,
            LoadedWorld,
            Options.bIncludeTextures,
            bIncludeLevelInstancesForWorld,
            bIncludePackedLevelActorsForWorld,
            /*bForcePackageLoad*/ false,
            /*bMarkRecoveryAttempt*/ false);

        const bool bInitialCollectionIncomplete = BaseCollection.GetLoadFailureCount() > 0;
        if (bInitialCollectionIncomplete || !BaseCollection.HasAnyAssets())
        {
            UE_LOG(
                LogStageZeroWarmupManifest,
                Warning,
                TEXT("StageZero: Base manifest collection for %s was %s on the initial pass (LoadFailures=%d Assets=%d). Retrying with explicit package reload."),
                *WorldAsset.PackageName.ToString(),
                bInitialCollectionIncomplete ? TEXT("incomplete") : TEXT("empty"),
                BaseCollection.GetLoadFailureCount(),
                BaseCollection.GetAssetCount());

            FStageZeroManifestWorldCollectionResult RecoveryCollection = SZ_ManifestCollectAssetsFromWorld(
                WorldAsset.PackageName,
                nullptr,
                Options.bIncludeTextures,
                bIncludeLevelInstancesForWorld,
                bIncludePackedLevelActorsForWorld,
                /*bForcePackageLoad*/ true,
                /*bMarkRecoveryAttempt*/ true);

            const int32 BaseFailureCount = BaseCollection.GetLoadFailureCount();
            const int32 RecoveryFailureCount = RecoveryCollection.GetLoadFailureCount();
            const bool bRecoveryIsMoreComplete = RecoveryFailureCount < BaseFailureCount;
            const bool bRecoveryIsEquivalentButRicher = RecoveryFailureCount == BaseFailureCount
                && RecoveryCollection.GetAssetCount() > BaseCollection.GetAssetCount();
            const bool bRecoveryLoadedMissingBaseWorld = RecoveryFailureCount == BaseFailureCount
                && RecoveryCollection.LoadedWorld
                && !BaseCollection.LoadedWorld;
            if (bRecoveryIsMoreComplete || bRecoveryIsEquivalentButRicher || bRecoveryLoadedMissingBaseWorld)
            {
                BaseCollection = MoveTemp(RecoveryCollection);
            }
        }

        LoadedWorld = BaseCollection.LoadedWorld;
        const bool bBaseCollectionHadAssets = BaseCollection.HasAnyAssets();
        const bool bWorldCollectionIncomplete = !BaseCollection.Stats.bWorldLoaded
            || BaseCollection.Stats.StreamingLevelLoadFailures > 0
            || BaseCollection.Stats.LevelInstanceLoadFailures > 0;
        if (bWorldCollectionIncomplete)
        {
            ++Result.NumWorldLoadFailures;
            UE_LOG(LogStageZeroWarmupManifest, Error,
                TEXT("StageZero: Incomplete world traversal for %s (WorldLoaded=%s StreamingFailures=%d LevelInstanceFailures=%d). The manifest will not be committed."),
                *WorldAsset.PackageName.ToString(),
                BaseCollection.Stats.bWorldLoaded ? TEXT("true") : TEXT("false"),
                BaseCollection.Stats.StreamingLevelLoadFailures,
                BaseCollection.Stats.LevelInstanceLoadFailures);
        }
        Entry.Materials = MoveTemp(BaseCollection.Materials);
        if (Options.bIncludeTextures)
        {
            Entry.Textures = MoveTemp(BaseCollection.Textures);
        }
        Entry.NiagaraSystems = MoveTemp(BaseCollection.NiagaraSystems);
        Entry.StaticMeshes = MoveTemp(BaseCollection.StaticMeshes);
        Entry.SkeletalMeshes = MoveTemp(BaseCollection.SkeletalMeshes);
        AppendTextureDependenciesForMaterials(Entry.Materials, Entry.Textures, WorldAsset.PackageName);

        UE_LOG(
            LogStageZeroWarmupManifest,
            Display,
            TEXT("StageZero: Loaded-world manifest collection for %s produced Materials=%d Textures=%d Niagara=%d StaticMeshes=%d SkeletalMeshes=%d"),
            *WorldAsset.PackageName.ToString(),
            Entry.Materials.Num(),
            Entry.Textures.Num(),
            Entry.NiagaraSystems.Num(),
            Entry.StaticMeshes.Num(),
            Entry.SkeletalMeshes.Num());

        if (!bBaseCollectionHadAssets)
        {
            UE_LOG(
                LogStageZeroWarmupManifest,
                Warning,
                TEXT("StageZero: Zero-asset map diagnostic for %s | WorldLoaded=%s PersistentActors=%d StreamingDiscovered=%d StreamingLoaded=%d StreamingFailures=%d LevelInstances=%d LevelInstanceFailures=%d Components=%d Recovery=%s"),
                *WorldAsset.PackageName.ToString(),
                BaseCollection.Stats.bWorldLoaded ? TEXT("true") : TEXT("false"),
                BaseCollection.Stats.PersistentLevelActorCount,
                BaseCollection.Stats.StreamingLevelsDiscovered,
                BaseCollection.Stats.StreamingLevelsLoaded,
                BaseCollection.Stats.StreamingLevelLoadFailures,
                BaseCollection.Stats.LevelInstancesTraversed,
                BaseCollection.Stats.LevelInstanceLoadFailures,
                BaseCollection.Stats.ComponentsInspected,
                BaseCollection.Stats.bRecoveryAttempted ? TEXT("true") : TEXT("false"));

            if (CVarStageZeroManifestAllowBroadDependencyFallback.GetValueOnAnyThread() != 0)
            {
                TSet<FName> PackagesForMap;
                TSet<FName> RootPackages;
                RootPackages.Add(WorldAsset.PackageName);
                if (!ExpandDependencyClosure(RootPackages, PackagesForMap))
                {
                    Result.FailureReason = DependencyTraversalFailureReason;
                    return Result;
                }
                AppendAssetsFromPackages(PackagesForMap, Entry);
                UE_LOG(
                    LogStageZeroWarmupManifest,
                    Warning,
                    TEXT("StageZero: Loaded-world collection for %s found no assets. Broad dependency fallback is enabled for compatibility and may over-collect."),
                    *WorldAsset.PackageName.ToString());
            }
            else
            {
                UE_LOG(
                    LogStageZeroWarmupManifest,
                    Warning,
                    TEXT("StageZero: Loaded-world collection for %s found no assets. Broad dependency fallback is disabled to avoid unrelated asset inflation."),
                    *WorldAsset.PackageName.ToString());
            }
        }

        if (Options.bIncludeWorldPartition)
        {
            UE_LOG(LogStageZeroWarmupManifest, Display, TEXT("StageZero WP: scanning world %s"), *WorldAsset.PackageName.ToString());
            if (!LoadedWorld)
            {
                UE_LOG(LogStageZeroWarmupManifest, Warning, TEXT("StageZero WP: failed to load world %s"), *WorldAsset.PackageName.ToString());
            }
            else if (UWorldPartition* WorldPartition = LoadedWorld->GetWorldPartition())
            {
                int32 DebugActorDescCount = 0;
#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4)
                FWorldPartitionHelpers::ForEachActorDescInstance(WorldPartition, [&DebugActorDescCount](const FWorldPartitionActorDescInstance* ActorDescInstance)
                {
                    if (ActorDescInstance)
                    {
                        ++DebugActorDescCount;
                    }
                    return true;
                });
#else
                FWorldPartitionHelpers::ForEachActorDesc(WorldPartition, [&DebugActorDescCount](const FWorldPartitionActorDesc* ActorDesc)
                {
                    ++DebugActorDescCount;
                    return true;
                });
#endif

                UE_LOG(LogStageZeroWarmupManifest, Display, TEXT("StageZero WP: world %s has %d actor descriptors"), *WorldAsset.PackageName.ToString(), DebugActorDescCount);
                UE_LOG(LogStageZeroWarmupManifest, Log, TEXT("[StageZero WP Manifest] Map '%s' has WorldPartition class '%s'."), *LoadedWorld->GetName(), *WorldPartition->GetClass()->GetName());

                const int32 CellsBefore = Manifest->WPCells.Num();
                const int32 AnchorsBefore = Manifest->WPAnchorGroups.Num();
                const int32 SplinesBefore = Manifest->WPSplineGroups.Num();

                auto BoxIntersectsSphere = [](const FBox& Box, const FSphere& Sphere)
                {
                    const double DistSq = Box.ComputeSquaredDistanceToPoint(Sphere.Center);
                    return DistSq <= FMath::Square(static_cast<double>(Sphere.W));
                };

                const auto IsFiniteVector = [](const FVector& Value)
                {
                    return FMath::IsFinite(Value.X)
                        && FMath::IsFinite(Value.Y)
                        && FMath::IsFinite(Value.Z);
                };

                struct FStageZeroWPCellBuildData
                {
                    FStageZeroWPCellEntry CellEntry;
                    TSet<FName> ActorPackages;
                };

                TMap<FString, FStageZeroWPCellBuildData> CellBuildData;
                TSet<FName> MapActorPackages;
                TSet<FName> AllActorPackages;
                TSet<FName> NonSpatialActorPackages;

                int32 NumActorDescs = 0;
                int32 NumActorDescsUsed = 0;
                int32 NumActorDescsSkippedRuntime = 0;
                int32 NumActorDescsSkippedEditorOnly = 0;
                int32 NumActorDescsSkippedInvalidBounds = 0;
                int32 NumActorDescsSkippedInvalidPackage = 0;
                int32 NumActorDescsNonSpatialIncluded = 0;

                auto ProcessActorDesc = [&](const FName& GridName, const FName& ActorPackageName, const FBox& ActorBounds, bool bIsEditorOnly, bool bIsSpatiallyLoaded, bool bIsRuntimeRelevant)
                {
                    (void)GridName;
                    ++NumActorDescs;
                    if (ActorPackageName == NAME_None)
                    {
                        ++NumActorDescsSkippedInvalidPackage;
                        return true;
                    }

                    // Keep editor-only descriptor packages available to the separate
                    // authoring-marker pass. They never contribute runtime asset paths.
                    AllActorPackages.Add(ActorPackageName);

                    if (!bIsRuntimeRelevant)
                    {
                        ++NumActorDescsSkippedRuntime;
                        return true;
                    }

                    if (bIsEditorOnly)
                    {
                        ++NumActorDescsSkippedEditorOnly;
                        return true;
                    }

                    MapActorPackages.Add(ActorPackageName);

                    if (!ActorBounds.IsValid || !IsFiniteVector(ActorBounds.Min) || !IsFiniteVector(ActorBounds.Max))
                    {
                        ++NumActorDescsSkippedInvalidBounds;
                        if (!bIsSpatiallyLoaded)
                        {
                            NonSpatialActorPackages.Add(ActorPackageName);
                        }
                        return true;
                    }

                    if (!bIsSpatiallyLoaded)
                    {
                        ++NumActorDescsNonSpatialIncluded;
                        NonSpatialActorPackages.Add(ActorPackageName);
                        ++NumActorDescsUsed;
                        return true;
                    }

                    // Preserve the existing serialized WPCells/runtime API, but store
                    // deterministic actor-coverage records instead of pretending these
                    // are Epic-generated streaming cells. Actor packages are stable and
                    // actor bounds give anchors/routes exact authoring coverage.
                    const FString CellId = ActorPackageName.ToString();

                    FStageZeroWPCellBuildData& BuildData = CellBuildData.FindOrAdd(CellId);
                    if (BuildData.CellEntry.CellId.IsEmpty())
                    {
                        BuildData.CellEntry.CellId = CellId;
                        BuildData.CellEntry.MapPackageName = WorldAsset.PackageName;
                    }

                    BuildData.CellEntry.Bounds += ActorBounds;
                    BuildData.ActorPackages.Add(ActorPackageName);
                    ++NumActorDescsUsed;
                    return true;
                };

#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4)
                bool bSawActorDescInstances = false;
                FWorldPartitionHelpers::ForEachActorDescInstance(WorldPartition, [&](const FWorldPartitionActorDescInstance* ActorDescInstance)
                {
                    if (!ActorDescInstance)
                    {
                        return true;
                    }

                    bSawActorDescInstances = true;

                    FBox ActorBounds = ActorDescInstance->GetRuntimeBounds();
                    if (!ActorBounds.IsValid)
                    {
                        ActorBounds = ActorDescInstance->GetEditorBounds();
                    }

                    return ProcessActorDesc(
                        ActorDescInstance->GetRuntimeGrid(),
                        ActorDescInstance->GetActorPackage(),
                        ActorBounds,
                        ActorDescInstance->GetActorIsEditorOnly(),
                        ActorDescInstance->GetIsSpatiallyLoaded(),
                        ActorDescInstance->IsRuntimeRelevant());
                });

                if (!bSawActorDescInstances)
                {
                    UE_LOG(LogStageZeroWarmupManifest, Warning, TEXT("[StageZero WP Manifest] Map '%s' reported zero actor desc instances; WP actor data will be limited."), *LoadedWorld->GetName());
                }
#else
                FWorldPartitionHelpers::ForEachActorDesc(WorldPartition, [&](const FWorldPartitionActorDesc* ActorDesc)
                {
                    if (!ActorDesc)
                    {
                        return true;
                    }

#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2)
                    const FBox ActorBounds = ActorDesc->GetEditorBounds();
#else
                    const FBox ActorBounds = ActorDesc->GetBounds();
#endif
                    return ProcessActorDesc(
                        ActorDesc->GetRuntimeGrid(),
                        ActorDesc->GetActorPackage(),
                        ActorBounds,
                        ActorDesc->GetActorIsEditorOnly(),
                        ActorDesc->GetIsSpatiallyLoaded(),
                        true);
                });
#endif

                UE_LOG(
                    LogStageZeroWarmupManifest,
                    Display,
                    TEXT("[StageZero WP Manifest] Map '%s': ActorDesc filter Total=%d Used=%d SkippedRuntime=%d SkippedEditorOnly=%d SkippedInvalidBounds=%d SkippedInvalidPackage=%d NonSpatialIncluded=%d PendingCells=%d"),
                    *LoadedWorld->GetName(),
                    NumActorDescs,
                    NumActorDescsUsed,
                    NumActorDescsSkippedRuntime,
                    NumActorDescsSkippedEditorOnly,
                    NumActorDescsSkippedInvalidBounds,
                    NumActorDescsSkippedInvalidPackage,
                    NumActorDescsNonSpatialIncluded,
                    CellBuildData.Num());

                FStageZeroPackageAssets AlwaysLoadedAssets;
                TArray<FName> SortedNonSpatialPackages = NonSpatialActorPackages.Array();
                SortedNonSpatialPackages.Sort(FNameLexicalLess());
                for (const FName& ActorPackage : SortedNonSpatialPackages)
                {
                    AppendTypedPackageAssets(AlwaysLoadedAssets, CollectActorDependencyAssets(ActorPackage));
                }
                FinalizeTypedPackageAssets(AlwaysLoadedAssets);

                auto AppendAssetsToCell = [&Options](FStageZeroWPCellEntry& Cell, const FStageZeroPackageAssets& Assets)
                {
                    Cell.Materials.Append(Assets.Materials);
                    if (Options.bIncludeTextures)
                    {
                        Cell.Textures.Append(Assets.Textures);
                    }
                    Cell.NiagaraSystems.Append(Assets.NiagaraSystems);
                    Cell.StaticMeshes.Append(Assets.StaticMeshes);
                    Cell.SkeletalMeshes.Append(Assets.SkeletalMeshes);
                };

                for (TPair<FString, FStageZeroWPCellBuildData>& CellPair : CellBuildData)
                {
                    FStageZeroWPCellBuildData& CellData = CellPair.Value;
                    AppendAssetsToCell(CellData.CellEntry, AlwaysLoadedAssets);

                    TArray<FName> SortedActorPackages = CellData.ActorPackages.Array();
                    SortedActorPackages.Sort(FNameLexicalLess());
                    for (const FName& ActorPackage : SortedActorPackages)
                    {
                        AppendAssetsToCell(CellData.CellEntry, CollectActorDependencyAssets(ActorPackage));
                    }

                    DedupAndUnion(
                        CellData.CellEntry.Materials,
                        CellData.CellEntry.Textures,
                        CellData.CellEntry.NiagaraSystems,
                        CellData.CellEntry.StaticMeshes,
                        CellData.CellEntry.SkeletalMeshes,
                        nullptr,
                        WorldAsset.PackageName);

                    const bool bCellHasAssets = CellData.CellEntry.Materials.Num() > 0
                        || CellData.CellEntry.Textures.Num() > 0
                        || CellData.CellEntry.NiagaraSystems.Num() > 0
                        || CellData.CellEntry.StaticMeshes.Num() > 0
                        || CellData.CellEntry.SkeletalMeshes.Num() > 0;
                    if (!bCellHasAssets)
                    {
                        UE_LOG(LogStageZeroWarmupManifest, Verbose,
                            TEXT("StageZero WP: skipped empty actor coverage record %s for %s."),
                            *CellData.CellEntry.CellId,
                            *WorldAsset.PackageName.ToString());
                        continue;
                    }

                    Manifest->WPCells.Add(MoveTemp(CellData.CellEntry));
                }

                if (NumActorDescs == 0)
                {
                    UE_LOG(LogStageZeroWarmupManifest, Warning,
                        TEXT("[StageZero WP Manifest] Map '%s' reported zero actor descriptors. No approximate cells were emitted; runtime map fallback remains authoritative."),
                        *LoadedWorld->GetName());
                }

                // Cache this map's deterministic actor-coverage records for anchor/spline aggregation.
                TArray<const FStageZeroWPCellEntry*> MapCells;
                for (const FStageZeroWPCellEntry& Cell : Manifest->WPCells)
                {
                    if (Cell.MapPackageName == WorldAsset.PackageName)
                    {
                        MapCells.Add(&Cell);
                    }
                }

                if (Options.bIncludeAnchorAndSplineRoutes)
                {
                    TSet<FName> AuthoringAssetPackageSet = AllActorPackages;
                    AuthoringAssetPackageSet.Add(WorldAsset.PackageName);
                    TArray<FName> AuthoringAssetPackages = AuthoringAssetPackageSet.Array();
                    AuthoringAssetPackages.Sort(FNameLexicalLess());

                    // Build anchors by intersecting their radius with WP cells to pull referenced assets.
                    for (const FName& AnchorPackage : AuthoringAssetPackages)
                    {
                        for (const FAssetData& AnchorAsset : CollectEditorMarkerAssets(AnchorPackage).AnchorAssets)
                        {
                            if (const AStageZeroWPAnchor* AnchorActor = Cast<AStageZeroWPAnchor>(AnchorAsset.GetAsset()))
                            {
                                const FVector AnchorLocation = AnchorActor->GetActorLocation();
                                const float AnchorRadius = AnchorActor->Radius;
                                if (!IsFiniteVector(AnchorLocation) || !FMath::IsFinite(AnchorRadius) || AnchorRadius < 0.0f)
                                {
                                    UE_LOG(LogStageZeroWarmupManifest, Warning,
                                        TEXT("StageZero WP Anchor: skipped '%s' because its location or radius was invalid."),
                                        *AnchorAsset.ToSoftObjectPath().ToString());
                                    continue;
                                }

                                FStageZeroWPAnchorGroup AnchorGroup;
                                AnchorGroup.MapPackageName = WorldAsset.PackageName;
                                AnchorGroup.GroupName = AnchorActor->GroupName;
                                AnchorGroup.AnchorId = AnchorAsset.ToSoftObjectPath().ToString();
                                AnchorGroup.Priority = AnchorActor->Priority;
                                AnchorGroup.Location = AnchorLocation;
                                AnchorGroup.Radius = AnchorRadius;

                                TSet<FSoftObjectPath> AnchorMaterials;
                                TSet<FSoftObjectPath> AnchorTextures;
                                TSet<FSoftObjectPath> AnchorNiagara;
                                TSet<FSoftObjectPath> AnchorStaticMeshes;
                                TSet<FSoftObjectPath> AnchorSkeletalMeshes;
                                AnchorMaterials.Append(AlwaysLoadedAssets.Materials);
                                if (Options.bIncludeTextures)
                                {
                                    AnchorTextures.Append(AlwaysLoadedAssets.Textures);
                                }
                                AnchorNiagara.Append(AlwaysLoadedAssets.NiagaraSystems);
                                AnchorStaticMeshes.Append(AlwaysLoadedAssets.StaticMeshes);
                                AnchorSkeletalMeshes.Append(AlwaysLoadedAssets.SkeletalMeshes);
                                int32 AnchorTouchedCells = 0;
                                const FSphere AnchorSphere(AnchorGroup.Location, AnchorGroup.Radius);

                                // Collect assets from any cells intersecting the anchor's bubble.
                                for (const FStageZeroWPCellEntry* CellPtr : MapCells)
                                {
                                    if (!CellPtr)
                                    {
                                        continue;
                                    }

                                    if (!CellPtr->Bounds.IsValid || !BoxIntersectsSphere(CellPtr->Bounds, AnchorSphere))
                                    {
                                        continue;
                                    }

                                    ++AnchorTouchedCells;
                                    AnchorMaterials.Append(CellPtr->Materials);
                                    if (Options.bIncludeTextures)
                                    {
                                        AnchorTextures.Append(CellPtr->Textures);
                                    }
                                    AnchorNiagara.Append(CellPtr->NiagaraSystems);
                                    AnchorStaticMeshes.Append(CellPtr->StaticMeshes);
                                    AnchorSkeletalMeshes.Append(CellPtr->SkeletalMeshes);
                                }

                                AnchorGroup.Materials = AnchorMaterials.Array();
                                AnchorGroup.Textures = Options.bIncludeTextures ? AnchorTextures.Array() : TArray<FSoftObjectPath>();
                                AnchorGroup.NiagaraSystems = AnchorNiagara.Array();
                                AnchorGroup.StaticMeshes = AnchorStaticMeshes.Array();
                                AnchorGroup.SkeletalMeshes = AnchorSkeletalMeshes.Array();
                                DedupAndUnion(
                                    AnchorGroup.Materials,
                                    AnchorGroup.Textures,
                                    AnchorGroup.NiagaraSystems,
                                    AnchorGroup.StaticMeshes,
                                    AnchorGroup.SkeletalMeshes,
                                    nullptr,
                                    WorldAsset.PackageName);

                                const bool bAnchorHasAssets = AnchorGroup.Materials.Num() > 0
                                    || AnchorGroup.Textures.Num() > 0
                                    || AnchorGroup.NiagaraSystems.Num() > 0
                                    || AnchorGroup.StaticMeshes.Num() > 0
                                    || AnchorGroup.SkeletalMeshes.Num() > 0;
                                if (!bAnchorHasAssets)
                                {
                                    UE_LOG(LogStageZeroWarmupManifest, Verbose,
                                        TEXT("StageZero WP Anchor: skipped empty group '%s' (%s)."),
                                        *AnchorGroup.GroupName.ToString(),
                                        *AnchorGroup.AnchorId);
                                    continue;
                                }

                                UE_LOG(LogStageZeroWarmupManifest, Display, TEXT("StageZero WP Anchor: Group '%s' touched %d cells, assets M/T/N/SM/SK = %d/%d/%d/%d/%d"),
                                    *AnchorGroup.GroupName.ToString(),
                                    AnchorTouchedCells,
                                    AnchorGroup.Materials.Num(),
                                    AnchorGroup.Textures.Num(),
                                    AnchorGroup.NiagaraSystems.Num(),
                                    AnchorGroup.StaticMeshes.Num(),
                                    AnchorGroup.SkeletalMeshes.Num());
                                Manifest->WPAnchorGroups.Add(MoveTemp(AnchorGroup));
                            }
                        }
                    }

                    constexpr int32 MaxSplineSamples = 2048;
                    for (const FName& SplinePackage : AuthoringAssetPackages)
                    {
                        for (const FAssetData& SplineAsset : CollectEditorMarkerAssets(SplinePackage).SplineAssets)
                        {
                            if (const AStageZeroWPSplineWarmup* SplineActor = Cast<AStageZeroWPSplineWarmup>(SplineAsset.GetAsset()))
                            {
                                const USplineComponent* SplineComp = SplineActor->FindComponentByClass<USplineComponent>();
                                const float SplineLength = SplineComp ? SplineComp->GetSplineLength() : -1.0f;
                                const float SampleDistance = SplineActor->SampleDistance;
                                const float SampleRadius = SplineActor->SampleRadius;
                                if (!SplineComp
                                    || !FMath::IsFinite(SplineLength)
                                    || SplineLength < 0.0f
                                    || !FMath::IsFinite(SampleDistance)
                                    || SampleDistance < 1.0f
                                    || !FMath::IsFinite(SampleRadius)
                                    || SampleRadius < 1.0f)
                                {
                                    UE_LOG(LogStageZeroWarmupManifest, Warning,
                                        TEXT("StageZero WP Spline: skipped '%s' because its spline or sampling settings were invalid."),
                                        *SplineAsset.ToSoftObjectPath().ToString());
                                    continue;
                                }

                                TArray<FVector> SamplePoints;
                                const double DesiredSegments = SplineLength > 0.0f
                                    ? static_cast<double>(SplineLength) / static_cast<double>(SampleDistance)
                                    : 0.0;
                                const int32 NumSegments = DesiredSegments <= 0.0
                                    ? 0
                                    : (DesiredSegments >= static_cast<double>(MaxSplineSamples - 1)
                                        ? MaxSplineSamples - 1
                                        : FMath::Max(1, FMath::CeilToInt(DesiredSegments)));
                                SamplePoints.Reserve(NumSegments + 1);
                                bool bSamplesValid = true;
                                for (int32 SampleIndex = 0; SampleIndex <= NumSegments; ++SampleIndex)
                                {
                                    const float Distance = NumSegments > 0
                                        ? SplineLength * (static_cast<float>(SampleIndex) / static_cast<float>(NumSegments))
                                        : 0.0f;
                                    const FVector Sample = SplineComp->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
                                    if (!IsFiniteVector(Sample))
                                    {
                                        bSamplesValid = false;
                                        break;
                                    }
                                    SamplePoints.Add(Sample);
                                }
                                if (!bSamplesValid || SamplePoints.Num() == 0)
                                {
                                    UE_LOG(LogStageZeroWarmupManifest, Warning,
                                        TEXT("StageZero WP Spline: skipped '%s' because it produced invalid samples."),
                                        *SplineAsset.ToSoftObjectPath().ToString());
                                    continue;
                                }

                                FBox RouteBounds(ForceInit);
                                for (const FVector& Sample : SamplePoints)
                                {
                                    RouteBounds += Sample;
                                }
                                RouteBounds = RouteBounds.ExpandBy(SampleRadius);

                                TSet<FSoftObjectPath> RouteMaterials;
                                TSet<FSoftObjectPath> RouteTextures;
                                TSet<FSoftObjectPath> RouteNiagara;
                                TSet<FSoftObjectPath> RouteStaticMeshes;
                                TSet<FSoftObjectPath> RouteSkeletalMeshes;
                                RouteMaterials.Append(AlwaysLoadedAssets.Materials);
                                if (Options.bIncludeTextures)
                                {
                                    RouteTextures.Append(AlwaysLoadedAssets.Textures);
                                }
                                RouteNiagara.Append(AlwaysLoadedAssets.NiagaraSystems);
                                RouteStaticMeshes.Append(AlwaysLoadedAssets.StaticMeshes);
                                RouteSkeletalMeshes.Append(AlwaysLoadedAssets.SkeletalMeshes);
                                int32 NumTouchedCells = 0;
                                for (const FStageZeroWPCellEntry* CellPtr : MapCells)
                                {
                                    if (!CellPtr || !CellPtr->Bounds.IsValid || !CellPtr->Bounds.Intersect(RouteBounds))
                                    {
                                        continue;
                                    }

                                    bool bIntersects = false;
                                    for (const FVector& Sample : SamplePoints)
                                    {
                                        const FSphere SampleSphere(Sample, SampleRadius);
                                        if (BoxIntersectsSphere(CellPtr->Bounds, SampleSphere))
                                        {
                                            bIntersects = true;
                                            break;
                                        }
                                    }

                                    if (bIntersects)
                                    {
                                        ++NumTouchedCells;
                                        RouteMaterials.Append(CellPtr->Materials);
                                        if (Options.bIncludeTextures)
                                        {
                                            RouteTextures.Append(CellPtr->Textures);
                                        }
                                        RouteNiagara.Append(CellPtr->NiagaraSystems);
                                        RouteStaticMeshes.Append(CellPtr->StaticMeshes);
                                        RouteSkeletalMeshes.Append(CellPtr->SkeletalMeshes);
                                    }
                                }

                                FStageZeroWPSplineGroup Group;
                                Group.MapPackageName = WorldAsset.PackageName;
                                Group.GroupName = SplineActor->GroupName;
                                Group.RouteId = SplineAsset.ToSoftObjectPath().ToString();
                                // Geometry is authoring-only. Runtime selectors consume the baked
                                // typed paths and never need thousands of serialized sample points.
                                Group.SampleRadius = SampleRadius;
                                Group.Materials = RouteMaterials.Array();
                                Group.Textures = Options.bIncludeTextures ? RouteTextures.Array() : TArray<FSoftObjectPath>();
                                Group.NiagaraSystems = RouteNiagara.Array();
                                Group.StaticMeshes = RouteStaticMeshes.Array();
                                Group.SkeletalMeshes = RouteSkeletalMeshes.Array();
                                DedupAndUnion(
                                    Group.Materials,
                                    Group.Textures,
                                    Group.NiagaraSystems,
                                    Group.StaticMeshes,
                                    Group.SkeletalMeshes,
                                    nullptr,
                                    WorldAsset.PackageName);

                                const bool bRouteHasAssets = Group.Materials.Num() > 0
                                    || Group.Textures.Num() > 0
                                    || Group.NiagaraSystems.Num() > 0
                                    || Group.StaticMeshes.Num() > 0
                                    || Group.SkeletalMeshes.Num() > 0;
                                if (!bRouteHasAssets)
                                {
                                    UE_LOG(LogStageZeroWarmupManifest, Verbose,
                                        TEXT("StageZero WP Spline: skipped empty group '%s' (%s)."),
                                        *Group.GroupName.ToString(),
                                        *Group.RouteId);
                                    continue;
                                }

                                UE_LOG(LogStageZeroWarmupManifest, Display,
                                    TEXT("StageZero WP Spline: Group '%s' Route '%s' touched %d cells, assets M/T/N/SM/SK = %d/%d/%d/%d/%d (Samples=%d)"),
                                    *Group.GroupName.ToString(),
                                    *Group.RouteId,
                                    NumTouchedCells,
                                    Group.Materials.Num(),
                                    Group.Textures.Num(),
                                    Group.NiagaraSystems.Num(),
                                    Group.StaticMeshes.Num(),
                                    Group.SkeletalMeshes.Num(),
                                    SamplePoints.Num());
                                Manifest->WPSplineGroups.Add(MoveTemp(Group));
                            }
                        }
                    }
                }
                const int32 CellsAdded = Manifest->WPCells.Num() - CellsBefore;
                const int32 AnchorsAdded = Manifest->WPAnchorGroups.Num() - AnchorsBefore;
                const int32 SplinesAdded = Manifest->WPSplineGroups.Num() - SplinesBefore;

                UE_LOG(LogStageZeroWarmupManifest, Display, TEXT("StageZero WP: world %s produced %d cells, %d anchor groups, %d spline groups (ActorDescs=%d Used=%d)"),
                    *WorldAsset.PackageName.ToString(),
                    CellsAdded,
                    AnchorsAdded,
                    SplinesAdded,
                    NumActorDescs,
                    NumActorDescsUsed);

                // Ensure WP actor packages contribute to the map-level entry so fallback warmup still has coverage.
                TArray<FName> SortedMapActorPackages = MapActorPackages.Array();
                SortedMapActorPackages.Sort(FNameLexicalLess());
                for (const FName& PackageName : SortedMapActorPackages)
                {
                    const FStageZeroPackageAssets& PackageAssets = CollectActorDependencyAssets(PackageName);
                    Entry.Materials.Append(PackageAssets.Materials);
                    if (Options.bIncludeTextures)
                    {
                        Entry.Textures.Append(PackageAssets.Textures);
                    }
                    Entry.NiagaraSystems.Append(PackageAssets.NiagaraSystems);
                    Entry.StaticMeshes.Append(PackageAssets.StaticMeshes);
                    Entry.SkeletalMeshes.Append(PackageAssets.SkeletalMeshes);
                }
            }
            else
            {
                UE_LOG(LogStageZeroWarmupManifest, Log, TEXT("[StageZero WP Manifest] Map '%s' is not World Partition (no UWorldPartition found)."), *LoadedWorld->GetName());
            }
        }
#endif

        if (bDependencyTraversalFailed)
        {
            Result.FailureReason = DependencyTraversalFailureReason;
            return Result;
        }

        if (Options.bIncludeTextures)
        {
            for (FStageZeroWPCellEntry& Cell : Manifest->WPCells)
            {
                if (Cell.MapPackageName == WorldAsset.PackageName)
                {
                    AppendTextureDependenciesForMaterials(Cell.Materials, Cell.Textures, WorldAsset.PackageName);
                    DedupAndUnion(Cell.Materials, Cell.Textures, Cell.NiagaraSystems, Cell.StaticMeshes, Cell.SkeletalMeshes, nullptr);
                }
            }

            for (FStageZeroWPAnchorGroup& AnchorGroup : Manifest->WPAnchorGroups)
            {
                if (AnchorGroup.MapPackageName == WorldAsset.PackageName)
                {
                    AppendTextureDependenciesForMaterials(AnchorGroup.Materials, AnchorGroup.Textures, WorldAsset.PackageName);
                    DedupAndUnion(AnchorGroup.Materials, AnchorGroup.Textures, AnchorGroup.NiagaraSystems, AnchorGroup.StaticMeshes, AnchorGroup.SkeletalMeshes, nullptr);
                }
            }

            for (FStageZeroWPSplineGroup& SplineGroup : Manifest->WPSplineGroups)
            {
                if (SplineGroup.MapPackageName == WorldAsset.PackageName)
                {
                    AppendTextureDependenciesForMaterials(SplineGroup.Materials, SplineGroup.Textures, WorldAsset.PackageName);
                    DedupAndUnion(SplineGroup.Materials, SplineGroup.Textures, SplineGroup.NiagaraSystems, SplineGroup.StaticMeshes, SplineGroup.SkeletalMeshes, nullptr);
                }
            }

            AppendTextureDependenciesForMaterials(Entry.Materials, Entry.Textures, WorldAsset.PackageName);
        }

        if (bDependencyTraversalFailed)
        {
            Result.FailureReason = DependencyTraversalFailureReason;
            return Result;
        }

        DedupAndUnion(Entry.Materials, Entry.Textures, Entry.NiagaraSystems, Entry.StaticMeshes, Entry.SkeletalMeshes, nullptr, WorldAsset.PackageName);

        ApplyExplicitOverrides(
            WorldAsset.PackageName,
            Entry.Materials,
            Entry.Textures,
            Entry.NiagaraSystems,
            Entry.StaticMeshes,
            Entry.SkeletalMeshes);
        for (FStageZeroWPCellEntry& Cell : Manifest->WPCells)
        {
            if (Cell.MapPackageName == WorldAsset.PackageName)
            {
                ApplyExplicitOverrides(
                    WorldAsset.PackageName,
                    Cell.Materials,
                    Cell.Textures,
                    Cell.NiagaraSystems,
                    Cell.StaticMeshes,
                    Cell.SkeletalMeshes,
                    &Cell.Assets);
            }
        }
        for (FStageZeroWPAnchorGroup& AnchorGroup : Manifest->WPAnchorGroups)
        {
            if (AnchorGroup.MapPackageName == WorldAsset.PackageName)
            {
                ApplyExplicitOverrides(
                    WorldAsset.PackageName,
                    AnchorGroup.Materials,
                    AnchorGroup.Textures,
                    AnchorGroup.NiagaraSystems,
                    AnchorGroup.StaticMeshes,
                    AnchorGroup.SkeletalMeshes,
                    &AnchorGroup.Assets);
            }
        }
        for (FStageZeroWPSplineGroup& SplineGroup : Manifest->WPSplineGroups)
        {
            if (SplineGroup.MapPackageName == WorldAsset.PackageName)
            {
                ApplyExplicitOverrides(
                    WorldAsset.PackageName,
                    SplineGroup.Materials,
                    SplineGroup.Textures,
                    SplineGroup.NiagaraSystems,
                    SplineGroup.StaticMeshes,
                    SplineGroup.SkeletalMeshes,
                    &SplineGroup.Assets);
            }
        }

        if (bDependencyTraversalFailed)
        {
            Result.FailureReason = DependencyTraversalFailureReason;
            return Result;
        }

        const bool bMapHasAssets =
            Entry.Materials.Num() > 0 ||
            Entry.Textures.Num() > 0 ||
            Entry.NiagaraSystems.Num() > 0 ||
            Entry.StaticMeshes.Num() > 0 ||
            Entry.SkeletalMeshes.Num() > 0;

        if (bMapHasAssets)
        {
            ++Result.NumMapsWithAssets;
        }
        else
        {
            ++Result.NumZeroAssetMaps;
        }

        // Progress reporting per map.
        const FStageZeroManifestProgress Progress{
            MapIndex,
            TargetWorldAssets.Num(),
            WorldAsset.PackageName,
            Entry.Materials.Num(),
            Entry.Textures.Num(),
            Entry.NiagaraSystems.Num(),
            Entry.StaticMeshes.Num(),
            Entry.SkeletalMeshes.Num()};
        if (GManifestProgressCallback)
        {
            GManifestProgressCallback(Progress);
        }
        UE_LOG(LogStageZeroWarmupManifest, Display, TEXT("StageZeroBuildManifestTotals: Maps=%d Selected=%d LastMap=%s Materials=%d Textures=%d Niagara=%d StaticMeshes=%d SkeletalMeshes=%d"),
            Progress.MapIndex,
            Progress.MapTotal,
            *Progress.MapPackage.ToString(),
            Progress.Materials,
            Progress.Textures,
            Progress.Niagara,
            Progress.StaticMeshes,
            Progress.SkeletalMeshes);

        Manifest->MapEntries.Add(MoveTemp(Entry));
    }

    if (bDependencyTraversalFailed)
    {
        Result.FailureReason = DependencyTraversalFailureReason.IsEmpty()
            ? TEXT("Manifest dependency traversal exceeded a safety bound")
            : DependencyTraversalFailureReason;
        return Result;
    }

    {
        Manifest->WPCells.Sort([](const FStageZeroWPCellEntry& Left, const FStageZeroWPCellEntry& Right)
        {
            return Left.MapPackageName == Right.MapPackageName
                ? Left.CellId < Right.CellId
                : Left.MapPackageName.LexicalLess(Right.MapPackageName);
        });
        Manifest->WPAnchorGroups.Sort([](const FStageZeroWPAnchorGroup& Left, const FStageZeroWPAnchorGroup& Right)
        {
            return Left.MapPackageName == Right.MapPackageName
                ? Left.AnchorId < Right.AnchorId
                : Left.MapPackageName.LexicalLess(Right.MapPackageName);
        });
        Manifest->WPSplineGroups.Sort([](const FStageZeroWPSplineGroup& Left, const FStageZeroWPSplineGroup& Right)
        {
            return Left.MapPackageName == Right.MapPackageName
                ? Left.RouteId < Right.RouteId
                : Left.MapPackageName.LexicalLess(Right.MapPackageName);
        });

        TSet<FSoftObjectPath> AllMatSet;
        TSet<FSoftObjectPath> AllTexSet;
        TSet<FSoftObjectPath> AllNiagaraSet;
        TSet<FSoftObjectPath> AllStaticMeshSet;
        TSet<FSoftObjectPath> AllSkeletalMeshSet;
        for (const FStageZeroMapMatTexEntry& Entry : Manifest->MapEntries)
        {
            AllMatSet.Append(Entry.Materials);
            AllTexSet.Append(Entry.Textures);
            AllNiagaraSet.Append(Entry.NiagaraSystems);
            AllStaticMeshSet.Append(Entry.StaticMeshes);
            AllSkeletalMeshSet.Append(Entry.SkeletalMeshes);
        }
        for (const FStageZeroWPCellEntry& Entry : Manifest->WPCells)
        {
            AllMatSet.Append(Entry.Materials);
            AllTexSet.Append(Entry.Textures);
            AllNiagaraSet.Append(Entry.NiagaraSystems);
            AllStaticMeshSet.Append(Entry.StaticMeshes);
            AllSkeletalMeshSet.Append(Entry.SkeletalMeshes);
        }
        for (const FStageZeroWPAnchorGroup& Entry : Manifest->WPAnchorGroups)
        {
            AllMatSet.Append(Entry.Materials);
            AllTexSet.Append(Entry.Textures);
            AllNiagaraSet.Append(Entry.NiagaraSystems);
            AllStaticMeshSet.Append(Entry.StaticMeshes);
            AllSkeletalMeshSet.Append(Entry.SkeletalMeshes);
        }
        for (const FStageZeroWPSplineGroup& Entry : Manifest->WPSplineGroups)
        {
            AllMatSet.Append(Entry.Materials);
            AllTexSet.Append(Entry.Textures);
            AllNiagaraSet.Append(Entry.NiagaraSystems);
            AllStaticMeshSet.Append(Entry.StaticMeshes);
            AllSkeletalMeshSet.Append(Entry.SkeletalMeshes);
        }

        if (Options.bIncludeAllProjectMaterials)
        {
            AppendRegistryAssetsByClass(UMaterialInterface::StaticClass(), true, AllMatSet);
        }

        if (Options.bIncludeTextures && Options.bIncludeAllProjectTextures)
        {
            AppendRegistryAssetsByClass(UTexture::StaticClass(), true, AllTexSet);
        }

        Manifest->AllMaterials = AllMatSet.Array();
        Manifest->AllTextures = AllTexSet.Array();
        Manifest->AllNiagaraSystems = AllNiagaraSet.Array();
        Manifest->AllStaticMeshes = AllStaticMeshSet.Array();
        Manifest->AllSkeletalMeshes = AllSkeletalMeshSet.Array();

        // Broad project scans happen after per-map assembly, so repeat the global
        // exclusion pass here to preserve the documented exclusions-win contract.
        GlobalExcludedAssets.RemoveFrom(
            Manifest->AllMaterials,
            Manifest->AllTextures,
            Manifest->AllNiagaraSystems,
            Manifest->AllStaticMeshes,
            Manifest->AllSkeletalMeshes);

        bool bHasAnyExplicitTextureIncludes = GlobalAdditionalAssets.HasTextures();
        if (!bHasAnyExplicitTextureIncludes)
        {
            for (const TPair<FName, FStageZeroExplicitAssetSet>& Pair : PerMapAdditionalAssets)
            {
                if (Pair.Value.HasTextures())
                {
                    bHasAnyExplicitTextureIncludes = true;
                    break;
                }
            }
        }
        DedupAndUnion(
            Manifest->AllMaterials,
            Manifest->AllTextures,
            Manifest->AllNiagaraSystems,
            Manifest->AllStaticMeshes,
            Manifest->AllSkeletalMeshes,
            nullptr,
            NAME_None,
            bHasAnyExplicitTextureIncludes);

        if (!Options.bIncludeTextures && !bHasAnyExplicitTextureIncludes && Manifest->AllTextures.Num() > 0)
        {
            UE_LOG(LogStageZeroWarmupManifest, Warning, TEXT("StageZero: IncludeTextures=false but AllTextures is non-empty (%d). This indicates stale data; clearing."), Manifest->AllTextures.Num());
            Manifest->AllTextures.Reset();
        }

        // Final progress ping
        const FStageZeroManifestProgress Progress{
            Manifest->MapEntries.Num(),
            TargetWorldAssets.Num(),
            Manifest->MapEntries.Num() > 0 ? Manifest->MapEntries.Last().MapPackageName : NAME_None,
            Manifest->AllMaterials.Num(),
            Manifest->AllTextures.Num(),
            Manifest->AllNiagaraSystems.Num(),
            Manifest->AllStaticMeshes.Num(),
            Manifest->AllSkeletalMeshes.Num()};
        if (GManifestProgressCallback)
        {
            GManifestProgressCallback(Progress);
        }
        UE_LOG(LogStageZeroWarmupManifest, Display, TEXT("StageZeroBuildManifestProgress: %d/%d Map=%s Materials=%d Textures=%d Niagara=%d StaticMeshes=%d SkeletalMeshes=%d"),
            Progress.MapIndex,
            Progress.MapTotal,
            *Progress.MapPackage.ToString(),
            Progress.Materials,
            Progress.Textures,
            Progress.Niagara,
            Progress.StaticMeshes,
            Progress.SkeletalMeshes);
    }
    if (Result.NumResolvedMaps == 0)
    {
        Result.FailureReason = TEXT("No target maps were resolved for manifest build");
        return Result;
    }

    if (Result.NumProcessedMaps == 0)
    {
        Result.FailureReason = TEXT("No maps were processed during manifest build");
        return Result;
    }

    if (Result.NumProcessedMaps != Result.NumResolvedMaps)
    {
        Result.FailureReason = FString::Printf(
            TEXT("Only %d of %d resolved maps were processed"),
            Result.NumProcessedMaps,
            Result.NumResolvedMaps);
        return Result;
    }

    if (Result.NumWorldLoadFailures > 0)
    {
        Result.FailureReason = FString::Printf(
            TEXT("Manifest collection could not load %d of %d processed worlds; the existing manifest was preserved"),
            Result.NumWorldLoadFailures,
            Result.NumProcessedMaps);
        return Result;
    }

    const bool bAccountingValid = Result.NumRequestedMaps == Result.NumResolvedMaps + Result.NumMissingRequestedMaps
        && Result.NumProcessedMaps == Result.NumResolvedMaps
        && Result.NumMapsWithAssets + Result.NumZeroAssetMaps == Result.NumProcessedMaps
        && Result.NumMissingRequestedMaps == Result.MissingRequestedMaps.Num()
        && Result.NumMissingRequestedMaps == Result.MissingRequestedMapReasons.Num();
    if (!bAccountingValid)
    {
        Result.FailureReason = TEXT("Manifest build accounting invariants failed; the existing manifest was preserved");
        return Result;
    }

    if (Result.NumMapsWithAssets == 0)
    {
        Result.FailureReason = TEXT("All processed maps produced zero assets");
        return Result;
    }

    Result.bSucceeded = true;
    return Result;
#endif
}
