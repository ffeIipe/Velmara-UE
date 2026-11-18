// Copyright (c) 2025 71 Studios. All rights reserved.

#include "StageZeroManifestAuthoring.h"

#include "StageZeroManifestBuilder.h"
#include "Misc/ScopeExit.h"

namespace
{
FStageZeroManifestAuthoringProgress ToPublicProgress(const FStageZeroManifestProgress& Progress)
{
    FStageZeroManifestAuthoringProgress PublicProgress;
    PublicProgress.MapIndex = Progress.MapIndex;
    PublicProgress.MapTotal = Progress.MapTotal;
    PublicProgress.MapPackage = Progress.MapPackage;
    PublicProgress.Materials = Progress.Materials;
    PublicProgress.Textures = Progress.Textures;
    PublicProgress.NiagaraSystems = Progress.Niagara;
    PublicProgress.StaticMeshes = Progress.StaticMeshes;
    PublicProgress.SkeletalMeshes = Progress.SkeletalMeshes;
    return PublicProgress;
}

FStageZeroManifestAuthoringResult ToPublicResult(const FStageZeroManifestBuildResult& Result)
{
    FStageZeroManifestAuthoringResult PublicResult;
    PublicResult.bSucceeded = Result.bSucceeded;
    PublicResult.bManifestSaved = Result.bManifestSaved;
    PublicResult.bExistingManifestPreserved = Result.bExistingManifestPreserved;
    PublicResult.bForcedAllZeroSave = Result.bForcedAllZeroSave;
    PublicResult.NumRequestedMaps = Result.NumRequestedMaps;
    PublicResult.NumResolvedMaps = Result.NumResolvedMaps;
    PublicResult.NumMissingRequestedMaps = Result.NumMissingRequestedMaps;
    PublicResult.NumProcessedMaps = Result.NumProcessedMaps;
    PublicResult.NumMapsWithAssets = Result.NumMapsWithAssets;
    PublicResult.NumZeroAssetMaps = Result.NumZeroAssetMaps;
    PublicResult.NumWorldLoadFailures = Result.NumWorldLoadFailures;
    PublicResult.NumExplicitIncludesResolved = Result.NumExplicitIncludesResolved;
    PublicResult.NumExplicitExclusionsResolved = Result.NumExplicitExclusionsResolved;
    PublicResult.NumExplicitOverrideMaps = Result.NumExplicitOverrideMaps;
    PublicResult.NumIgnoredExplicitExclusions = Result.NumIgnoredExplicitExclusions;
    PublicResult.RequestedMapPackages = Result.RequestedMapPackages;
    PublicResult.ResolvedMapPackages = Result.ResolvedMapPackages;
    PublicResult.ProcessedMapPackages = Result.ProcessedMapPackages;
    PublicResult.MissingRequestedMaps = Result.MissingRequestedMaps;
    PublicResult.MissingRequestedMapReasons = Result.MissingRequestedMapReasons;
    PublicResult.IgnoredExplicitExclusions = Result.IgnoredExplicitExclusions;
    PublicResult.FailureReason = Result.FailureReason;
    return PublicResult;
}
}

FStageZeroManifestBuildOptions FStageZeroManifestAuthoring::LoadOptions()
{
    return FStageZeroManifestBuilder::LoadManifestBuildOptions();
}

void FStageZeroManifestAuthoring::SaveOptions(const FStageZeroManifestBuildOptions& Options)
{
    FStageZeroManifestBuilder::SaveManifestBuildOptions(Options);
}

UStageZeroWarmupManifest* FStageZeroManifestAuthoring::BuildManifest(
    const FStageZeroManifestBuildOptions& Options,
    FStageZeroManifestAuthoringResult* OutResult,
    FStageZeroManifestAuthoringProgressCallback ProgressCallback)
{
    if (!IsInGameThread())
    {
        if (OutResult)
        {
            *OutResult = FStageZeroManifestAuthoringResult{};
            OutResult->FailureReason = TEXT("Manifest authoring must run on the game thread.");
        }
        return nullptr;
    }

    // The private builder currently owns a single synchronous callback slot.
    // Clear it on both entry and exit so a public API call cannot inherit or leak
    // callback state into a later manifest build.
    FStageZeroManifestBuilder::ClearProgressCallback();
    ON_SCOPE_EXIT
    {
        FStageZeroManifestBuilder::ClearProgressCallback();
    };

    if (ProgressCallback)
    {
        FStageZeroManifestBuilder::SetProgressCallback(
            [Callback = MoveTemp(ProgressCallback)](const FStageZeroManifestProgress& Progress) mutable
            {
                Callback(ToPublicProgress(Progress));
            });
    }

    FStageZeroManifestBuildResult InternalResult;
    UStageZeroWarmupManifest* Manifest =
        FStageZeroManifestBuilder::BuildManifestWithOptions(Options, &InternalResult);

    if (OutResult)
    {
        *OutResult = ToPublicResult(InternalResult);
    }
    return Manifest;
}
