// Copyright (c) 2025 71 Studios. All rights reserved.

#include "StageZeroBlueprintLibrary.h"
#include "StageZeroProjectSettings.h"
#include "StageZeroLoader.h"
#include "StageZeroSubsystem.h"
#include "Engine/Texture2D.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"

namespace
{
UStageZeroSubsystem* FindStageZeroSubsystemFromAnyWorld()
{
    if (!GEngine)
    {
        return nullptr;
    }

    for (const FWorldContext& Context : GEngine->GetWorldContexts())
    {
        UWorld* World = Context.World();
        if (!World)
        {
            continue;
        }

        if (!World->IsGameWorld() && !World->IsPlayInEditor())
        {
            continue;
        }

        if (UGameInstance* GI = World->GetGameInstance())
        {
            if (UStageZeroSubsystem* Subsystem = GI->GetSubsystem<UStageZeroSubsystem>())
            {
                return Subsystem;
            }
        }
    }

    return nullptr;
}
}

UStageZeroProjectSettings* UStageZeroBlueprintLibrary::GetMutableSettings()
{
    return GetMutableDefault<UStageZeroProjectSettings>();
}

const UStageZeroProjectSettings* UStageZeroBlueprintLibrary::GetSettings()
{
    return GetDefault<UStageZeroProjectSettings>();
}

void UStageZeroBlueprintLibrary::MaybeShowDebugMessage(const UObject* WorldContextObject, bool bShowDebug, const FString& Message)
{
    if (!bShowDebug || Message.IsEmpty())
    {
        return;
    }

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, Message);
    }

    UE_LOG(LogStageZeroSettings, Display, TEXT("%s"), *Message);
}

void UStageZeroBlueprintLibrary::SetWarmUpAllMaps(bool bEnabled, bool bShowDebug)
{
    if (UStageZeroSubsystem* Subsystem = FindStageZeroSubsystemFromAnyWorld())
    {
        Subsystem->SetWarmUpAllMaps(bEnabled, bShowDebug);
        UE_LOG(LogStageZeroSettings, Display, TEXT("StageZero: WarmUpAllMaps set to %s"), bEnabled ? TEXT("true") : TEXT("false"));
        MaybeShowDebugMessage(nullptr, bShowDebug, FString::Printf(TEXT("StageZero: WarmUpAllMaps set to %s"), bEnabled ? TEXT("true") : TEXT("false")));
        return;
    }

    UE_LOG(LogStageZeroSettings, Warning, TEXT("StageZero: WarmUpAllMaps could not be set (no valid world/subsystem)."));
    MaybeShowDebugMessage(nullptr, bShowDebug, TEXT("StageZero: WarmUpAllMaps failed (no valid world/subsystem)"));
}

void UStageZeroBlueprintLibrary::GetWarmUpAllMaps(bool& bEnabled)
{
    bEnabled = false;
    if (const UStageZeroSubsystem* Subsystem = FindStageZeroSubsystemFromAnyWorld())
    {
        bEnabled = Subsystem->IsWarmUpAllMapsEnabled();
    }
}

void UStageZeroBlueprintLibrary::SetWarmUpLevelInstances(bool bEnabled, bool bShowDebug)
{
    if (UStageZeroProjectSettings* Settings = GetMutableSettings())
    {
        Settings->bWarmUpLevelInstances = bEnabled;
        Settings->SaveConfig();
        UE_LOG(LogStageZeroSettings, Display, TEXT("StageZero: WarmUpLevelInstances set to %s"), bEnabled ? TEXT("true") : TEXT("false"));
        MaybeShowDebugMessage(nullptr, bShowDebug, FString::Printf(TEXT("StageZero: WarmUpLevelInstances set to %s"), bEnabled ? TEXT("true") : TEXT("false")));
    }
}

void UStageZeroBlueprintLibrary::GetWarmUpLevelInstances(bool& bEnabled)
{
    bEnabled = false;
    if (const UStageZeroProjectSettings* Settings = GetSettings())
    {
        bEnabled = Settings->bWarmUpLevelInstances;
    }
}

void UStageZeroBlueprintLibrary::SetWarmUpPackedLevelActors(bool bEnabled, bool bShowDebug)
{
    if (UStageZeroProjectSettings* Settings = GetMutableSettings())
    {
        Settings->bWarmUpPackedLevelActors = bEnabled;
        Settings->SaveConfig();
        UE_LOG(LogStageZeroSettings, Display, TEXT("StageZero: WarmUpPackedLevelActors set to %s"), bEnabled ? TEXT("true") : TEXT("false"));
        MaybeShowDebugMessage(nullptr, bShowDebug, FString::Printf(TEXT("StageZero: WarmUpPackedLevelActors set to %s"), bEnabled ? TEXT("true") : TEXT("false")));
    }
}

void UStageZeroBlueprintLibrary::GetWarmUpPackedLevelActors(bool& bEnabled)
{
    bEnabled = false;
    if (const UStageZeroProjectSettings* Settings = GetSettings())
    {
        bEnabled = Settings->bWarmUpPackedLevelActors;
    }
}

void UStageZeroBlueprintLibrary::SetLoadingIconTexture(const TSoftObjectPtr<UTexture2D>& Icon, bool bShowDebug)
{
    if (UStageZeroProjectSettings* Settings = GetMutableSettings())
    {
        Settings->LoadingIconTexture = Icon;
        Settings->SaveConfig();
        UE_LOG(LogStageZeroSettings, Display, TEXT("StageZero: Loading icon updated to %s"), *Icon.ToString());
        MaybeShowDebugMessage(nullptr, bShowDebug, FString::Printf(TEXT("StageZero: Loading icon updated to %s"), *Icon.ToString()));
    }
}

void UStageZeroBlueprintLibrary::GetLoadingIconTexture(TSoftObjectPtr<UTexture2D>& OutIcon)
{
    OutIcon.Reset();
    if (const UStageZeroProjectSettings* Settings = GetSettings())
    {
        OutIcon = Settings->LoadingIconTexture;
    }
}

void UStageZeroBlueprintLibrary::SetBackgroundSlides(const TArray<TSoftObjectPtr<UTexture2D>>& Slides, bool bShowDebug)
{
    if (UStageZeroProjectSettings* Settings = GetMutableSettings())
    {
        Settings->BackgroundSlides = Slides;
        Settings->SaveConfig();
        UE_LOG(LogStageZeroSettings, Display, TEXT("StageZero: Set %d background slides"), Settings->BackgroundSlides.Num());
        MaybeShowDebugMessage(nullptr, bShowDebug, FString::Printf(TEXT("StageZero: Set %d background slides"), Settings->BackgroundSlides.Num()));
    }
}

void UStageZeroBlueprintLibrary::GetBackgroundSlides(TArray<TSoftObjectPtr<UTexture2D>>& OutSlides)
{
    OutSlides.Reset();
    if (const UStageZeroProjectSettings* Settings = GetSettings())
    {
        OutSlides = Settings->BackgroundSlides;
    }
}

void UStageZeroBlueprintLibrary::SetBootTips(const TArray<FText>& Tips, bool bShowDebug)
{
    if (UStageZeroProjectSettings* Settings = GetMutableSettings())
    {
        Settings->BootTips = Tips;
        Settings->SaveConfig();
        UE_LOG(LogStageZeroSettings, Display, TEXT("StageZero: Set %d boot tips"), Settings->BootTips.Num());
        MaybeShowDebugMessage(nullptr, bShowDebug, FString::Printf(TEXT("StageZero: Set %d boot tips"), Settings->BootTips.Num()));
    }
}

void UStageZeroBlueprintLibrary::GetBootTips(TArray<FText>& OutTips)
{
    OutTips.Reset();
    if (const UStageZeroProjectSettings* Settings = GetSettings())
    {
        OutTips = Settings->BootTips;
    }
}

void UStageZeroBlueprintLibrary::SetBackgroundSlideInterval(float Interval, bool bShowDebug)
{
    if (UStageZeroProjectSettings* Settings = GetMutableSettings())
    {
        Settings->BackgroundSlideInterval = FMath::Max(0.1f, Interval);
        Settings->SaveConfig();
        MaybeShowDebugMessage(nullptr, bShowDebug, FString::Printf(TEXT("StageZero: BackgroundSlideInterval set to %.2f"), Settings->BackgroundSlideInterval));
    }
}

void UStageZeroBlueprintLibrary::SetBootTipInterval(float Interval, bool bShowDebug)
{
    if (UStageZeroProjectSettings* Settings = GetMutableSettings())
    {
        Settings->BootTipInterval = FMath::Max(0.1f, Interval);
        Settings->SaveConfig();
        MaybeShowDebugMessage(nullptr, bShowDebug, FString::Printf(TEXT("StageZero: BootTipInterval set to %.2f"), Settings->BootTipInterval));
    }
}

void UStageZeroBlueprintLibrary::SetSpinnerOnlyPhaseDuration(float Duration, bool bShowDebug)
{
    if (UStageZeroProjectSettings* Settings = GetMutableSettings())
    {
        Settings->SpinnerOnlyPhaseDuration = FMath::Max(0.0f, Duration);
        Settings->SaveConfig();
        MaybeShowDebugMessage(nullptr, bShowDebug, FString::Printf(TEXT("StageZero: SpinnerOnlyPhaseDuration set to %.2f"), Settings->SpinnerOnlyPhaseDuration));
    }
}

void UStageZeroBlueprintLibrary::SetShowSpinnerDuringProgress(bool bShow, bool bShowDebug)
{
    if (UStageZeroProjectSettings* Settings = GetMutableSettings())
    {
        Settings->bShowSpinnerDuringProgress = bShow;
        Settings->SaveConfig();
        MaybeShowDebugMessage(nullptr, bShowDebug, FString::Printf(TEXT("StageZero: ShowSpinnerDuringProgress set to %s"), bShow ? TEXT("true") : TEXT("false")));
    }
}

void UStageZeroBlueprintLibrary::SetLoopBackgroundSlides(bool bLoop, bool bShowDebug)
{
    if (UStageZeroProjectSettings* Settings = GetMutableSettings())
    {
        Settings->bLoopBackgroundSlides = bLoop;
        Settings->SaveConfig();
        MaybeShowDebugMessage(nullptr, bShowDebug, FString::Printf(TEXT("StageZero: LoopBackgroundSlides set to %s"), bLoop ? TEXT("true") : TEXT("false")));
    }
}

void UStageZeroBlueprintLibrary::ClearMapsToWarmUp(bool bShowDebug)
{
    if (UStageZeroSubsystem* Subsystem = FindStageZeroSubsystemFromAnyWorld())
    {
        Subsystem->ClearWarmupMaps(bShowDebug);
        UE_LOG(LogStageZeroSettings, Display, TEXT("StageZero: Cleared warmup maps (runtime override)"));
        MaybeShowDebugMessage(nullptr, bShowDebug, TEXT("StageZero: Cleared warmup maps (runtime override)"));
        return;
    }

    UE_LOG(LogStageZeroSettings, Warning, TEXT("StageZero: ClearMapsToWarmUp failed (no valid world/subsystem)."));
    MaybeShowDebugMessage(nullptr, bShowDebug, TEXT("StageZero: ClearMapsToWarmUp failed (no valid world/subsystem)"));
}

void UStageZeroBlueprintLibrary::AddMapToWarmUp(const TSoftObjectPtr<UWorld>& Map, bool bShowDebug)
{
    if (Map.IsNull())
    {
        UE_LOG(LogStageZeroSettings, Warning, TEXT("StageZero: AddMapToWarmUp called with null map"));
        return;
    }

    if (UStageZeroSubsystem* Subsystem = FindStageZeroSubsystemFromAnyWorld())
    {
        Subsystem->AddWarmupMap(Map, bShowDebug);
        UE_LOG(LogStageZeroSettings, Display, TEXT("StageZero: Added warmup map (runtime override): %s"), *Map.ToString());
        MaybeShowDebugMessage(nullptr, bShowDebug, FString::Printf(TEXT("StageZero: Added warmup map (runtime override): %s"), *Map.ToString()));
        return;
    }

    UE_LOG(LogStageZeroSettings, Warning, TEXT("StageZero: AddMapToWarmUp failed (no valid world/subsystem)."));
    MaybeShowDebugMessage(nullptr, bShowDebug, TEXT("StageZero: AddMapToWarmUp failed (no valid world/subsystem)"));
}

void UStageZeroBlueprintLibrary::SetMapsToWarmUp(const TArray<TSoftObjectPtr<UWorld>>& Maps, bool bShowDebug)
{
    if (UStageZeroSubsystem* Subsystem = FindStageZeroSubsystemFromAnyWorld())
    {
        Subsystem->SetWarmupMaps(Maps, bShowDebug);
        UE_LOG(LogStageZeroSettings, Display, TEXT("StageZero: Set %d warmup maps (runtime override)"), Maps.Num());
        MaybeShowDebugMessage(nullptr, bShowDebug, FString::Printf(TEXT("StageZero: Set %d warmup maps (runtime override)"), Maps.Num()));
        return;
    }

    UE_LOG(LogStageZeroSettings, Warning, TEXT("StageZero: SetMapsToWarmUp failed (no valid world/subsystem)."));
    MaybeShowDebugMessage(nullptr, bShowDebug, TEXT("StageZero: SetMapsToWarmUp failed (no valid world/subsystem)"));
}

void UStageZeroBlueprintLibrary::GetMapsToWarmUp(TArray<TSoftObjectPtr<UWorld>>& OutMaps)
{
    OutMaps.Reset();
    if (const UStageZeroSubsystem* Subsystem = FindStageZeroSubsystemFromAnyWorld())
    {
        OutMaps = Subsystem->GetWarmupMaps();
    }
}

bool UStageZeroBlueprintLibrary::TriggerStageZeroWarmUp(const UObject* WorldContextObject, bool bShowDebug)
{
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!World)
    {
        UE_LOG(LogStageZeroSettings, Warning, TEXT("StageZero: TriggerStageZeroWarmUp failed - invalid world context"));
        return false;
    }

    UGameInstance* GI = World->GetGameInstance();
    UStageZeroSubsystem* Subsystem = GI ? GI->GetSubsystem<UStageZeroSubsystem>() : nullptr;

    AStageZeroLoader* LoaderActor = nullptr;
    for (TActorIterator<AStageZeroLoader> It(World); It; ++It)
    {
        if (AStageZeroLoader* Candidate = *It)
        {
            LoaderActor = Candidate;
            break;
        }
    }

    if (Subsystem)
    {
        if (LoaderActor)
        {
            Subsystem->ApplyLegacyActorSettings(LoaderActor);
            LoaderActor->BindSubsystemDelegates(Subsystem);
        }

        // Ensure the loading screen is always presented when triggering via BP.
        Subsystem->SetAutoSpawnWidget(true);

        Subsystem->StartWarmup(bShowDebug);
        MaybeShowDebugMessage(WorldContextObject, bShowDebug, TEXT("StageZero: Warmup triggered via Blueprint"));
        return true;
    }

    if (LoaderActor)
    {
        LoaderActor->StartStageZero();
        MaybeShowDebugMessage(WorldContextObject, bShowDebug, TEXT("StageZero: Warmup triggered via legacy loader fallback"));
        return true;
    }

    UE_LOG(LogStageZeroSettings, Warning, TEXT("StageZero: No StageZeroLoader found in world %s"), *World->GetMapName());
    MaybeShowDebugMessage(WorldContextObject, bShowDebug, TEXT("StageZero: Warmup trigger failed (no loader found)"));
    return false;
}

EStageZeroWarmupStartResult UStageZeroBlueprintLibrary::TriggerStageZeroWarmUpIfNeeded(const UObject* WorldContextObject, bool bRequirePipelineCacheClear, bool bShowDebug)
{
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!World)
    {
        UE_LOG(LogStageZeroSettings, Warning, TEXT("StageZero: TriggerStageZeroWarmUpIfNeeded failed - invalid world context"));
        MaybeShowDebugMessage(WorldContextObject, bShowDebug, TEXT("StageZero: Warmup-if-needed failed (invalid world context)"));
        return EStageZeroWarmupStartResult::Failed;
    }

    UGameInstance* GI = World->GetGameInstance();
    UStageZeroSubsystem* Subsystem = GI ? GI->GetSubsystem<UStageZeroSubsystem>() : nullptr;
    if (!Subsystem)
    {
        UE_LOG(LogStageZeroSettings, Warning, TEXT("StageZero: TriggerStageZeroWarmUpIfNeeded failed - no subsystem"));
        MaybeShowDebugMessage(WorldContextObject, bShowDebug, TEXT("StageZero: Warmup-if-needed failed (no subsystem)"));
        return EStageZeroWarmupStartResult::Failed;
    }

    for (TActorIterator<AStageZeroLoader> It(World); It; ++It)
    {
        if (AStageZeroLoader* LoaderActor = *It)
        {
            Subsystem->ApplyLegacyActorSettings(LoaderActor);
            LoaderActor->BindSubsystemDelegates(Subsystem);
            break;
        }
    }

    const EStageZeroWarmupStartResult Result = Subsystem->StartWarmupIfNeeded(bRequirePipelineCacheClear, bShowDebug);
    MaybeShowDebugMessage(WorldContextObject, bShowDebug,
        TEXT("StageZero: Deprecated warmup-if-needed wrapper started current-process warmup; disk-cache skipping is retired."));
    return Result;
}

bool UStageZeroBlueprintLibrary::CancelStageZeroWarmUp(const UObject* WorldContextObject, bool bShowDebug)
{
    if (UStageZeroSubsystem* Subsystem = GetStageZeroSubsystem(WorldContextObject))
    {
        Subsystem->CancelWarmup(bShowDebug);
        MaybeShowDebugMessage(WorldContextObject, bShowDebug, TEXT("StageZero: Warmup cancelled"));
        return true;
    }

    MaybeShowDebugMessage(WorldContextObject, bShowDebug, TEXT("StageZero: Cancel warmup failed (no subsystem)"));
    return false;
}

UStageZeroSubsystem* UStageZeroBlueprintLibrary::GetStageZeroSubsystem(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (World)
    {
        if (UGameInstance* GI = World->GetGameInstance())
        {
            return GI->GetSubsystem<UStageZeroSubsystem>();
        }
    }

    return nullptr;
}

void UStageZeroBlueprintLibrary::SetStageZeroBackgroundSlides(const UObject* WorldContextObject, const TArray<TSoftObjectPtr<UTexture2D>>& Slides, bool bLoopSlides, bool bShowDebug)
{
    if (UStageZeroSubsystem* Subsystem = GetStageZeroSubsystem(WorldContextObject))
    {
        Subsystem->SetBackgroundSlidesOverride(Slides, bLoopSlides, bShowDebug);
    }
    else
    {
        UE_LOG(LogStageZeroSettings, Warning, TEXT("StageZero: SetStageZeroBackgroundSlides failed - no valid world/subsystem."));
    }
}

FStageZeroWarmupStatus UStageZeroBlueprintLibrary::GetStageZeroWarmupStatus(const UObject* WorldContextObject)
{
    if (const UStageZeroSubsystem* Subsystem = GetStageZeroSubsystem(WorldContextObject))
    {
        return Subsystem->GetWarmupStatus();
    }
    return FStageZeroWarmupStatus();
}

bool UStageZeroBlueprintLibrary::CanSkipStageZeroWarmup(const UObject* WorldContextObject, bool bRequirePipelineCacheClear, bool bShowDebug)
{
    if (UStageZeroSubsystem* Subsystem = GetStageZeroSubsystem(WorldContextObject))
    {
        return Subsystem->CanSkipStageZeroWarmup(bRequirePipelineCacheClear, bShowDebug);
    }
    return false;
}

FStageZeroWarmupCacheStatus UStageZeroBlueprintLibrary::GetStageZeroWarmupCacheStatus(const UObject* WorldContextObject, bool bRequirePipelineCacheClear, bool bShowDebug)
{
    if (UStageZeroSubsystem* Subsystem = GetStageZeroSubsystem(WorldContextObject))
    {
        return Subsystem->GetWarmupCacheStatus(bRequirePipelineCacheClear, bShowDebug);
    }
    return FStageZeroWarmupCacheStatus();
}

void UStageZeroBlueprintLibrary::GetStageZeroWarmupMaps(const UObject* WorldContextObject, TArray<TSoftObjectPtr<UWorld>>& OutMaps)
{
    OutMaps.Reset();
    if (const UStageZeroSubsystem* Subsystem = GetStageZeroSubsystem(WorldContextObject))
    {
        OutMaps = Subsystem->GetWarmupMaps();
    }
}

TSoftObjectPtr<UWorld> UStageZeroBlueprintLibrary::GetStageZeroPrimaryLevel(const UObject* WorldContextObject)
{
    if (const UStageZeroSubsystem* Subsystem = GetStageZeroSubsystem(WorldContextObject))
    {
        return Subsystem->GetPrimaryLevel();
    }
    return TSoftObjectPtr<UWorld>();
}

float UStageZeroBlueprintLibrary::NormalizeWarmupProgress(int32 CompletedTasks, int32 TotalTasks)
{
    const int32 SafeTotal = FMath::Max(TotalTasks, 1);
    const float Percent01 = FMath::Clamp(static_cast<float>(CompletedTasks) / static_cast<float>(SafeTotal), 0.0f, 1.0f);
    return FMath::FloorToFloat(Percent01 * 100.0f);
}

void UStageZeroBlueprintLibrary::GetStageZeroMapCounts(const UObject* WorldContextObject, int32& OutCompleted, int32& OutTotal)
{
    OutCompleted = 0;
    OutTotal = 0;
    if (const UStageZeroSubsystem* Subsystem = GetStageZeroSubsystem(WorldContextObject))
    {
        OutCompleted = Subsystem->GetCompletedMaps();
        OutTotal = Subsystem->GetTotalMaps();
    }
}

FStageZeroBootViewConfig UStageZeroBlueprintLibrary::GetStageZeroBootConfig(const UObject* WorldContextObject)
{
    if (const UStageZeroSubsystem* Subsystem = GetStageZeroSubsystem(WorldContextObject))
    {
        return Subsystem->GetBootViewConfig();
    }
    return FStageZeroBootViewConfig();
}

void UStageZeroBlueprintLibrary::SetStageZeroBootConfig(const UObject* WorldContextObject, const FStageZeroBootViewConfig& InConfig, bool bShowDebug)
{
    if (UStageZeroSubsystem* Subsystem = GetStageZeroSubsystem(WorldContextObject))
    {
        Subsystem->SetBootViewConfig(InConfig, bShowDebug);
        MaybeShowDebugMessage(WorldContextObject, bShowDebug, TEXT("StageZero: Boot config applied at runtime"));
    }
}

void UStageZeroBlueprintLibrary::SetStageZeroAutoTransition(const UObject* WorldContextObject, bool bEnable, bool bShowDebug)
{
    if (UStageZeroSubsystem* Subsystem = GetStageZeroSubsystem(WorldContextObject))
    {
        Subsystem->SetAutoTransitionToPrimary(bEnable, bShowDebug);
        MaybeShowDebugMessage(WorldContextObject, bShowDebug, FString::Printf(TEXT("StageZero: Auto transition set to %s"), bEnable ? TEXT("true") : TEXT("false")));
    }
}

void UStageZeroBlueprintLibrary::SetStageZeroAutoWidget(const UObject* WorldContextObject, bool bEnable, bool bShowDebug)
{
    if (UStageZeroSubsystem* Subsystem = GetStageZeroSubsystem(WorldContextObject))
    {
        Subsystem->SetAutoSpawnWidget(bEnable, bShowDebug);
        MaybeShowDebugMessage(WorldContextObject, bShowDebug, FString::Printf(TEXT("StageZero: Auto widget spawn set to %s"), bEnable ? TEXT("true") : TEXT("false")));
    }
}

void UStageZeroBlueprintLibrary::SetStageZeroAllowCreatePlayer(const UObject* WorldContextObject, bool bEnable, bool bShowDebug)
{
    if (UStageZeroSubsystem* Subsystem = GetStageZeroSubsystem(WorldContextObject))
    {
        Subsystem->SetAllowCreatePlayerIfMissing(bEnable, bShowDebug);
        MaybeShowDebugMessage(WorldContextObject, bShowDebug, FString::Printf(TEXT("StageZero: AllowCreatePlayerIfMissing set to %s"), bEnable ? TEXT("true") : TEXT("false")));
    }
}

void UStageZeroBlueprintLibrary::SetStageZeroMinimumDisplaySeconds(const UObject* WorldContextObject, float Seconds, bool bShowDebug)
{
    if (UStageZeroSubsystem* Subsystem = GetStageZeroSubsystem(WorldContextObject))
    {
        Subsystem->SetMinimumDisplaySeconds(Seconds, bShowDebug);
        MaybeShowDebugMessage(WorldContextObject, bShowDebug, FString::Printf(TEXT("StageZero: Minimum display seconds set to %.2f"), Seconds));
    }
}

void UStageZeroBlueprintLibrary::SetStageZeroTravelMinimumDisplaySeconds(const UObject* WorldContextObject, float Seconds, bool bShowDebug)
{
    if (UStageZeroSubsystem* Subsystem = GetStageZeroSubsystem(WorldContextObject))
    {
        Subsystem->SetTravelMinimumDisplaySeconds(Seconds, bShowDebug);
        MaybeShowDebugMessage(WorldContextObject, bShowDebug, FString::Printf(TEXT("StageZero: Travel minimum display seconds set to %.2f"), Seconds));
    }
}

void UStageZeroBlueprintLibrary::SetStageZeroDelayAfterComplete(const UObject* WorldContextObject, float Seconds, bool bShowDebug)
{
    if (UStageZeroSubsystem* Subsystem = GetStageZeroSubsystem(WorldContextObject))
    {
        Subsystem->SetDelayAfterComplete(Seconds, bShowDebug);
        MaybeShowDebugMessage(WorldContextObject, bShowDebug, FString::Printf(TEXT("StageZero: Delay after complete set to %.2f"), Seconds));
    }
}

bool UStageZeroBlueprintLibrary::TransitionStageZeroPrimaryLevel(const UObject* WorldContextObject, bool bUseConfiguredDelay, bool bShowDebug)
{
    UStageZeroSubsystem* Subsystem = GetStageZeroSubsystem(WorldContextObject);
    if (!Subsystem)
    {
        MaybeShowDebugMessage(WorldContextObject, bShowDebug, TEXT("StageZero: Transition failed (no subsystem)"));
        return false;
    }

    const bool bResult = Subsystem->TransitionToPrimaryLevel(bUseConfiguredDelay, bShowDebug);
    if (bResult)
    {
        MaybeShowDebugMessage(WorldContextObject, bShowDebug, TEXT("StageZero: Transitioning to primary level"));
    }
    return bResult;
}

bool UStageZeroBlueprintLibrary::OpenLevelWithStageZeroLoadingScreen(const UObject* WorldContextObject, FName LevelName, bool bAbsolute, FString Options, bool bShowDebug)
{
    UStageZeroSubsystem* Subsystem = GetStageZeroSubsystem(WorldContextObject);
    if (!Subsystem)
    {
        MaybeShowDebugMessage(WorldContextObject, bShowDebug, TEXT("StageZero: Open level failed (no subsystem)"));
        return false;
    }

    return Subsystem->OpenLevelWithLoadingScreen(LevelName, bAbsolute, Options, bShowDebug);
}

UUserWidget* UStageZeroBlueprintLibrary::RegisterStageZeroLoadingWidget(const UObject* WorldContextObject, TSubclassOf<UUserWidget> WidgetClass, bool bAddToViewport, int32 ZOrder, bool bHideAfterComplete, bool bShowDebug)
{
    if (!WorldContextObject || !WidgetClass)
    {
        MaybeShowDebugMessage(WorldContextObject, bShowDebug, TEXT("StageZero: Register widget failed (no context or class)"));
        return nullptr;
    }

    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!World)
    {
        MaybeShowDebugMessage(WorldContextObject, bShowDebug, TEXT("StageZero: Register widget failed (no world)"));
        return nullptr;
    }

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        PC = UGameplayStatics::GetPlayerController(World, 0);
    }

    if (!PC)
    {
        MaybeShowDebugMessage(WorldContextObject, bShowDebug, TEXT("StageZero: Register widget failed (no PlayerController)"));
        return nullptr;
    }

    UUserWidget* Widget = CreateWidget<UUserWidget>(PC, WidgetClass);
    if (!Widget)
    {
        MaybeShowDebugMessage(WorldContextObject, bShowDebug, TEXT("StageZero: Register widget failed (CreateWidget returned null)"));
        return nullptr;
    }

    if (bAddToViewport)
    {
        Widget->AddToViewport(ZOrder);
    }

    if (UStageZeroSubsystem* Subsystem = GetStageZeroSubsystem(WorldContextObject))
    {
        Subsystem->RegisterLoadingWidget(Widget, bHideAfterComplete, bShowDebug);
        MaybeShowDebugMessage(WorldContextObject, bShowDebug, FString::Printf(TEXT("StageZero: Widget %s registered"), *Widget->GetName()));
    }
    else
    {
        MaybeShowDebugMessage(WorldContextObject, bShowDebug, TEXT("StageZero: Register widget failed (no subsystem)"));
    }

    return Widget;
}
