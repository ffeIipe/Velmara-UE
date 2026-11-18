// Copyright (c) 2025 71 Studios. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "Engine/Texture.h"
#include "Materials/MaterialInterface.h"
#include "StageZeroWarmup.generated.h"

USTRUCT(BlueprintType)
struct STAGEZERO_API FStageZeroWarmupSet
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StageZero|Warmup")
    TArray<TSoftObjectPtr<UMaterialInterface>> Materials;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StageZero|Warmup")
    TArray<TSoftObjectPtr<UTexture>> Textures;
};

/**
 * Legacy asynchronous asset loader retained for Stage Zero compatibility flows.
 * It only loads and retains assets. It never renders materials, creates render
 * targets, updates texture resources, streams hidden maps, or claims PSO coverage.
 */
UCLASS(meta = (DeprecatedNode, DeprecationMessage = "Use UStageZeroSubsystem. This compatibility loader only performs bounded asynchronous asset loading and cannot provide PSO coverage."))
class STAGEZERO_API UStageZeroWarmup : public UObject
{
    GENERATED_BODY()

public:
    /**
     * Auto-collect assets from a target map package
     * @param MapPackageName - Package name of the target map
     * @param OutSet - Output collection of materials and textures
     * @param World - Optional already-loaded world to inspect. Stage Zero never loads or streams a world here.
     * @param MaxItemsOverride - If >0, hard cap the number of collected items.
     *                           If 0, collect all available items (no cap).
     *                           If <0, use the project default.
     * @param bUseLoadedWorldFallback - Inspect actors/components in levels already resident in World before package fallback.
     * @param bAllowPackageFallback - Enables legacy Asset Registry package-dependency discovery when loaded-world discovery finds nothing.
     *                                The package fallback itself never loads, streams, or traverses a world.
     */
    static void AutoCollectAssets(const FString& MapPackageName, FStageZeroWarmupSet& OutSet, UWorld* World = nullptr, int32 MaxItemsOverride = -1, bool bUseLoadedWorldFallback = true, bool bAllowPackageFallback = true);
    
    /**
     * Initialize warmup with assets to process
     * @param World - Current world context
     * @param Set - Collection of materials and textures to warm
     * @param MsPerTick - Deprecated compatibility parameter; asynchronous loading is deadline-bound instead.
     */
    void Init(UWorld* World, const FStageZeroWarmupSet& Set, float MsPerTick = 3.0f, bool bAllowMaterialRendering = false);

    /**
     * Poll the wall-clock deadline. Asset loading itself is asynchronous.
     * @param DeltaSeconds - Frame delta time
     */
    void Tick(float DeltaSeconds);

    /**
     * Check if all warmup items have been processed
     * @return true when warmup is complete
     */
    bool IsDone() const;

    /**
     * Get warmup progress percentage
     * @return Progress from 0.0 to 1.0
     */
    float GetProgress() const;

    /**
     * Get completion status. Processed includes both successful and failed terminal items;
     * use WasSuccessful/GetFailureCount to distinguish the result.
     */
    void GetStatus(int32& OutProcessed, int32& OutTotal) const;

    /** True only after every requested asset loaded with its expected type before the deadline. */
    bool WasSuccessful() const;

    /** Number of requested assets that failed to load, had the wrong type, or exceeded the deadline. */
    int32 GetFailureCount() const;

    /** True when the most recent request reached its wall-clock deadline. */
    bool DidTimeOut() const;

    /** Human-readable summary for the most recent failed request. Empty after a successful request. */
    const FString& GetFailureReason() const;

    virtual void BeginDestroy() override;

private:
    void HandleAsyncLoadComplete(uint64 RequestSerial);
    void FinalizeAsyncLoad(uint64 RequestSerial, bool bTimedOut, const FString& FailureContext);
    void CancelActiveRequest();
    int32 CountCurrentlyResolvedItems() const;

    // Pending soft references are retained for validation after the bulk async request completes.
    TArray<TSoftObjectPtr<UMaterialInterface>> PendingMaterialRefs;
    TArray<TSoftObjectPtr<UTexture>> PendingTextureRefs;

    // Strong references keep successfully loaded assets resident for the life of this loader.
    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInterface>> RetainedMaterials;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UTexture>> RetainedTextures;
    
    // Processing state
    int32 TotalMaterials = 0;
    int32 TotalTextures = 0;
    int32 ProcessedMaterials = 0;
    int32 ProcessedTextures = 0;
    int32 FailedMaterials = 0;
    int32 FailedTextures = 0;

    // Retained for binary compatibility with callers that still pass a per-tick budget.
    // No synchronous or incremental loading occurs in Tick.
    float BudgetMs = 3.0f;

    FStreamableManager StreamableManager;
    TSharedPtr<FStreamableHandle> ActiveLoadHandle;
    uint64 LoadRequestSerial = 0;
    uint64 ActiveLoadRequestSerial = 0;
    double LoadDeadlineTimeSeconds = 0.0;
    bool bLoadTerminal = true;
    bool bLoadSuccessful = true;
    bool bLoadTimedOut = false;
    FString LastFailureReason;
};

// Manifest-driven warmup now lives in the subsystem; this class is an async-loading compatibility adapter.
