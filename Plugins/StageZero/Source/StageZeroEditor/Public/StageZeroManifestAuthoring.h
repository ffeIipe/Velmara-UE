// Copyright (c) 2025 71 Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "StageZeroManifestBuildOptions.h"

class UStageZeroWarmupManifest;

/** Progress snapshot emitted synchronously while an editor manifest build is active. */
struct STAGEZEROEDITOR_API FStageZeroManifestAuthoringProgress
{
    int32 MapIndex = 0;
    int32 MapTotal = 0;
    FName MapPackage = NAME_None;
    int32 Materials = 0;
    int32 Textures = 0;
    int32 NiagaraSystems = 0;
    int32 StaticMeshes = 0;
    int32 SkeletalMeshes = 0;
};

/**
 * Public result for a transactional editor manifest build.
 *
 * A failed build does not publish partially generated data. When possible, the
 * previously authored manifest is preserved and bExistingManifestPreserved is set.
 */
struct STAGEZEROEDITOR_API FStageZeroManifestAuthoringResult
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

using FStageZeroManifestAuthoringProgressCallback =
    TFunction<void(const FStageZeroManifestAuthoringProgress&)>;

/**
 * Supported C++ entry point for editor-only Stage Zero manifest authoring.
 *
 * Add StageZeroEditor as an editor-target module dependency before including
 * this header. Runtime modules should load the authored manifest through the
 * Stage Zero runtime API and must not depend on this service.
 */
class STAGEZEROEDITOR_API FStageZeroManifestAuthoring
{
public:
    /** Load the manifest-generation options stored in the editor configuration. */
    static FStageZeroManifestBuildOptions LoadOptions();

    /** Persist manifest-generation options to the editor configuration. */
    static void SaveOptions(const FStageZeroManifestBuildOptions& Options);

    /**
     * Transactionally build and publish the authored manifest.
     *
     * This operation must be called on the game thread. ProgressCallback is
     * optional, invoked synchronously during this call, and never retained after
     * the function returns. The returned UObject is the published manifest on
     * success and nullptr on failure.
     */
    static UStageZeroWarmupManifest* BuildManifest(
        const FStageZeroManifestBuildOptions& Options,
        FStageZeroManifestAuthoringResult* OutResult = nullptr,
        FStageZeroManifestAuthoringProgressCallback ProgressCallback = {});
};
