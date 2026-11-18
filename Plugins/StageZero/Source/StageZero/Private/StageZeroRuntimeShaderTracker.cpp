// Copyright (c) 2025 71 Studios. All rights reserved.

#include "StageZeroRuntimeShaderTracker.h"

#include "StageZeroSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogStageZeroDeprecatedTracker, Log, All);

UStageZeroRuntimeShaderTracker::UStageZeroRuntimeShaderTracker() = default;
UStageZeroRuntimeShaderTracker::~UStageZeroRuntimeShaderTracker() = default;

void UStageZeroRuntimeShaderTracker::Initialize(UWorld* InWorld, const TArray<ULevelStreaming*>& /*StreamedLevels*/)
{
    Shutdown();
    if (InWorld)
    {
        if (UGameInstance* GameInstance = InWorld->GetGameInstance())
        {
            Subsystem = GameInstance->GetSubsystem<UStageZeroSubsystem>();
        }
    }

    if (!bWarningEmitted)
    {
        bWarningEmitted = true;
        UE_LOG(LogStageZeroDeprecatedTracker, Warning,
            TEXT("UStageZeroRuntimeShaderTracker is deprecated. Its read-only queries now forward to UStageZeroSubsystem; legacy shader heuristics and render fences remain disabled."));
    }
}

void UStageZeroRuntimeShaderTracker::Shutdown()
{
    Subsystem.Reset();
}

void UStageZeroRuntimeShaderTracker::Tick(float /*DeltaTime*/)
{
}

float UStageZeroRuntimeShaderTracker::GetProgress() const
{
    return Subsystem.IsValid() ? Subsystem->GetWarmupProgress(false) : 0.0f;
}

void UStageZeroRuntimeShaderTracker::GetJobCounts(int32& OutCompleted, int32& OutTotal) const
{
    if (Subsystem.IsValid())
    {
        Subsystem->GetTaskCounts(OutCompleted, OutTotal, false);
        return;
    }

    OutCompleted = 0;
    OutTotal = 0;
}

bool UStageZeroRuntimeShaderTracker::IsComplete() const
{
    return Subsystem.IsValid() && Subsystem->IsWarmupCompleted(false);
}

void UStageZeroRuntimeShaderTracker::RefreshMaterialStates()
{
}

void UStageZeroRuntimeShaderTracker::SaveStatistics()
{
}

void UStageZeroRuntimeShaderTracker::LoadStatistics()
{
}

FString UStageZeroRuntimeShaderTracker::GetDebugStatus() const
{
    if (!Subsystem.IsValid())
    {
        return TEXT("Deprecated compatibility facade: no Stage Zero subsystem is attached.");
    }

    const FStageZeroWarmupStatus Status = Subsystem->GetWarmupStatus(false);
    return FString::Printf(
        TEXT("Deprecated compatibility facade: Result=%d Progress=%.2f%% Tasks=%d/%d Map=%s"),
        static_cast<int32>(Status.Result),
        Status.Progress * 100.0f,
        Status.CompletedTasks,
        Status.TotalTasks,
        *Status.CurrentMap.ToString());
}

bool UStageZeroRuntimeShaderTracker::IsAvailable() const
{
    return Subsystem.IsValid();
}
