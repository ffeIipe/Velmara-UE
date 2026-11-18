#pragma once

#include "CoreMinimal.h"
#include "StageZeroManifestBuildOptions.h"

/** Stable, delimiter-safe serialization used by config and the manifest commandlet. */
namespace StageZeroManifestOverrideSerialization
{
void NormalizePathList(TArray<FSoftObjectPath>& Paths);
void NormalizeOptions(FStageZeroManifestBuildOptions& Options);

bool EncodePathList(const TArray<FSoftObjectPath>& Paths, FString& OutValue, FString& OutError);
bool DecodePathList(const FString& Value, TArray<FSoftObjectPath>& OutPaths, FString& OutError);

bool EncodeMapOverride(const FStageZeroManifestMapAssetOverrides& Override, FString& OutValue, FString& OutError);
bool DecodeMapOverride(const FString& Value, FStageZeroManifestMapAssetOverrides& OutOverride, FString& OutError);

bool EncodeMapOverrides(const TArray<FStageZeroManifestMapAssetOverrides>& Overrides, FString& OutValue, FString& OutError);
bool DecodeMapOverrides(const FString& Value, TArray<FStageZeroManifestMapAssetOverrides>& OutOverrides, FString& OutError);
}
