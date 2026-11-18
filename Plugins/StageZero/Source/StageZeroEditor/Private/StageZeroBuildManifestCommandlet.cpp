// Copyright (c) 2025 71 Studios. All rights reserved.

#include "StageZeroBuildManifestCommandlet.h"

#include "StageZeroManifestBuilder.h"
#include "StageZeroManifestOverrideSerialization.h"
#include "StageZeroWarmupManifest.h"
#include "StageZeroWarmupUtils.h"
#include "Misc/Parse.h"

UStageZeroBuildManifestCommandlet::UStageZeroBuildManifestCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
}

namespace StageZeroBuildManifest
{
static bool ApplyCommandLineOverrides(const FString& Params, FStageZeroManifestBuildOptions& Options, FString& OutError)
{
    bool bBoolValue = false;
    if (FParse::Bool(*Params, TEXT("StageZeroUseAllMaps="), bBoolValue))
    {
        Options.bUseAllMaps = bBoolValue;
    }
    if (FParse::Bool(*Params, TEXT("StageZeroIncludeWorldPartition="), bBoolValue))
    {
        Options.bIncludeWorldPartition = bBoolValue;
    }
    if (FParse::Bool(*Params, TEXT("StageZeroIncludeAnchorAndSplineRoutes="), bBoolValue))
    {
        Options.bIncludeAnchorAndSplineRoutes = bBoolValue;
    }
    if (FParse::Bool(*Params, TEXT("StageZeroIncludeTextures="), bBoolValue))
    {
        Options.bIncludeTextures = bBoolValue;
    }
    if (FParse::Bool(*Params, TEXT("StageZeroIncludeAllProjectMaterials="), bBoolValue))
    {
        Options.bIncludeAllProjectMaterials = bBoolValue;
    }
    if (FParse::Bool(*Params, TEXT("StageZeroIncludeAllProjectTextures="), bBoolValue))
    {
        Options.bIncludeAllProjectTextures = bBoolValue;
    }
    if (FParse::Bool(*Params, TEXT("StageZeroIncludeEngineContent="), bBoolValue))
    {
        Options.bIncludeEngineContent = bBoolValue;
    }

    FString SelectedMapsArg;
    if (FParse::Value(*Params, TEXT("StageZeroSelectedMaps="), SelectedMapsArg))
    {
        Options.SelectedMaps.Reset();

        TArray<FString> Tokens;
        SelectedMapsArg.ParseIntoArray(Tokens, TEXT(";"), true);
        if (Tokens.Num() == 1 && Tokens[0].Contains(TEXT(",")))
        {
            TArray<FString> CommaTokens;
            Tokens[0].ParseIntoArray(CommaTokens, TEXT(","), true);
            Tokens = CommaTokens;
        }

        for (const FString& Token : Tokens)
        {
            const FString Trimmed = Token.TrimStartAndEnd();
            if (!Trimmed.IsEmpty())
            {
                Options.SelectedMaps.Add(FSoftObjectPath(Trimmed));
            }
        }
    }

    FString SerializedOverrides;
    if (FParse::Value(*Params, TEXT("StageZeroGlobalAdditionalAssets="), SerializedOverrides)
        && !StageZeroManifestOverrideSerialization::DecodePathList(
            SerializedOverrides, Options.GlobalAdditionalAssets, OutError))
    {
        OutError = FString::Printf(TEXT("Invalid -StageZeroGlobalAdditionalAssets value: %s"), *OutError);
        return false;
    }
    if (FParse::Value(*Params, TEXT("StageZeroGlobalExcludedAssets="), SerializedOverrides)
        && !StageZeroManifestOverrideSerialization::DecodePathList(
            SerializedOverrides, Options.GlobalExcludedAssets, OutError))
    {
        OutError = FString::Printf(TEXT("Invalid -StageZeroGlobalExcludedAssets value: %s"), *OutError);
        return false;
    }
    if (FParse::Value(*Params, TEXT("StageZeroPerMapAssetOverrides="), SerializedOverrides)
        && !StageZeroManifestOverrideSerialization::DecodeMapOverrides(
            SerializedOverrides, Options.PerMapAssetOverrides, OutError))
    {
        OutError = FString::Printf(TEXT("Invalid -StageZeroPerMapAssetOverrides value: %s"), *OutError);
        return false;
    }

    StageZeroManifestOverrideSerialization::NormalizeOptions(Options);
    OutError.Reset();
    return true;
}
} // namespace StageZeroBuildManifest

int32 UStageZeroBuildManifestCommandlet::Main(const FString& Params)
{
    FStageZeroManifestBuildOptions Options = FStageZeroManifestBuilder::LoadManifestBuildOptions();
    FString OverrideParseError;
    if (!StageZeroBuildManifest::ApplyCommandLineOverrides(Params, Options, OverrideParseError))
    {
        UE_LOG(LogStageZeroWarmupManifest, Error,
            TEXT("StageZeroBuildManifest: command-line option parsing failed; no manifest was changed. %s"),
            *OverrideParseError);
        return 1;
    }

    Options.bAllowShortNameMapFallback = false;

    UE_LOG(LogStageZeroWarmupManifest, Display,
        TEXT("StageZeroBuildManifest: options UseAllMaps=%s Selected=%d IncludeWP=%s IncludeAnchorsAndSplines=%s IncludeTextures=%s AllMaterials=%s AllTextures=%s IncludeEngine=%s GlobalIncludes=%d GlobalExcludes=%d PerMapOverrides=%d"),
        Options.bUseAllMaps ? TEXT("true") : TEXT("false"),
        Options.SelectedMaps.Num(),
        Options.bIncludeWorldPartition ? TEXT("true") : TEXT("false"),
        Options.bIncludeAnchorAndSplineRoutes ? TEXT("true") : TEXT("false"),
        Options.bIncludeTextures ? TEXT("true") : TEXT("false"),
        Options.bIncludeAllProjectMaterials ? TEXT("true") : TEXT("false"),
        Options.bIncludeAllProjectTextures ? TEXT("true") : TEXT("false"),
        Options.bIncludeEngineContent ? TEXT("true") : TEXT("false"),
        Options.GlobalAdditionalAssets.Num(),
        Options.GlobalExcludedAssets.Num(),
        Options.PerMapAssetOverrides.Num());

    FStageZeroManifestBuildResult BuildResult;
    if (UStageZeroWarmupManifest* Manifest = FStageZeroManifestBuilder::BuildManifestWithOptions(Options, &BuildResult))
    {
        UE_LOG(LogStageZeroWarmupManifest, Display, TEXT("StageZeroBuildManifest: completed. Maps=%d Materials=%d Textures=%d Niagara=%d StaticMeshes=%d SkeletalMeshes=%d WPCells=%d WPAnchors=%d WPSplines=%d Requested=%d Resolved=%d Missing=%d Processed=%d ZeroAssetMaps=%d WorldLoadFailures=%d ExplicitIncludes=%d ExplicitExclusions=%d OverrideMaps=%d IgnoredExclusions=%d Saved=%s"),
            Manifest->MapEntries.Num(),
            Manifest->AllMaterials.Num(),
            Manifest->AllTextures.Num(),
            Manifest->AllNiagaraSystems.Num(),
            Manifest->AllStaticMeshes.Num(),
            Manifest->AllSkeletalMeshes.Num(),
            Manifest->WPCells.Num(),
            Manifest->WPAnchorGroups.Num(),
            Manifest->WPSplineGroups.Num(),
            BuildResult.NumRequestedMaps,
            BuildResult.NumResolvedMaps,
            BuildResult.NumMissingRequestedMaps,
            BuildResult.NumProcessedMaps,
            BuildResult.NumZeroAssetMaps,
            BuildResult.NumWorldLoadFailures,
            BuildResult.NumExplicitIncludesResolved,
            BuildResult.NumExplicitExclusionsResolved,
            BuildResult.NumExplicitOverrideMaps,
            BuildResult.NumIgnoredExplicitExclusions,
            BuildResult.bManifestSaved ? TEXT("true") : TEXT("false"));
        return 0;
    }

    UE_LOG(LogStageZeroWarmupManifest, Error, TEXT("StageZeroBuildManifest: manifest build failed. Requested=%d Resolved=%d Missing=%d Processed=%d ZeroAssetMaps=%d WorldLoadFailures=%d Preserved=%s MissingMaps=%s Reason=%s"),
        BuildResult.NumRequestedMaps,
        BuildResult.NumResolvedMaps,
        BuildResult.NumMissingRequestedMaps,
        BuildResult.NumProcessedMaps,
        BuildResult.NumZeroAssetMaps,
        BuildResult.NumWorldLoadFailures,
        BuildResult.bExistingManifestPreserved ? TEXT("true") : TEXT("false"),
        *FString::Join(BuildResult.MissingRequestedMaps, TEXT(",")),
        BuildResult.FailureReason.IsEmpty() ? TEXT("Unknown") : *BuildResult.FailureReason);
    return 1;
}
