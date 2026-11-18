// Copyright (c) 2025 71 Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UObject/SoftObjectPath.h"
#include "Engine/AssetManagerTypes.h"
#include "StageZeroWarmupManifest.generated.h"

USTRUCT()
struct FStageZeroMapMatTexEntry
{
    GENERATED_BODY()

    // Package name of the map (UWorld package)
    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup")
    FName MapPackageName;

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup")
    TArray<FSoftObjectPath> Materials;

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup")
    TArray<FSoftObjectPath> Textures;

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup")
    TArray<FSoftObjectPath> NiagaraSystems;

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup")
    TArray<FSoftObjectPath> StaticMeshes;

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup")
    TArray<FSoftObjectPath> SkeletalMeshes;
};

USTRUCT()
struct FStageZeroWPCellEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup|WorldPartition")
    FName MapPackageName = NAME_None;

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup|WorldPartition")
    FString CellId;

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup|WorldPartition")
    FBox Bounds;

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup|WorldPartition")
    TArray<FSoftObjectPath> Assets;

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup|WorldPartition")
    TArray<FSoftObjectPath> Materials;

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup|WorldPartition")
    TArray<FSoftObjectPath> Textures;

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup|WorldPartition")
    TArray<FSoftObjectPath> NiagaraSystems;

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup|WorldPartition")
    TArray<FSoftObjectPath> StaticMeshes;

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup|WorldPartition")
    TArray<FSoftObjectPath> SkeletalMeshes;
};

USTRUCT()
struct FStageZeroWPAnchorGroup
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup|WorldPartition")
    FName MapPackageName = NAME_None;

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup|WorldPartition")
    FName GroupName = NAME_None;

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup|WorldPartition")
    FString AnchorId;

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup|WorldPartition")
    int32 Priority = 0;

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup|WorldPartition")
    FVector Location = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup|WorldPartition")
    float Radius = 0.0f;

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup|WorldPartition")
    TArray<FSoftObjectPath> Assets;

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup|WorldPartition")
    TArray<FSoftObjectPath> Materials;

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup|WorldPartition")
    TArray<FSoftObjectPath> Textures;

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup|WorldPartition")
    TArray<FSoftObjectPath> NiagaraSystems;

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup|WorldPartition")
    TArray<FSoftObjectPath> StaticMeshes;

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup|WorldPartition")
    TArray<FSoftObjectPath> SkeletalMeshes;
};

USTRUCT()
struct FStageZeroWPSplineGroup
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup|WorldPartition")
    FName MapPackageName = NAME_None;

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup|WorldPartition")
    FName GroupName = NAME_None;

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup|WorldPartition")
    FString RouteId;

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup|WorldPartition")
    TArray<FVector> SampledPoints;

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup|WorldPartition")
    float SampleRadius = 0.0f;

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup|WorldPartition")
    TArray<FSoftObjectPath> Assets;

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup|WorldPartition")
    TArray<FSoftObjectPath> Materials;

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup|WorldPartition")
    TArray<FSoftObjectPath> Textures;

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup|WorldPartition")
    TArray<FSoftObjectPath> NiagaraSystems;

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup|WorldPartition")
    TArray<FSoftObjectPath> StaticMeshes;

    UPROPERTY(EditAnywhere, Category = "StageZero|Warmup|WorldPartition")
    TArray<FSoftObjectPath> SkeletalMeshes;
};

/**
 * Manifest describing materials and textures referenced by project maps.
 * Built in editor from Asset Registry metadata (no world loads) and consumed at runtime.
 */
UCLASS(BlueprintType)
class STAGEZERO_API UStageZeroWarmupManifest : public UDataAsset
{
    GENERATED_BODY()

public:
    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        static const FPrimaryAssetType ManifestType(TEXT("StageZeroWarmupManifest"));
        return FPrimaryAssetId(ManifestType, GetFName());
    }

    // All materials used across all maps
    UPROPERTY(VisibleAnywhere, Category = "StageZero|Warmup")
    TArray<FSoftObjectPath> AllMaterials;

    // All textures used across all maps
    UPROPERTY(VisibleAnywhere, Category = "StageZero|Warmup")
    TArray<FSoftObjectPath> AllTextures;

    // All Niagara systems used across all maps
    UPROPERTY(VisibleAnywhere, Category = "StageZero|Warmup")
    TArray<FSoftObjectPath> AllNiagaraSystems;

    // All static meshes used across all maps
    UPROPERTY(VisibleAnywhere, Category = "StageZero|Warmup")
    TArray<FSoftObjectPath> AllStaticMeshes;

    // All skeletal meshes used across all maps
    UPROPERTY(VisibleAnywhere, Category = "StageZero|Warmup")
    TArray<FSoftObjectPath> AllSkeletalMeshes;

    // Per-map material/texture entries
    UPROPERTY(VisibleAnywhere, Category = "StageZero|Warmup")
    TArray<FStageZeroMapMatTexEntry> MapEntries;

    /** Per-cell World Partition asset subsets baked by the editor manifest builder. */
    UPROPERTY(VisibleAnywhere, Category = "StageZero|Warmup|WorldPartition")
    TArray<FStageZeroWPCellEntry> WPCells;

    /** Anchor asset subsets baked by the editor manifest builder. */
    UPROPERTY(VisibleAnywhere, Category = "StageZero|Warmup|WorldPartition")
    TArray<FStageZeroWPAnchorGroup> WPAnchorGroups;

    /** Spline-route asset subsets baked by the editor manifest builder. */
    UPROPERTY(VisibleAnywhere, Category = "StageZero|Warmup|WorldPartition")
    TArray<FStageZeroWPSplineGroup> WPSplineGroups;
};
