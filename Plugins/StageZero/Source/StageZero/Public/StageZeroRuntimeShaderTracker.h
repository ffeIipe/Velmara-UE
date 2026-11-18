// Copyright (c) 2025 71 Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "StageZeroRuntimeShaderTracker.generated.h"

class ULevelStreaming;
class UStageZeroSubsystem;
class UWorld;

USTRUCT()
struct FShaderProgressMetrics
{
    GENERATED_BODY()

    // PSO Compilation metrics
    int32 TotalPSOsExpected = 0;
    int32 CompletedPSOs = 0;
    int32 ActivePSOCompilations = 0;
    
    // Material state metrics
    int32 InitialIncompleteMaterials = 0;
    int32 CurrentIncompleteMaterials = 0;
    
    // Shader cache metrics
    int32 ExpectedCacheEntries = 0;
    int32 LoadedCacheEntries = 0;
    
    // Render fence metrics
    int32 PendingRenderFences = 0;
    int32 CompletedRenderFences = 0;
    
    // Timing metrics
    double StartTime = 0.0;
    double LastUpdateTime = 0.0;
    double EstimatedTotalTime = 10.0; // Default 10 seconds
    
    // Progress tracking
    float LastReportedProgress = 0.0f;
    float SmoothingFactor = 0.15f; // For exponential smoothing
    
    // Dynamic total management
    int32 HighWaterMarkTotal = 0; // Tracks the highest total we've seen
    int32 EstimatedHeadroom = 0; // Extra jobs we expect but haven't seen yet
    float ProgressCeiling = 0.95f; // Max progress until we're truly done
    bool bHasReachedCompletion = false; // True when we've seen jobs stop growing
};

USTRUCT()
struct FShaderTimingStatistics
{
    GENERATED_BODY()
    
    // Historical data for prediction
    TMap<FString, float> LevelCompileTimes;
    TMap<int32, float> MaterialCountToTime;
    TMap<FString, int32> LevelPSOCounts;
    
    // Running averages
    float AverageTimePerMaterial = 0.05f; // 50ms default
    float AverageTimePerPSO = 0.02f; // 20ms default
    float AverageTimePerCacheEntry = 0.01f; // 10ms default
    
    // Last session data
    FString LastLevelLoaded;
    float LastCompileTime = 0.0f;
    int32 LastMaterialCount = 0;
    int32 LastPSOCount = 0;
};

/**
 * Deprecated source-compatibility shell.
 *
 * The former implementation reported heuristic shader/PSO progress and inserted
 * render fences without owning the underlying work. Stage Zero now exposes only
 * the canonical subsystem ledger and recorded pipeline-cache stage. This class
 * remains for one compatibility cycle so external C++ code still links.
 */
UCLASS(meta = (DeprecatedNode, DeprecationMessage = "Use UStageZeroSubsystem::GetWarmupStatus. Runtime shader progress cannot be inferred safely."))
class STAGEZERO_API UStageZeroRuntimeShaderTracker : public UObject
{
    GENERATED_BODY()

public:
    UStageZeroRuntimeShaderTracker();
    virtual ~UStageZeroRuntimeShaderTracker();

    /**
     * Initialize the tracker with world context and level information
     */
    void Initialize(UWorld* InWorld, const TArray<ULevelStreaming*>& StreamedLevels);
    
    /**
     * Shutdown and cleanup all tracking systems
     */
    void Shutdown();
    
    /**
     * Update tracking metrics (call each tick)
     */
    void Tick(float DeltaTime);
    
    /**
     * Get current progress (0.0 - 1.0)
     */
    float GetProgress() const;
    
    /**
     * Get detailed job counts
     */
    void GetJobCounts(int32& OutCompleted, int32& OutTotal) const;
    
    /**
     * Check if all tracked work is complete
     */
    bool IsComplete() const;
    
    /**
     * Force a rescan of materials (expensive, use sparingly)
     */
    void RefreshMaterialStates();
    
    /**
     * Save timing statistics for future predictions
     */
    void SaveStatistics();
    
    /**
     * Load timing statistics from disk
     */
    void LoadStatistics();

    /**
     * Get human-readable status for debugging
     */
    FString GetDebugStatus() const;

    /** True when this compatibility facade is attached to the authoritative Stage Zero subsystem. */
    bool IsAvailable() const;
    
private:
    /** Read-only bridge to owned Stage Zero work. It never starts, discovers, renders, or fences work. */
    TWeakObjectPtr<UStageZeroSubsystem> Subsystem;

    bool bWarningEmitted = false;
};
