#pragma once

#include "CoreMinimal.h"
#include "StageZeroManifestBuildOptions.h"
#include "StageZeroManifestOverridesEditorObject.generated.h"

/** Transient details-panel model for the advanced manifest override editor. */
UCLASS(Transient, NotBlueprintable)
class UStageZeroManifestOverridesEditorObject : public UObject
{
    GENERATED_BODY()

public:
    /** Assets always included for every map. Automatic discovery remains unchanged. */
    UPROPERTY(EditAnywhere, Category = "Global Overrides", meta = (DisplayName = "Additional Assets", AllowedClasses = "/Script/Engine.MaterialInterface,/Script/Engine.Texture,/Script/Niagara.NiagaraSystem,/Script/Engine.StaticMesh,/Script/Engine.SkeletalMesh", ToolTip = "Add cookable runtime assets that automatic map discovery cannot see. Supported types are materials, textures, Niagara systems, static meshes, and skeletal meshes. Render targets and editor-only assets are rejected."))
    TArray<FSoftObjectPath> GlobalAdditionalAssets;

    /** Assets always excluded. Exclusions deterministically win over every include. */
    UPROPERTY(EditAnywhere, Category = "Global Overrides", meta = (DisplayName = "Excluded Assets", AllowedClasses = "/Script/Engine.MaterialInterface,/Script/Engine.Texture,/Script/Niagara.NiagaraSystem,/Script/Engine.StaticMesh,/Script/Engine.SkeletalMesh", ToolTip = "Remove assets from every generated map and World Partition subset. Exclusions are applied last."))
    TArray<FSoftObjectPath> GlobalExcludedAssets;

    /** Optional exact-map rules for exceptional coverage. */
    UPROPERTY(EditAnywhere, Category = "Per-Map Overrides", meta = (DisplayName = "Map Overrides", ToolTip = "Add or exclude supported runtime assets for a specific map. The exact map must be part of this build."))
    TArray<FStageZeroManifestMapAssetOverrides> PerMapAssetOverrides;
};
