// Copyright (c) 2025 71 Studios. All rights reserved.

#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "StageZeroLoader.h"
#include "StageZeroSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogStageZeroCmd, Log, All);

namespace
{
const TCHAR* StageZeroResultToString(EStageZeroWarmupResult Result)
{
    switch (Result)
    {
        case EStageZeroWarmupResult::NotStarted:
            return TEXT("NotStarted");
        case EStageZeroWarmupResult::Running:
            return TEXT("Running");
        case EStageZeroWarmupResult::Succeeded:
            return TEXT("Succeeded");
        case EStageZeroWarmupResult::Failed:
            return TEXT("Failed");
        case EStageZeroWarmupResult::Cancelled:
            return TEXT("Cancelled");
        case EStageZeroWarmupResult::TimedOut:
            return TEXT("TimedOut");
        case EStageZeroWarmupResult::Partial:
            return TEXT("Partial");
        default:
            return TEXT("Unknown");
    }
}

UStageZeroSubsystem* GetStageZeroSubsystemFromWorld(UWorld* World)
{
    if (!World)
    {
        return nullptr;
    }

    if (UGameInstance* GameInstance = World->GetGameInstance())
    {
        return GameInstance->GetSubsystem<UStageZeroSubsystem>();
    }

    return nullptr;
}
}

// Command to show runtime tracker status
static FAutoConsoleCommandWithWorld CmdShowTrackerStatus(
    TEXT("StageZero.ShowTrackerStatus"),
    TEXT("Display the current Stage Zero subsystem warmup status"),
    FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
    {
        UStageZeroSubsystem* Subsystem = GetStageZeroSubsystemFromWorld(World);
        if (!Subsystem)
        {
            UE_LOG(LogStageZeroCmd, Warning, TEXT("No Stage Zero subsystem is available for the current world."));
            return;
        }

        const FStageZeroWarmupStatus Status = Subsystem->GetWarmupStatus(false);
        UE_LOG(LogStageZeroCmd, Display, TEXT("=== StageZero Warmup Status ==="));
        UE_LOG(LogStageZeroCmd, Display, TEXT("Result=%s Strict=%s Progress=%.1f%% Tasks=%d/%d Map=%s Maps=%d/%d"),
            StageZeroResultToString(Status.Result),
            Status.bStrictCompletion ? TEXT("true") : TEXT("false"),
            Status.Progress * 100.0f,
            Status.CompletedTasks,
            Status.TotalTasks,
            *Status.CurrentMap.ToString(),
            Status.CompletedMaps,
            Status.TotalMaps);
        UE_LOG(LogStageZeroCmd, Display, TEXT("Manifest=%d/%d Pipeline=%d/%d RemainingPSO=%d Runtime=%d/%d Failed=%d"),
            Status.ManifestAssetsCompleted,
            Status.ManifestAssetsTotal,
            Status.PipelineCacheCompleted,
            Status.PipelineCacheTotal,
            Status.PipelineCacheRemaining,
            Status.RuntimeDiscoveredCompleted,
            Status.RuntimeDiscoveredTotal,
            Status.FailedSkippedTotal);
        if (!Status.StatusMessage.IsEmpty())
        {
            UE_LOG(LogStageZeroCmd, Display, TEXT("StatusMessage=%s"), *Status.StatusMessage);
        }
        UE_LOG(LogStageZeroCmd, Display, TEXT("================================"));
    })
);

// Compatibility command: remove the retired tracker's legacy statistics file.
static FAutoConsoleCommand CmdClearStatistics(
    TEXT("StageZero.ClearStatistics"),
    TEXT("Remove the deprecated runtime shader tracker statistics file"),
    FConsoleCommandDelegate::CreateLambda([]()
    {
        FString FilePath = FPaths::ProjectSavedDir() / TEXT("StageZero") / TEXT("RuntimeShaderStats.json");
        
        if (IFileManager::Get().Delete(*FilePath))
        {
            UE_LOG(LogStageZeroCmd, Display, TEXT("Statistics file deleted: %s"), *FilePath);
        }
        else
        {
            UE_LOG(LogStageZeroCmd, Warning, TEXT("Failed to delete statistics file: %s"), *FilePath);
        }
    })
);

// Compatibility command retained for scripts that used the retired tracker.
static FAutoConsoleCommand CmdToggleVerbose(
    TEXT("StageZero.ToggleVerbose"),
    TEXT("Deprecated: runtime shader tracker logging no longer exists"),
    FConsoleCommandDelegate::CreateLambda([]()
    {
        UE_LOG(LogStageZeroCmd, Warning,
            TEXT("StageZero.ToggleVerbose is deprecated. Use standard log-category verbosity and StageZero.ShowTrackerStatus."));
    })
);

// Command to force transition (debug)
static FAutoConsoleCommandWithWorld CmdForceTransition(
    TEXT("StageZero.ForceTransition"),
    TEXT("Force immediate subsystem transition to the configured primary level (development only)"),
    FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
    {
        if (UStageZeroSubsystem* Subsystem = GetStageZeroSubsystemFromWorld(World))
        {
            const bool bTransitioned = Subsystem->TransitionToPrimaryLevel(false, false);
            UE_LOG(LogStageZeroCmd, Display, TEXT("StageZero subsystem transition %s."), bTransitioned ? TEXT("requested") : TEXT("not requested"));
            if (bTransitioned)
            {
                return;
            }
        }

        if (!World)
        {
            UE_LOG(LogStageZeroCmd, Warning, TEXT("No valid world context"));
            return;
        }

        TArray<AActor*> FoundActors;
        UGameplayStatics::GetAllActorsOfClass(World, AStageZeroLoader::StaticClass(), FoundActors);
        
        for (AActor* Actor : FoundActors)
        {
            if (AStageZeroLoader* Loader = Cast<AStageZeroLoader>(Actor))
            {
                Loader->ForceTransitionNow();
                UE_LOG(LogStageZeroCmd, Display, TEXT("Forced transition to primary level"));
                break;
            }
        }
    })
);
