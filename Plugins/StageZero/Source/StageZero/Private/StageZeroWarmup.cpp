// Copyright (c) 2025 71 Studios. All rights reserved.

#include "StageZeroWarmup.h"

#include "StageZeroAssetRegistryCompat.h"
#include "StageZeroProjectSettings.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/PostProcessComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/Level.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/Scene.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "LevelInstance/LevelInstanceActor.h"
#include "LevelInstance/LevelInstanceSubsystem.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "PackedLevelActor/PackedLevelActor.h"
#include "Runtime/Launch/Resources/Version.h"
#include "UObject/SoftObjectPath.h"

#include <limits>

DEFINE_LOG_CATEGORY_STATIC(LogStageZeroWarmup, Log, All);

namespace
{
void CollectAssetsFromMapPackage(
    const FString& MapPackageName,
    int32 MaxItems,
    TArray<TSoftObjectPtr<UMaterialInterface>>& OutMaterials,
    TArray<TSoftObjectPtr<UTexture>>& OutTextures)
{
    if (MapPackageName.IsEmpty() || MaxItems <= 0)
    {
        return;
    }

    IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    TSet<FSoftObjectPath> SeenAssets;
    TSet<FName> VisitedPackages;
    TArray<FName> PackageQueue;
    int32 PackageCursor = 0;
    PackageQueue.Add(FName(*MapPackageName));

    const auto TryAddAsset = [&](const FAssetData& AssetData)
    {
        if (SeenAssets.Num() >= MaxItems)
        {
            return;
        }

        const FSoftObjectPath AssetPath = AssetData.ToSoftObjectPath();
        if (!AssetPath.IsValid() || SeenAssets.Contains(AssetPath))
        {
            return;
        }

        if (StageZeroAssetRegistryCompat::IsAssetClassOrChild(AssetData, UMaterialInterface::StaticClass())
            || StageZeroAssetRegistryCompat::IsAssetClassOrChild(AssetData, UMaterial::StaticClass())
            || StageZeroAssetRegistryCompat::IsAssetClassOrChild(AssetData, UMaterialInstance::StaticClass()))
        {
            OutMaterials.Add(TSoftObjectPtr<UMaterialInterface>(AssetPath));
            SeenAssets.Add(AssetPath);
        }
        else if (StageZeroAssetRegistryCompat::IsAssetClassOrChild(AssetData, UTexture::StaticClass())
            || StageZeroAssetRegistryCompat::IsAssetClassOrChild(AssetData, UTexture2D::StaticClass()))
        {
            OutTextures.Add(TSoftObjectPtr<UTexture>(AssetPath));
            SeenAssets.Add(AssetPath);
        }
    };

    while (PackageCursor < PackageQueue.Num() && SeenAssets.Num() < MaxItems)
    {
        const FName PackageName = PackageQueue[PackageCursor++];
        if (PackageName.IsNone() || VisitedPackages.Contains(PackageName))
        {
            continue;
        }
        VisitedPackages.Add(PackageName);

        TArray<FAssetData> PackageAssets;
        AssetRegistry.GetAssetsByPackageName(PackageName, PackageAssets);
        for (const FAssetData& Asset : PackageAssets)
        {
            TryAddAsset(Asset);
            if (SeenAssets.Num() >= MaxItems)
            {
                break;
            }
        }

        TArray<FName> Dependencies;
        using namespace UE::AssetRegistry;
        FDependencyQuery DependencyQuery;
        AssetRegistry.GetDependencies(PackageName, Dependencies, EDependencyCategory::Package, DependencyQuery);
        for (const FName& Dependency : Dependencies)
        {
            if (!Dependency.IsNone() && !VisitedPackages.Contains(Dependency))
            {
                PackageQueue.AddUnique(Dependency);
            }
        }
    }

    UE_LOG(LogStageZeroWarmup, Display,
        TEXT("Legacy package discovery completed without world traversal: %d material(s), %d texture(s), %d package(s)."),
        OutMaterials.Num(),
        OutTextures.Num(),
        VisitedPackages.Num());
}

template <typename SoftObjectType>
void DeduplicateSoftReferences(TArray<SoftObjectType>& References)
{
    TSet<FSoftObjectPath> Seen;
    References.RemoveAll([&Seen](const SoftObjectType& Reference)
    {
        const FSoftObjectPath Path = Reference.ToSoftObjectPath();
        if (!Path.IsValid() || Seen.Contains(Path))
        {
            return true;
        }
        Seen.Add(Path);
        return false;
    });
}

// STAGEZERO_LOADED_WORLD_COLLECTOR_BEGIN
class FStageZeroLoadedWorldCollector
{
public:
    FStageZeroLoadedWorldCollector(
        int32 InMaxItems,
        bool bInIncludeLevelInstances,
        bool bInIncludePackedLevelActors,
        TArray<TSoftObjectPtr<UMaterialInterface>>& InMaterials,
        TArray<TSoftObjectPtr<UTexture>>& InTextures)
        : MaxItems(InMaxItems)
        , bIncludeLevelInstances(bInIncludeLevelInstances)
        , bIncludePackedLevelActors(bInIncludePackedLevelActors)
        , Materials(InMaterials)
        , Textures(InTextures)
    {
    }

    void Collect(UWorld* World)
    {
        if (!IsValid(World) || !IsInGameThread())
        {
            return;
        }

        // GetLevels() and the Level Instance lookups below are read-only views
        // of levels which are already resident. This collector must never
        // request a load or inspect a hidden warmup world.
        ULevelInstanceSubsystem* LevelInstanceSubsystem = World->GetSubsystem<ULevelInstanceSubsystem>();
        for (ULevel* Level : World->GetLevels())
        {
            if (!HasCapacity())
            {
                break;
            }
            if (!IsValid(Level))
            {
                continue;
            }

            if (!bIncludeLevelInstances
                && LevelInstanceSubsystem
                && LevelInstanceSubsystem->GetOwningLevelInstance(Level) != nullptr)
            {
                continue;
            }

            CollectLevel(Level, LevelInstanceSubsystem, 0);
        }
    }

private:
    static constexpr int32 MaxLevelInstanceDepth = 32;
    static constexpr int32 MaxVisitedLevels = 256;

    bool HasCapacity() const
    {
        return Materials.Num() + Textures.Num() < MaxItems;
    }

    void AddTexture(UTexture* Texture)
    {
        if (!Texture || !HasCapacity() || SeenTextures.Contains(Texture))
        {
            return;
        }

        SeenTextures.Add(Texture);
        Textures.Add(TSoftObjectPtr<UTexture>(Texture));
    }

    void AddMaterial(UMaterialInterface* Material)
    {
        if (!Material || !HasCapacity() || SeenMaterials.Contains(Material))
        {
            return;
        }

        SeenMaterials.Add(Material);
        Materials.Add(TSoftObjectPtr<UMaterialInterface>(Material));
        if (!HasCapacity())
        {
            return;
        }

        TArray<UTexture*> UsedTextures;
#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7)
        Material->GetUsedTextures(UsedTextures);
#else
        Material->GetUsedTextures(UsedTextures, EMaterialQualityLevel::Num, true, ERHIFeatureLevel::SM5, true);
#endif
        for (UTexture* Texture : UsedTextures)
        {
            AddTexture(Texture);
            if (!HasCapacity())
            {
                break;
            }
        }
    }

    void AddPostProcessSettings(const FPostProcessSettings& Settings)
    {
        for (const FWeightedBlendable& Blendable : Settings.WeightedBlendables.Array)
        {
            if (UMaterialInterface* Material = Cast<UMaterialInterface>(Blendable.Object))
            {
                AddMaterial(Material);
            }
            else
            {
                AddTexture(Cast<UTexture>(Blendable.Object));
            }

            if (!HasCapacity())
            {
                break;
            }
        }
    }

    void CollectActor(AActor* Actor, ULevelInstanceSubsystem* LevelInstanceSubsystem, int32 Depth)
    {
        if (!IsValid(Actor) || !HasCapacity() || VisitedActors.Contains(Actor))
        {
            return;
        }

        VisitedActors.Add(Actor);
        if (!bIncludePackedLevelActors && Actor->IsA<APackedLevelActor>())
        {
            return;
        }

        if (const APostProcessVolume* PostProcessVolume = Cast<APostProcessVolume>(Actor))
        {
            AddPostProcessSettings(PostProcessVolume->Settings);
        }

        TInlineComponentArray<UActorComponent*> Components;
        Actor->GetComponents(Components);
        for (UActorComponent* Component : Components)
        {
            if (!HasCapacity())
            {
                break;
            }
            if (!IsValid(Component))
            {
                continue;
            }

            if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component))
            {
                const int32 MaterialCount = Primitive->GetNumMaterials();
                for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount && HasCapacity(); ++MaterialIndex)
                {
                    AddMaterial(Primitive->GetMaterial(MaterialIndex));
                }
            }

            if (const UPostProcessComponent* PostProcessComponent = Cast<UPostProcessComponent>(Component))
            {
                AddPostProcessSettings(PostProcessComponent->Settings);
            }

        }

        if (bIncludeLevelInstances && Depth < MaxLevelInstanceDepth && LevelInstanceSubsystem)
        {
            if (const ALevelInstance* LevelInstance = Cast<ALevelInstance>(Actor))
            {
                ULevel* LoadedLevel = nullptr;
#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4)
                LoadedLevel = LevelInstance->GetLoadedLevel();
#elif WITH_EDITOR
                // UE 5.3 exposes the already-loaded editor level through the
                // subsystem. This is a lookup only; it never requests loading.
                LoadedLevel = LevelInstanceSubsystem->GetLevelInstanceLevel(LevelInstance);
#endif
                CollectLevel(LoadedLevel, LevelInstanceSubsystem, Depth + 1);
            }
        }
    }

    void CollectLevel(ULevel* Level, ULevelInstanceSubsystem* LevelInstanceSubsystem, int32 Depth)
    {
        if (!IsValid(Level)
            || Depth > MaxLevelInstanceDepth
            || VisitedLevels.Num() >= MaxVisitedLevels
            || VisitedLevels.Contains(Level))
        {
            return;
        }

        VisitedLevels.Add(Level);
        for (AActor* Actor : Level->Actors)
        {
            CollectActor(Actor, LevelInstanceSubsystem, Depth);
            if (!HasCapacity())
            {
                break;
            }
        }
    }

    int32 MaxItems;
    bool bIncludeLevelInstances;
    bool bIncludePackedLevelActors;
    TArray<TSoftObjectPtr<UMaterialInterface>>& Materials;
    TArray<TSoftObjectPtr<UTexture>>& Textures;
    TSet<TWeakObjectPtr<ULevel>> VisitedLevels;
    TSet<TWeakObjectPtr<AActor>> VisitedActors;
    TSet<TWeakObjectPtr<UMaterialInterface>> SeenMaterials;
    TSet<TWeakObjectPtr<UTexture>> SeenTextures;
};
// STAGEZERO_LOADED_WORLD_COLLECTOR_END
}

void UStageZeroWarmup::AutoCollectAssets(
    const FString& MapPackageName,
    FStageZeroWarmupSet& OutSet,
    UWorld* World,
    int32 MaxItemsOverride,
    bool bUseLoadedWorldFallback,
    bool bAllowPackageFallback)
{
    OutSet.Materials.Reset();
    OutSet.Textures.Reset();

    const UStageZeroProjectSettings* Settings = GetDefault<UStageZeroProjectSettings>();
    int32 MaxItems = FMath::Max(1, Settings ? Settings->WarmupMaxItems : 2048);
    if (MaxItemsOverride > 0)
    {
        MaxItems = MaxItemsOverride;
    }
    else if (MaxItemsOverride == 0)
    {
        MaxItems = std::numeric_limits<int32>::max();
    }

    if (bUseLoadedWorldFallback && World)
    {
        if (IsInGameThread())
        {
            FStageZeroLoadedWorldCollector Collector(
                MaxItems,
                !Settings || Settings->bWarmUpLevelInstances,
                !Settings || Settings->bWarmUpPackedLevelActors,
                OutSet.Materials,
                OutSet.Textures);
            Collector.Collect(World);
        }
        else
        {
            UE_LOG(LogStageZeroWarmup, Warning,
                TEXT("Loaded-world discovery was skipped because UObject traversal must run on the game thread."));
        }
    }

    if (OutSet.Materials.IsEmpty() && OutSet.Textures.IsEmpty() && bAllowPackageFallback)
    {
        FString PackageName = MapPackageName;
        if (FPackageName::IsValidObjectPath(PackageName))
        {
            PackageName = FPackageName::ObjectPathToPackageName(PackageName);
        }

        if (FPackageName::IsValidLongPackageName(PackageName))
        {
            CollectAssetsFromMapPackage(PackageName, MaxItems, OutSet.Materials, OutSet.Textures);
        }
        else
        {
            UE_LOG(LogStageZeroWarmup, Warning, TEXT("Legacy package discovery rejected invalid map package: %s"), *MapPackageName);
        }
    }

    DeduplicateSoftReferences(OutSet.Materials);
    DeduplicateSoftReferences(OutSet.Textures);

    if (OutSet.Materials.Num() + OutSet.Textures.Num() > MaxItems)
    {
        OutSet.Materials.SetNum(FMath::Min(OutSet.Materials.Num(), MaxItems));
        const int32 Remaining = FMath::Max(0, MaxItems - OutSet.Materials.Num());
        OutSet.Textures.SetNum(FMath::Min(OutSet.Textures.Num(), Remaining));
    }
}

void UStageZeroWarmup::Init(
    UWorld* World,
    const FStageZeroWarmupSet& Set,
    float MsPerTick,
    bool bAllowMaterialRendering)
{
    (void)World;
    CancelActiveRequest();

    if (bAllowMaterialRendering)
    {
        UE_LOG(LogStageZeroWarmup, Warning,
            TEXT("Runtime material rendering was requested through a legacy API and was ignored. Stage Zero only loads and retains assets."));
    }

    // Preserve the legacy parameter value for binary compatibility and diagnostics only.
    // Tick never performs synchronous or budgeted loading.
    BudgetMs = FMath::Max(0.1f, MsPerTick);

    PendingMaterialRefs = Set.Materials;
    PendingTextureRefs = Set.Textures;
    DeduplicateSoftReferences(PendingMaterialRefs);
    DeduplicateSoftReferences(PendingTextureRefs);
    RetainedMaterials.Reset();
    RetainedTextures.Reset();
    ProcessedMaterials = 0;
    ProcessedTextures = 0;
    FailedMaterials = 0;
    FailedTextures = 0;
    TotalMaterials = PendingMaterialRefs.Num();
    TotalTextures = PendingTextureRefs.Num();
    bLoadTerminal = TotalMaterials + TotalTextures == 0;
    bLoadSuccessful = bLoadTerminal;
    bLoadTimedOut = false;
    LastFailureReason.Reset();
    LoadDeadlineTimeSeconds = 0.0;

    UE_LOG(LogStageZeroWarmup, Display,
        TEXT("Legacy compatibility preload initialized: %d material(s), %d texture(s). Loading is asynchronous; no rendering or resource updates will be performed."),
        TotalMaterials,
        TotalTextures);

    if (bLoadTerminal)
    {
        return;
    }

    TArray<FSoftObjectPath> RequestPaths;
    RequestPaths.Reserve(TotalMaterials + TotalTextures);
    for (const TSoftObjectPtr<UMaterialInterface>& MaterialRef : PendingMaterialRefs)
    {
        RequestPaths.Add(MaterialRef.ToSoftObjectPath());
    }
    for (const TSoftObjectPtr<UTexture>& TextureRef : PendingTextureRefs)
    {
        RequestPaths.Add(TextureRef.ToSoftObjectPath());
    }

    const UStageZeroProjectSettings* Settings = GetDefault<UStageZeroProjectSettings>();
    const double TimeoutSeconds = FMath::Max(1.0, static_cast<double>(Settings ? Settings->AssetLoadTimeoutSeconds : 300.0f));
    const uint64 RequestSerial = ++LoadRequestSerial;
    ActiveLoadRequestSerial = RequestSerial;
    LoadDeadlineTimeSeconds = FPlatformTime::Seconds() + TimeoutSeconds;
    ActiveLoadHandle = StreamableManager.RequestAsyncLoad(
        RequestPaths,
        FStreamableDelegate::CreateUObject(this, &UStageZeroWarmup::HandleAsyncLoadComplete, RequestSerial),
        FStreamableManager::DefaultAsyncLoadPriority,
        false,
        true,
        TEXT("StageZeroLegacyCompatibilityLoad"));

    if (!ActiveLoadHandle.IsValid())
    {
        FinalizeAsyncLoad(RequestSerial, false,
            TEXT("The asynchronous load request could not be created. Synchronous fallback is intentionally disabled."));
        return;
    }

    // Ownership, serial validation, and the wall-clock deadline are established
    // before starting, so an already-loaded request may safely invoke its callback immediately.
    const TSharedPtr<FStreamableHandle> HandleToStart = ActiveLoadHandle;
    HandleToStart->StartStalledHandle();
}

void UStageZeroWarmup::Tick(float DeltaSeconds)
{
    (void)DeltaSeconds;
    if (bLoadTerminal || ActiveLoadRequestSerial == 0)
    {
        return;
    }

    if (LoadDeadlineTimeSeconds > 0.0 && FPlatformTime::Seconds() >= LoadDeadlineTimeSeconds)
    {
        const uint64 TimedOutRequestSerial = ActiveLoadRequestSerial;
        FinalizeAsyncLoad(TimedOutRequestSerial, true,
            TEXT("The asynchronous asset preload exceeded its wall-clock deadline."));
    }
}

bool UStageZeroWarmup::IsDone() const
{
    return bLoadTerminal;
}

float UStageZeroWarmup::GetProgress() const
{
    const int32 TotalItems = TotalMaterials + TotalTextures;
    if (TotalItems <= 0 || bLoadTerminal)
    {
        return 1.0f;
    }

    // This is completion progress, not a success claim. A terminal failed request
    // returns 1.0 and must be distinguished with WasSuccessful/GetFailureCount.
    return FMath::Clamp(
        static_cast<float>(CountCurrentlyResolvedItems()) / static_cast<float>(TotalItems),
        0.0f,
        0.99f);
}

void UStageZeroWarmup::GetStatus(int32& OutProcessed, int32& OutTotal) const
{
    OutProcessed = ProcessedMaterials + ProcessedTextures;
    OutTotal = TotalMaterials + TotalTextures;
}

bool UStageZeroWarmup::WasSuccessful() const
{
    return bLoadTerminal && bLoadSuccessful;
}

int32 UStageZeroWarmup::GetFailureCount() const
{
    return FailedMaterials + FailedTextures;
}

bool UStageZeroWarmup::DidTimeOut() const
{
    return bLoadTimedOut;
}

const FString& UStageZeroWarmup::GetFailureReason() const
{
    return LastFailureReason;
}

void UStageZeroWarmup::HandleAsyncLoadComplete(uint64 RequestSerial)
{
    FinalizeAsyncLoad(RequestSerial, false, FString());
}

void UStageZeroWarmup::FinalizeAsyncLoad(uint64 RequestSerial, bool bTimedOut, const FString& FailureContext)
{
    if (RequestSerial == 0 || RequestSerial != ActiveLoadRequestSerial)
    {
        return;
    }

    // Keep the handle alive while resolving and retaining objects. It may be the
    // final strong owner supplied by the streamable manager.
    TSharedPtr<FStreamableHandle> CompletedHandle = ActiveLoadHandle;
    ActiveLoadRequestSerial = 0;
    LoadDeadlineTimeSeconds = 0.0;
    FailedMaterials = 0;
    FailedTextures = 0;
    RetainedMaterials.Reset();
    RetainedTextures.Reset();

    constexpr int32 MaxDetailedFailureLogs = 8;
    int32 DetailedFailureLogs = 0;
    const auto LogFailedPath = [&DetailedFailureLogs](const TCHAR* ExpectedType, const FSoftObjectPath& Path)
    {
        if (DetailedFailureLogs < MaxDetailedFailureLogs)
        {
            UE_LOG(LogStageZeroWarmup, Warning,
                TEXT("Legacy compatibility preload could not retain %s asset: %s"),
                ExpectedType,
                *Path.ToString());
        }
        ++DetailedFailureLogs;
    };

    for (const TSoftObjectPtr<UMaterialInterface>& MaterialRef : PendingMaterialRefs)
    {
        if (UMaterialInterface* Material = MaterialRef.Get())
        {
            RetainedMaterials.AddUnique(Material);
        }
        else
        {
            ++FailedMaterials;
            LogFailedPath(TEXT("material"), MaterialRef.ToSoftObjectPath());
        }
    }

    for (const TSoftObjectPtr<UTexture>& TextureRef : PendingTextureRefs)
    {
        if (UTexture* Texture = TextureRef.Get())
        {
            RetainedTextures.AddUnique(Texture);
        }
        else
        {
            ++FailedTextures;
            LogFailedPath(TEXT("texture"), TextureRef.ToSoftObjectPath());
        }
    }

    ProcessedMaterials = TotalMaterials;
    ProcessedTextures = TotalTextures;
    bLoadTimedOut = bTimedOut;
    bLoadSuccessful = !bTimedOut && GetFailureCount() == 0 && FailureContext.IsEmpty();
    bLoadTerminal = true;

    if (bLoadSuccessful)
    {
        LastFailureReason.Reset();
        UE_LOG(LogStageZeroWarmup, Display,
            TEXT("Legacy compatibility preload completed: retained %d material(s) and %d texture(s)."),
            RetainedMaterials.Num(),
            RetainedTextures.Num());
    }
    else
    {
        LastFailureReason = FailureContext;
        if (LastFailureReason.IsEmpty())
        {
            LastFailureReason = FString::Printf(
                TEXT("%d of %d requested assets failed to load with the expected type."),
                GetFailureCount(),
                TotalMaterials + TotalTextures);
        }
        else if (GetFailureCount() > 0)
        {
            LastFailureReason += FString::Printf(
                TEXT(" %d of %d requested assets were not retained."),
                GetFailureCount(),
                TotalMaterials + TotalTextures);
        }

        UE_LOG(LogStageZeroWarmup, Error, TEXT("Legacy compatibility preload failed: %s"), *LastFailureReason);
    }

    ActiveLoadHandle.Reset();
    if (bTimedOut && CompletedHandle.IsValid())
    {
        CompletedHandle->CancelHandle();
    }
}

void UStageZeroWarmup::CancelActiveRequest()
{
    ActiveLoadRequestSerial = 0;
    LoadDeadlineTimeSeconds = 0.0;
    if (ActiveLoadHandle.IsValid())
    {
        ActiveLoadHandle->CancelHandle();
        ActiveLoadHandle.Reset();
    }
}

int32 UStageZeroWarmup::CountCurrentlyResolvedItems() const
{
    int32 ResolvedItems = 0;
    for (const TSoftObjectPtr<UMaterialInterface>& MaterialRef : PendingMaterialRefs)
    {
        ResolvedItems += MaterialRef.Get() != nullptr ? 1 : 0;
    }
    for (const TSoftObjectPtr<UTexture>& TextureRef : PendingTextureRefs)
    {
        ResolvedItems += TextureRef.Get() != nullptr ? 1 : 0;
    }
    return ResolvedItems;
}

void UStageZeroWarmup::BeginDestroy()
{
    CancelActiveRequest();
    RetainedMaterials.Reset();
    RetainedTextures.Reset();
    Super::BeginDestroy();
}
