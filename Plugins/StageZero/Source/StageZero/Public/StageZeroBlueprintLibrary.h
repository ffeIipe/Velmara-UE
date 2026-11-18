// Copyright (c) 2025 71 Studios. All rights reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Blueprint/UserWidget.h"
#include "StageZeroSubsystem.h"
#include "StageZeroBlueprintLibrary.generated.h"

class UStageZeroProjectSettings;
class UTexture2D;
class UStageZeroSubsystem;
class UUserWidget;

/**
 * Blueprint helpers for Stage Zero configuration and control
 */
UCLASS()
class STAGEZERO_API UStageZeroBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Runtime warmup overrides
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Settings", meta = (AdvancedDisplay = "bShowDebug", ToolTip = "Runtime override: warm all maps via the Stage Zero subsystem (does not edit project settings)."))
    static void SetWarmUpAllMaps(bool bEnabled, bool bShowDebug = false);

    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Settings", meta = (ToolTip = "Runtime override state for Warm Up All Maps."))
    static void GetWarmUpAllMaps(bool& bEnabled);

    /** Configure Level Instance inclusion for future editor manifests and the legacy read-only loaded-world collector. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Manifest Build Settings", meta = (AdvancedDisplay = "bShowDebug", ToolTip = "Controls Level Instance collection for future Stage Zero manifest builds and legacy inspection of levels already loaded in memory. It never loads or streams a level; rebuild the manifest after changing it."))
    static void SetWarmUpLevelInstances(bool bEnabled, bool bShowDebug = false);

    /** Read the shared Level Instance inclusion policy used by manifest authoring and legacy resident-world inspection. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Manifest Build Settings", meta = (ToolTip = "Returns whether future manifests and legacy read-only inspection include Level Instance content."))
    static void GetWarmUpLevelInstances(bool& bEnabled);

    /** Configure Packed Level Actor inclusion for future editor manifests and the legacy read-only loaded-world collector. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Manifest Build Settings", meta = (AdvancedDisplay = "bShowDebug", ToolTip = "Controls Packed Level Actor collection for future Stage Zero manifest builds and legacy inspection of actors already loaded in memory. It never loads or streams a level; rebuild the manifest after changing it."))
    static void SetWarmUpPackedLevelActors(bool bEnabled, bool bShowDebug = false);

    /** Read the shared Packed Level Actor inclusion policy used by manifest authoring and legacy resident-world inspection. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Manifest Build Settings", meta = (ToolTip = "Returns whether future manifests and legacy read-only inspection include Packed Level Actor content."))
    static void GetWarmUpPackedLevelActors(bool& bEnabled);

    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Settings", meta = (AdvancedDisplay = "bShowDebug"))
    static void SetLoadingIconTexture(const TSoftObjectPtr<UTexture2D>& Icon, bool bShowDebug = false);

    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Settings")
    static void GetLoadingIconTexture(TSoftObjectPtr<UTexture2D>& OutIcon);

    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Settings", meta = (AdvancedDisplay = "bShowDebug"))
    static void SetBackgroundSlides(const TArray<TSoftObjectPtr<UTexture2D>>& Slides, bool bShowDebug = false);

    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Settings")
    static void GetBackgroundSlides(TArray<TSoftObjectPtr<UTexture2D>>& OutSlides);

    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Settings", meta = (AdvancedDisplay = "bShowDebug"))
    static void SetBootTips(const TArray<FText>& Tips, bool bShowDebug = false);

    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Settings")
    static void GetBootTips(TArray<FText>& OutTips);

    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Settings", meta = (AdvancedDisplay = "bShowDebug"))
    static void SetBackgroundSlideInterval(float Interval, bool bShowDebug = false);

    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Settings", meta = (AdvancedDisplay = "bShowDebug"))
    static void SetBootTipInterval(float Interval, bool bShowDebug = false);

    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Settings", meta = (AdvancedDisplay = "bShowDebug"))
    static void SetSpinnerOnlyPhaseDuration(float Duration, bool bShowDebug = false);

    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Settings", meta = (AdvancedDisplay = "bShowDebug"))
    static void SetShowSpinnerDuringProgress(bool bShow, bool bShowDebug = false);

    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Settings", meta = (AdvancedDisplay = "bShowDebug"))
    static void SetLoopBackgroundSlides(bool bLoop, bool bShowDebug = false);

    // Map list management (runtime overrides)
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Settings", meta = (AdvancedDisplay = "bShowDebug", ToolTip = "Runtime override: clear the warmup map list on the Stage Zero subsystem."))
    static void ClearMapsToWarmUp(bool bShowDebug = false);

    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Settings", meta = (AdvancedDisplay = "bShowDebug", ToolTip = "Runtime override: add a map to the Stage Zero subsystem warmup list."))
    static void AddMapToWarmUp(const TSoftObjectPtr<UWorld>& Map, bool bShowDebug = false);

    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Settings", meta = (AdvancedDisplay = "bShowDebug", ToolTip = "Runtime override: set the warmup map list on the Stage Zero subsystem."))
    static void SetMapsToWarmUp(const TArray<TSoftObjectPtr<UWorld>>& Maps, bool bShowDebug = false);

    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Settings", meta = (ToolTip = "Runtime override list currently used by the Stage Zero subsystem."))
    static void GetMapsToWarmUp(TArray<TSoftObjectPtr<UWorld>>& OutMaps);

    // Runtime trigger
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Control", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "bShowDebug"))
    static bool TriggerStageZeroWarmUp(const UObject* WorldContextObject, bool bShowDebug = false);

    /** Deprecated compatibility wrapper. Cross-process warmup skipping was retired; this always starts normal current-process work. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Deprecated", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "bRequirePipelineCacheClear,bShowDebug", DeprecatedFunction, DeprecationMessage = "Cross-process warmup skipping was retired. Use Trigger Stage Zero Warm Up.", BlueprintInternalUseOnly = "true"))
    static EStageZeroWarmupStartResult TriggerStageZeroWarmUpIfNeeded(const UObject* WorldContextObject, bool bRequirePipelineCacheClear = true, bool bShowDebug = false);

    /** Cancel an in-flight Stage Zero warmup. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Control", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "bShowDebug"))
    static bool CancelStageZeroWarmUp(const UObject* WorldContextObject, bool bShowDebug = false);

    /** Access the Stage Zero Subsystem for the given world context. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero", meta = (WorldContext = "WorldContextObject"))
    static UStageZeroSubsystem* GetStageZeroSubsystem(const UObject* WorldContextObject);

    /** Poll warmup status snapshot for UI. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero", meta = (WorldContext = "WorldContextObject"))
    static FStageZeroWarmupStatus GetStageZeroWarmupStatus(const UObject* WorldContextObject);

    /** Deprecated compatibility wrapper. Always false because prior-process readiness cannot skip current-process work. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Deprecated", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "bRequirePipelineCacheClear,bShowDebug", DeprecatedFunction, DeprecationMessage = "Cross-process warmup skipping was retired.", BlueprintInternalUseOnly = "true"))
    static bool CanSkipStageZeroWarmup(const UObject* WorldContextObject, bool bRequirePipelineCacheClear = true, bool bShowDebug = false);

    /** Deprecated compatibility wrapper returning a retired-cache status. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Deprecated", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "bRequirePipelineCacheClear,bShowDebug", DeprecatedFunction, DeprecationMessage = "Cross-process warmup skipping was retired.", BlueprintInternalUseOnly = "true"))
    static FStageZeroWarmupCacheStatus GetStageZeroWarmupCacheStatus(const UObject* WorldContextObject, bool bRequirePipelineCacheClear = true, bool bShowDebug = false);

    /** Get current map list being used by the subsystem. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero", meta = (WorldContext = "WorldContextObject"))
    static void GetStageZeroWarmupMaps(const UObject* WorldContextObject, TArray<TSoftObjectPtr<UWorld>>& OutMaps);

    /** Get the current primary level configured for transition. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero", meta = (WorldContext = "WorldContextObject"))
    static TSoftObjectPtr<UWorld> GetStageZeroPrimaryLevel(const UObject* WorldContextObject);

    /** Convert warmup completed/total into a percentage (0..100). */
    UFUNCTION(BlueprintPure, Category = "Stage Zero|Warmup", meta = (DisplayName = "Normalize Warmup Progress", ToolTip = "Convert warmup completed/total into a percentage (0..100)."))
    static float NormalizeWarmupProgress(int32 CompletedTasks, int32 TotalTasks);

    /** Get total and completed map counts. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero", meta = (WorldContext = "WorldContextObject"))
    static void GetStageZeroMapCounts(const UObject* WorldContextObject, int32& OutCompleted, int32& OutTotal);

    /** Get the active boot screen configuration. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero", meta = (WorldContext = "WorldContextObject"))
    static FStageZeroBootViewConfig GetStageZeroBootConfig(const UObject* WorldContextObject);

    /** Override the boot screen configuration at runtime. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "bShowDebug"))
    static void SetStageZeroBootConfig(const UObject* WorldContextObject, const FStageZeroBootViewConfig& InConfig, bool bShowDebug = false);

    /** Runtime toggle for auto transition. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "bShowDebug"))
    static void SetStageZeroAutoTransition(const UObject* WorldContextObject, bool bEnable, bool bShowDebug = false);

    /** Runtime toggle for auto widget spawn. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "bShowDebug"))
    static void SetStageZeroAutoWidget(const UObject* WorldContextObject, bool bEnable, bool bShowDebug = false);

    /** Runtime toggle for creating a player if missing. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "bShowDebug"))
    static void SetStageZeroAllowCreatePlayer(const UObject* WorldContextObject, bool bEnable, bool bShowDebug = false);

    /** Runtime configuration for minimum display seconds. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "bShowDebug"))
    static void SetStageZeroMinimumDisplaySeconds(const UObject* WorldContextObject, float Seconds, bool bShowDebug = false);

    /** Runtime configuration for between-level travel loading screen minimum display seconds. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Loading Screen", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "bShowDebug"))
    static void SetStageZeroTravelMinimumDisplaySeconds(const UObject* WorldContextObject, float Seconds, bool bShowDebug = false);

    /** Runtime configuration for delay after complete. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "bShowDebug"))
    static void SetStageZeroDelayAfterComplete(const UObject* WorldContextObject, float Seconds, bool bShowDebug = false);

    /** Manually transition to the configured primary level after warmup (for lobby flows). */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Control", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "bShowDebug"))
    static bool TransitionStageZeroPrimaryLevel(const UObject* WorldContextObject, bool bUseConfiguredDelay = true, bool bShowDebug = false);

    /** Open a level using Stage Zero's configured map-travel loading screen. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Loading Screen", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "bAbsolute, Options, bShowDebug"))
    static bool OpenLevelWithStageZeroLoadingScreen(const UObject* WorldContextObject, FName LevelName, bool bAbsolute = true, FString Options = TEXT(""), bool bShowDebug = false);

    /** Spawn a widget, optionally add to viewport, and register it with the subsystem (class picker friendly). */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|UI", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "bAddToViewport, ZOrder, bShowDebug"))
    static UUserWidget* RegisterStageZeroLoadingWidget(const UObject* WorldContextObject, TSubclassOf<UUserWidget> WidgetClass, bool bAddToViewport = true, int32 ZOrder = 0, bool bHideAfterComplete = false, bool bShowDebug = false);

    /** Override boot background slides at runtime (applies immediately if the loading widget is active). */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|UI", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "bLoopSlides, bShowDebug"))
    static void SetStageZeroBackgroundSlides(const UObject* WorldContextObject, const TArray<TSoftObjectPtr<UTexture2D>>& Slides, bool bLoopSlides = true, bool bShowDebug = false);

private:
    static UStageZeroProjectSettings* GetMutableSettings();
    static const UStageZeroProjectSettings* GetSettings();
    static void MaybeShowDebugMessage(const UObject* WorldContextObject, bool bShowDebug, const FString& Message);
};
