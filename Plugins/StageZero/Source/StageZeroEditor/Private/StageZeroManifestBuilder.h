#pragma once

#include "CoreMinimal.h"
#include "StageZeroManifestBuildOptions.h"

class UStageZeroWarmupManifest;

struct FStageZeroManifestProgress
{
    int32 MapIndex = 0;
    int32 MapTotal = 0;
    FName MapPackage = NAME_None;
    int32 Materials = 0;
    int32 Textures = 0;
    int32 Niagara = 0;
    int32 StaticMeshes = 0;
    int32 SkeletalMeshes = 0;
};

struct FStageZeroManifestBuildResult
{
    bool bSucceeded = false;
    bool bManifestSaved = false;
    bool bExistingManifestPreserved = false;
    bool bForcedAllZeroSave = false;
    int32 NumRequestedMaps = 0;
    int32 NumResolvedMaps = 0;
    int32 NumMissingRequestedMaps = 0;
    int32 NumProcessedMaps = 0;
    int32 NumMapsWithAssets = 0;
    int32 NumZeroAssetMaps = 0;
    int32 NumWorldLoadFailures = 0;
    int32 NumExplicitIncludesResolved = 0;
    int32 NumExplicitExclusionsResolved = 0;
    int32 NumExplicitOverrideMaps = 0;
    int32 NumIgnoredExplicitExclusions = 0;
    TArray<FString> RequestedMapPackages;
    TArray<FString> ResolvedMapPackages;
    TArray<FString> ProcessedMapPackages;
    TArray<FString> MissingRequestedMaps;
    TArray<FString> MissingRequestedMapReasons;
    TArray<FString> IgnoredExplicitExclusions;
    FString FailureReason;
};

#if WITH_DEV_AUTOMATION_TESTS
struct FStageZeroManifestReflectedAssetReferences
{
    TArray<FSoftObjectPath> Materials;
    TArray<FSoftObjectPath> Textures;
    TArray<FSoftObjectPath> NiagaraSystems;
    TArray<FSoftObjectPath> StaticMeshes;
    TArray<FSoftObjectPath> SkeletalMeshes;
};
#endif

/** Editor-only manifest authoring service. Runtime loading remains in FStageZeroWarmupUtils. */
class FStageZeroManifestBuilder
{
public:
    static FStageZeroManifestBuildOptions LoadManifestBuildOptions();
    static void SaveManifestBuildOptions(const FStageZeroManifestBuildOptions& Options);
    static UStageZeroWarmupManifest* BuildManifestWithOptions(const FStageZeroManifestBuildOptions& Options, FStageZeroManifestBuildResult* OutResult = nullptr);
    static FStageZeroManifestBuildResult BuildManifestContents(UStageZeroWarmupManifest* Manifest, const FStageZeroManifestBuildOptions& Options);
    static void SetProgressCallback(TFunction<void(const FStageZeroManifestProgress&)> InCallback);
    static void ClearProgressCallback();
    static const FStageZeroManifestBuildResult& GetLastBuildResult();
#if WITH_DEV_AUTOMATION_TESTS
    static FStageZeroManifestReflectedAssetReferences CollectReflectedAssetReferencesForTesting(
        UObject* Source,
        bool bIncludeTextures = true);
    static bool ValidateManifestContentsForTesting(
        const UStageZeroWarmupManifest* Manifest,
        int32 ExpectedMapCount,
        FString& OutFailureReason);
#endif
};
