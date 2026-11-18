// Copyright (c) 2025 71 Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPath.h"
#include "StageZeroManifestBuildOptions.generated.h"

/**
 * Explicit asset coverage applied to one resolved map during manifest authoring.
 *
 * These paths are authoring inputs. Generated manifest arrays remain read-only
 * outputs and are replaced transactionally on every successful build.
 */
USTRUCT()
struct STAGEZERO_API FStageZeroManifestMapAssetOverrides
{
    GENERATED_BODY()

    /** Exact map whose generated coverage should receive these overrides. */
    UPROPERTY(Config, EditAnywhere, Category = "Manual Overrides", meta = (AllowedClasses = "/Script/Engine.World"))
    FSoftObjectPath Map;

    /** Runtime assets that must be included for this map after automatic discovery. */
    UPROPERTY(Config, EditAnywhere, Category = "Manual Overrides", meta = (AllowedClasses = "/Script/Engine.MaterialInterface,/Script/Engine.Texture,/Script/Niagara.NiagaraSystem,/Script/Engine.StaticMesh,/Script/Engine.SkeletalMesh"))
    TArray<FSoftObjectPath> AdditionalAssets;

    /** Runtime assets that must be removed from this map after all includes are applied. */
    UPROPERTY(Config, EditAnywhere, Category = "Manual Overrides", meta = (AllowedClasses = "/Script/Engine.MaterialInterface,/Script/Engine.Texture,/Script/Niagara.NiagaraSystem,/Script/Engine.StaticMesh,/Script/Engine.SkeletalMesh"))
    TArray<FSoftObjectPath> ExcludedAssets;
};

/**
 * Configurable options that control how the Stage Zero manifest is built.
 */
USTRUCT()
struct STAGEZERO_API FStageZeroManifestBuildOptions
{
    GENERATED_BODY()

    /** If true, scan every UWorld in the project. If false, only process SelectedMaps. */
    UPROPERTY(Config, EditAnywhere, Category = "Manifest")
    bool bUseAllMaps = true;

    /** Optional explicit list of maps to scan when bUseAllMaps is false. */
    UPROPERTY(Config, EditAnywhere, Category = "Manifest", meta = (AllowedClasses = "/Script/Engine.World"))
    TArray<FSoftObjectPath> SelectedMaps;

    /** If true, gather runtime-relevant World Partition cells and external-actor assets for the selected maps during editor manifest generation. */
    UPROPERTY(Config, EditAnywhere, Category = "Manifest", meta = (DisplayName = "Include World Partition Data"))
    bool bIncludeWorldPartition = true;

    /** If true, bake Stage Zero anchor and spline route asset groups into the manifest (requires World Partition collection). */
    UPROPERTY(Config, EditAnywhere, Category = "Manifest", meta = (EditCondition = "bIncludeWorldPartition"))
    bool bIncludeAnchorAndSplineRoutes = true;

    /** If true, automatically discover textures. Explicit manual texture includes are still honored when false. */
    UPROPERTY(Config, EditAnywhere, Category = "Manifest")
    bool bIncludeTextures = true;

    /** If true, append every UMaterialInterface in the project (not just referenced assets). */
    UPROPERTY(Config, EditAnywhere, Category = "Manifest")
    bool bIncludeAllProjectMaterials = false;

    /** If true, append every cookable UTexture in the project except render targets (not just referenced assets). */
    UPROPERTY(Config, EditAnywhere, Category = "Manifest")
    bool bIncludeAllProjectTextures = false;

    /** If true, also scan Engine content (/Engine) in addition to project content (/Game) when building the manifest. Defaults to false to avoid pulling in template/sample maps. */
    UPROPERTY(Config, EditAnywhere, Category = "Manifest")
    bool bIncludeEngineContent = false;

    /** Runtime assets to include for every resolved map after automatic discovery. */
    UPROPERTY(Config, EditAnywhere, AdvancedDisplay, Category = "Manual Overrides", meta = (AllowedClasses = "/Script/Engine.MaterialInterface,/Script/Engine.Texture,/Script/Niagara.NiagaraSystem,/Script/Engine.StaticMesh,/Script/Engine.SkeletalMesh"))
    TArray<FSoftObjectPath> GlobalAdditionalAssets;

    /** Runtime assets to remove from every resolved map. Exclusions always win over includes. */
    UPROPERTY(Config, EditAnywhere, AdvancedDisplay, Category = "Manual Overrides", meta = (AllowedClasses = "/Script/Engine.MaterialInterface,/Script/Engine.Texture,/Script/Niagara.NiagaraSystem,/Script/Engine.StaticMesh,/Script/Engine.SkeletalMesh"))
    TArray<FSoftObjectPath> GlobalExcludedAssets;

    /** Optional exact-map include/exclude rules. A referenced map must be part of the current build. */
    UPROPERTY(Config, EditAnywhere, AdvancedDisplay, Category = "Manual Overrides")
    TArray<FStageZeroManifestMapAssetOverrides> PerMapAssetOverrides;

    /** Deprecated serialized field. Selected maps now require exact package identity. */
    UPROPERTY(Config, meta = (DeprecatedProperty, DeprecationMessage = "Short-name map matching was removed because it can select the wrong map. Use exact package paths."))
    bool bAllowShortNameMapFallback = false;
};
