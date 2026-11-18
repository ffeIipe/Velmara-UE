// Copyright (c) 2025 71 Studios. All rights reserved.

#pragma once

#include "Containers/Array.h"
#include "Containers/Set.h"
#include "Containers/UnrealString.h"
#include "CoreTypes.h"

/** One cache-key-scoped lifecycle record observed from Unreal's public delegates. */
struct FStageZeroPipelineCacheRecordObservation
{
    FString CacheKey;
    FString ContextName;
    uint64 Generation = 0;
    uint32 OpenedPSOs = 0;
    uint32 ExpectedPSOs = 0;
    uint32 CompletedPSOs = 0;
    bool bActive = false;
    bool bCompleted = false;
    bool bIncomplete = false;
};

/**
 * Thread-safe snapshot of recorded pipeline-cache work observed in this process.
 *
 * The bootstrap module subscribes before Unreal opens the startup cache. The
 * runtime module reads this value later and never infers cache ownership from
 * global PSO-precache counters.
 */
struct FStageZeroPipelineCacheObservation
{
    bool bObservedCacheOpen = false;
    bool bObservedCacheCompletion = false;
    bool bObservedIncompleteClose = false;
    bool bProtocolError = false;
    FString ProtocolErrorReason;
    int32 ActiveCacheTasks = 0;
    uint32 ActiveExpectedPSOs = 0;
    uint32 LastOpenedPSOs = 0;
    uint32 LastCompletedPSOs = 0;
    uint64 OpenGeneration = 0;
    uint64 CompletionGeneration = 0;
    TArray<FStageZeroPipelineCacheRecordObservation> CacheRecords;
};

/** Cache-key-scoped interpretation used by the runtime coordinator and tests. */
struct FStageZeroPipelineCacheKeyAssessment
{
    bool bHasRelevantRecord = false;
    bool bHasActiveRelevantRecord = false;
    bool bHasCompletedRelevantRecord = false;
    bool bHasIncompleteRelevantRecord = false;
    bool bAllRelevantRecordsCompleted = true;
    bool bAllRequiredRecordsObserved = true;
    bool bAllRequiredRecordsCompleted = true;
    uint32 LatestOpenedPSOs = 0;
    uint32 LatestCompletedPSOs = 0;
    uint64 LatestRelevantGeneration = 0;
};

namespace StageZeroPipelineCacheObserver
{
/** Return a coherent copy of the current process-local observation ledger. */
STAGEZEROBOOTSTRAP_API FStageZeroPipelineCacheObservation GetSnapshot();

/**
 * Assess only the expected project/base-chunk and user cache identities.
 * Required keys must have a record newer than RequiredGenerationBaseline.
 */
STAGEZEROBOOTSTRAP_API FStageZeroPipelineCacheKeyAssessment AssessSnapshotForCacheKeys(
    const FStageZeroPipelineCacheObservation& Observation,
    const FString& ExpectedProjectKey,
    const FString& ExpectedUserKey,
    const TSet<FString>& RequiredCacheKeys,
    uint64 RequiredGenerationBaseline);
}
