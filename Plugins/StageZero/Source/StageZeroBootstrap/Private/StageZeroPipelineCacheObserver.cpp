// Copyright (c) 2025 71 Studios. All rights reserved.

#include "StageZeroPipelineCacheObserver.h"

#include "HAL/CriticalSection.h"
#include "Containers/Map.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/ScopeLock.h"
#include "Modules/ModuleManager.h"
#include "ShaderPipelineCache.h"

namespace
{
FCriticalSection GObservationMutex;
struct FObservedCacheRecord
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

TMap<FString, FObservedCacheRecord> GCacheRecords;
uint64 GOpenGeneration = 0;
bool GProtocolError = false;
FString GProtocolErrorReason;

void RecordProtocolError(const FString& Reason)
{
    GProtocolError = true;
    if (GProtocolErrorReason.IsEmpty())
    {
        GProtocolErrorReason = Reason;
    }
}

FObservedCacheRecord* FindUniqueActiveRecordByContext(const FString& ContextName)
{
    FObservedCacheRecord* Match = nullptr;
    for (TPair<FString, FObservedCacheRecord>& Pair : GCacheRecords)
    {
        FObservedCacheRecord& Record = Pair.Value;
        if (!Record.bActive || !Record.ContextName.Equals(ContextName, ESearchCase::CaseSensitive))
        {
            continue;
        }
        if (Match)
        {
            RecordProtocolError(FString::Printf(TEXT("Multiple active pipeline caches shared precompile context '%s'."), *ContextName));
            return nullptr;
        }
        Match = &Record;
    }
    return Match;
}

void RecordCacheOpened(const FString& CacheKey, const FString& ContextName, uint32 Count)
{
    FScopeLock Lock(&GObservationMutex);
    FObservedCacheRecord& Record = GCacheRecords.FindOrAdd(CacheKey);
    if (Record.bActive)
    {
        Record.bIncomplete = true;
        RecordProtocolError(FString::Printf(TEXT("Pipeline cache '%s' opened again before its prior task completed."), *CacheKey));
    }
    Record.CacheKey = CacheKey;
    Record.ContextName = ContextName;
    Record.Generation = ++GOpenGeneration;
    Record.OpenedPSOs = Count;
    Record.ExpectedPSOs = Count;
    Record.CompletedPSOs = 0;
    Record.bActive = true;
    Record.bCompleted = false;
    // A prior incomplete close is cleared only by a matched completion below.
}

void RecordPrecompileBegin(const FString& ContextName, uint32 Count)
{
    FScopeLock Lock(&GObservationMutex);
    if (FObservedCacheRecord* Record = FindUniqueActiveRecordByContext(ContextName))
    {
        Record->ExpectedPSOs = FMath::Max(Record->ExpectedPSOs, Count);
    }
    else if (!GProtocolError)
    {
        RecordProtocolError(FString::Printf(TEXT("Pipeline precompile begin had no matching open cache for context '%s'."), *ContextName));
    }
}

void RecordPrecompileComplete(const FString& ContextName, uint32 Count)
{
    FScopeLock Lock(&GObservationMutex);
    if (FObservedCacheRecord* Record = FindUniqueActiveRecordByContext(ContextName))
    {
        Record->CompletedPSOs = Count;
        Record->bActive = false;
        Record->bCompleted = true;
        Record->bIncomplete = false;
    }
    else if (!GProtocolError)
    {
        RecordProtocolError(FString::Printf(TEXT("Pipeline precompile completion had no matching open cache for context '%s'."), *ContextName));
    }
}

void RecordCacheClosed(const FString& CacheKey)
{
    FScopeLock Lock(&GObservationMutex);
    if (FObservedCacheRecord* Record = GCacheRecords.Find(CacheKey))
    {
        if (Record->bActive)
        {
            Record->bActive = false;
            Record->bIncomplete = true;
        }
    }
    else
    {
        RecordProtocolError(FString::Printf(TEXT("Pipeline cache '%s' closed without a matching observed open."), *CacheKey));
    }
}
}

FStageZeroPipelineCacheObservation StageZeroPipelineCacheObserver::GetSnapshot()
{
    FScopeLock Lock(&GObservationMutex);
    FStageZeroPipelineCacheObservation Snapshot;
    Snapshot.OpenGeneration = GOpenGeneration;
    Snapshot.bProtocolError = GProtocolError;
    Snapshot.ProtocolErrorReason = GProtocolErrorReason;
    uint64 ActiveExpectedPSOs = 0;
    const FObservedCacheRecord* LatestRecord = nullptr;
    for (const TPair<FString, FObservedCacheRecord>& Pair : GCacheRecords)
    {
        const FObservedCacheRecord& Record = Pair.Value;
        FStageZeroPipelineCacheRecordObservation& PublicRecord = Snapshot.CacheRecords.AddDefaulted_GetRef();
        PublicRecord.CacheKey = Record.CacheKey;
        PublicRecord.ContextName = Record.ContextName;
        PublicRecord.Generation = Record.Generation;
        PublicRecord.OpenedPSOs = Record.OpenedPSOs;
        PublicRecord.ExpectedPSOs = Record.ExpectedPSOs;
        PublicRecord.CompletedPSOs = Record.CompletedPSOs;
        PublicRecord.bActive = Record.bActive;
        PublicRecord.bCompleted = Record.bCompleted;
        PublicRecord.bIncomplete = Record.bIncomplete;
        Snapshot.bObservedIncompleteClose |= Record.bIncomplete;
        if (!LatestRecord || Record.Generation > LatestRecord->Generation)
        {
            LatestRecord = &Record;
        }
        if (Record.bActive)
        {
            ++Snapshot.ActiveCacheTasks;
            ActiveExpectedPSOs += Record.ExpectedPSOs;
        }
    }
    if (LatestRecord)
    {
        Snapshot.bObservedCacheOpen = LatestRecord->Generation > 0;
        Snapshot.bObservedCacheCompletion = LatestRecord->bCompleted;
        Snapshot.LastOpenedPSOs = LatestRecord->OpenedPSOs;
        Snapshot.LastCompletedPSOs = LatestRecord->bCompleted ? LatestRecord->CompletedPSOs : 0;
        Snapshot.CompletionGeneration = LatestRecord->bCompleted ? LatestRecord->Generation : 0;
    }
    Snapshot.ActiveExpectedPSOs = static_cast<uint32>(
        FMath::Min<uint64>(ActiveExpectedPSOs, static_cast<uint64>(MAX_uint32)));
    Snapshot.CacheRecords.Sort([](
        const FStageZeroPipelineCacheRecordObservation& Left,
        const FStageZeroPipelineCacheRecordObservation& Right)
    {
        if (Left.Generation != Right.Generation)
        {
            return Left.Generation < Right.Generation;
        }
        return Left.CacheKey < Right.CacheKey;
    });
    return Snapshot;
}

FStageZeroPipelineCacheKeyAssessment StageZeroPipelineCacheObserver::AssessSnapshotForCacheKeys(
    const FStageZeroPipelineCacheObservation& Observation,
    const FString& ExpectedProjectKey,
    const FString& ExpectedUserKey,
    const TSet<FString>& RequiredCacheKeys,
    uint64 RequiredGenerationBaseline)
{
    const auto IsRelevantKey = [&ExpectedProjectKey, &ExpectedUserKey](const FString& CacheKey)
    {
        if (!ExpectedUserKey.IsEmpty() && CacheKey.Equals(ExpectedUserKey, ESearchCase::CaseSensitive))
        {
            return true;
        }
        return !ExpectedProjectKey.IsEmpty()
            && (CacheKey.Equals(ExpectedProjectKey, ESearchCase::CaseSensitive)
                || CacheKey.StartsWith(ExpectedProjectKey + TEXT("_Chunk"), ESearchCase::CaseSensitive));
    };

    FStageZeroPipelineCacheKeyAssessment Assessment;
    for (const FStageZeroPipelineCacheRecordObservation& Record : Observation.CacheRecords)
    {
        if (!IsRelevantKey(Record.CacheKey))
        {
            continue;
        }

        Assessment.bHasRelevantRecord = true;
        Assessment.bHasActiveRelevantRecord |= Record.bActive;
        Assessment.bHasCompletedRelevantRecord |= Record.bCompleted;
        Assessment.bHasIncompleteRelevantRecord |= Record.bIncomplete;
        Assessment.bAllRelevantRecordsCompleted &= Record.bCompleted && !Record.bActive && !Record.bIncomplete;
        if (Record.Generation >= Assessment.LatestRelevantGeneration)
        {
            Assessment.LatestRelevantGeneration = Record.Generation;
            Assessment.LatestOpenedPSOs = Record.OpenedPSOs;
            Assessment.LatestCompletedPSOs = Record.bCompleted ? Record.CompletedPSOs : 0;
        }
    }

    for (const FString& RequiredCacheKey : RequiredCacheKeys)
    {
        const FStageZeroPipelineCacheRecordObservation* RequiredRecord = Observation.CacheRecords.FindByPredicate(
            [&RequiredCacheKey, RequiredGenerationBaseline](const FStageZeroPipelineCacheRecordObservation& Record)
            {
                return Record.CacheKey.Equals(RequiredCacheKey, ESearchCase::CaseSensitive)
                    && Record.Generation > RequiredGenerationBaseline;
            });
        if (!RequiredRecord)
        {
            Assessment.bAllRequiredRecordsObserved = false;
            Assessment.bAllRequiredRecordsCompleted = false;
            continue;
        }
        Assessment.bAllRequiredRecordsCompleted &= RequiredRecord->bCompleted
            && !RequiredRecord->bActive
            && !RequiredRecord->bIncomplete;
    }

    if (!Assessment.bHasRelevantRecord)
    {
        Assessment.bAllRelevantRecordsCompleted = false;
    }
    return Assessment;
}

class FStageZeroBootstrapModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        {
            FScopeLock Lock(&GObservationMutex);
            GCacheRecords.Reset();
            GOpenGeneration = 0;
            GProtocolError = false;
            GProtocolErrorReason.Reset();
        }

        CacheOpenedHandle = FShaderPipelineCache::GetCacheOpenedDelegate().AddLambda(
            [](const FString& CacheKey, EShaderPlatform, uint32 Count, const FGuid&, FShaderPipelineCache::FShaderCachePrecompileContext& Context)
            {
                // Never call back into FShaderPipelineCache here. Unreal may hold
                // its internal cache mutex while broadcasting this delegate.
                RecordCacheOpened(CacheKey, Context.GetCacheName(), Count);
            });
        PrecompileBeginHandle = FShaderPipelineCache::GetPrecompilationBeginDelegate().AddLambda(
            [](uint32 Count, const FShaderPipelineCache::FShaderCachePrecompileContext& Context)
            {
                RecordPrecompileBegin(Context.GetCacheName(), Count);
            });
        PrecompileCompleteHandle = FShaderPipelineCache::GetPrecompilationCompleteDelegate().AddLambda(
            [](uint32 Count, double, const FShaderPipelineCache::FShaderCachePrecompileContext& Context)
            {
                RecordPrecompileComplete(Context.GetCacheName(), Count);
            });
        CacheClosedHandle = FShaderPipelineCache::GetCacheClosedDelegate().AddLambda(
            [](const FString& CacheKey, EShaderPlatform)
            {
                RecordCacheClosed(CacheKey);
            });
    }

    virtual void ShutdownModule() override
    {
        FShaderPipelineCache::GetCacheOpenedDelegate().Remove(CacheOpenedHandle);
        FShaderPipelineCache::GetPrecompilationBeginDelegate().Remove(PrecompileBeginHandle);
        FShaderPipelineCache::GetPrecompilationCompleteDelegate().Remove(PrecompileCompleteHandle);
        FShaderPipelineCache::GetCacheClosedDelegate().Remove(CacheClosedHandle);
        CacheOpenedHandle.Reset();
        PrecompileBeginHandle.Reset();
        PrecompileCompleteHandle.Reset();
        CacheClosedHandle.Reset();
    }

private:
    FDelegateHandle CacheOpenedHandle;
    FDelegateHandle PrecompileBeginHandle;
    FDelegateHandle PrecompileCompleteHandle;
    FDelegateHandle CacheClosedHandle;
};

IMPLEMENT_MODULE(FStageZeroBootstrapModule, StageZeroBootstrap)
