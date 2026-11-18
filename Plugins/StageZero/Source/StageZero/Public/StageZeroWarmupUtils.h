// Copyright (c) 2025 71 Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"

class UStageZeroWarmupManifest;

STAGEZERO_API DECLARE_LOG_CATEGORY_EXTERN(LogStageZeroWarmupManifest, Log, All);

class STAGEZERO_API FStageZeroWarmupUtils
{
public:
    /** Load the authored manifest. Runtime code never rebuilds or mutates project content. */
    static UStageZeroWarmupManifest* LoadAuthoredManifest();

    /** Legacy source-compatible wrapper. bForceRebuild is ignored; editor callers should use StageZeroEditor's manifest builder service. */
    UE_DEPRECATED(5.3, "Use LoadAuthoredManifest at runtime or FStageZeroManifestAuthoring from the StageZeroEditor module when authoring content.")
    static UStageZeroWarmupManifest* GetOrBuildMatTexManifest(bool bForceRebuild = false);

    // Runtime manifests must never contain editor-only assets, even when engine content is included.
    static bool IsRuntimeWarmupPackageAllowed(const FName& PackageName);
    static bool IsRuntimeWarmupAssetPathAllowed(const FSoftObjectPath& Path);
    static bool IsRuntimeWarmupAssetAllowed(const UObject* Object);

    static const TCHAR* GetManifestObjectPath();
    static const TCHAR* GetManifestPackagePath();
};
