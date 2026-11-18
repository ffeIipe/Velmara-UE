// Copyright (c) 2025 71 Studios for games Development. All rights reserved. 
#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPtr.h"
#include "GameFramework/Actor.h"
#include "Delegates/DelegateCombinations.h"
#include "StageZeroLoader.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStageZeroEvent);

class UStageZeroLoadingWidget;
class USoundBase;
class UAudioComponent;
class UTexture2D;
class UStageZeroSubsystem;
class UWorld;

UCLASS(Blueprintable, meta = (DisplayName = "Stage Zero Loader (Legacy)", DeprecatedNode, DeprecationMessage = "Use the Stage Zero Game Instance Subsystem for new integrations."))
class STAGEZERO_API AStageZeroLoader : public AActor
{
    GENERATED_BODY()

public:
    AStageZeroLoader();
    virtual ~AStageZeroLoader() = default;

    /** Set target levels dynamically before starting StageZero */
    UFUNCTION(BlueprintCallable, Category = "StageZero|Levels", meta = (ToolTip = "Set all target levels at once. Primary is opened after warmup; WarmupMaps are prep-only.", DeprecatedFunction, DeprecationMessage = "Use Stage Zero Subsystem runtime overrides instead."))
    void SetLevelTargets(TSoftObjectPtr<UWorld> InPrimary, const TArray<TSoftObjectPtr<UWorld>>& InWarmupMaps);

    /** Manually start StageZero (use when AutoStart is disabled) */
    UFUNCTION(BlueprintCallable, Category = "StageZero|Control", meta = (DeprecatedFunction, DeprecationMessage = "Call Stage Zero Subsystem StartWarmup instead."))
    void StartStageZero();

    /** Set primary level */
    UFUNCTION(BlueprintCallable, Category = "StageZero|Levels", meta = (ToolTip = "Set the primary (destination) level. This is the level that will open after warmup.", DeprecatedFunction, DeprecationMessage = "Use Stage Zero Subsystem SetPrimaryLevel instead."))
    void SetPrimaryLevelBP(TSoftObjectPtr<UWorld> InPrimary);

    /** Set warmup maps (array) */
    UFUNCTION(BlueprintCallable, Category = "StageZero|Levels", meta = (ToolTip = "Set the list of maps to warm (prep-only, not opened).", DeprecatedFunction, DeprecationMessage = "Use Stage Zero Subsystem SetWarmupMaps instead."))
    void SetWarmupMapsBP(const TArray<TSoftObjectPtr<UWorld>>& InWarmupMaps);

    /** Add a single warmup map */
    UFUNCTION(BlueprintCallable, Category = "StageZero|Levels", meta = (DeprecatedFunction, DeprecationMessage = "Use Stage Zero Subsystem AddWarmupMap instead."))
    void AddWarmupMapBP(TSoftObjectPtr<UWorld> InWarmupMap);

    /** Clear warmup maps */
    UFUNCTION(BlueprintCallable, Category = "StageZero|Levels", meta = (DeprecatedFunction, DeprecationMessage = "Use Stage Zero Subsystem ClearWarmupMaps instead."))
    void ClearWarmupMapsBP();

    /** Set World Partition warmup (load all cells) */
    UFUNCTION(BlueprintCallable, Category = "StageZero|WarmUp", meta = (DeprecatedFunction, DeprecationMessage = "World Partition warmup is handled internally by the Stage Zero Subsystem."))
    void SetWorldPartitionWarmAllCells(bool bEnable);

    /** Set minimum display seconds for UI */
    UFUNCTION(BlueprintCallable, Category = "StageZero|UI", meta = (DeprecatedFunction, DeprecationMessage = "Configure boot timing on Stage Zero Subsystem instead."))
    void SetMinimumDisplaySeconds(float InSeconds);

    /** Create/loading widget manually (if auto UI is disabled) */
    UFUNCTION(BlueprintCallable, Category = "StageZero|UI", meta = (DeprecatedFunction, DeprecationMessage = "Widgets are managed by Stage Zero Subsystem."))
    void CreateLoadingWidget();

    /** Get progress (0-1) */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "StageZero|Progress", meta = (DeprecatedFunction, DeprecationMessage = "Use Stage Zero Subsystem progress queries instead."))
    float GetStageZeroProgress() const;

    /** Get progress counts (Completed, Total) */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "StageZero|Progress", meta = (DeprecatedFunction, DeprecationMessage = "Use Stage Zero Subsystem progress queries instead."))
    void GetStageZeroProgressCounts(int32& OutCompleted, int32& OutTotal) const;

    /** 0-1 progress for accepted recorded pipeline-cache work only. Asset preload progress is intentionally excluded. */
    UFUNCTION(BlueprintPure, Category = "StageZero|Progress", meta = (DeprecatedFunction, DeprecationMessage = "Use Stage Zero Subsystem progress queries instead."))
    float GetShaderProgress() const;
    
    /** 0-1 combined progress including warmup */
    UFUNCTION(BlueprintPure, Category = "StageZero|Progress", meta = (DeprecatedFunction, DeprecationMessage = "Use Stage Zero Subsystem progress queries instead."))
    float GetTotalProgress() const;

    /** Force immediate transition to primary level (debug only) */
    UFUNCTION(BlueprintCallable, Category = "StageZero|Debug", meta = (DeprecatedFunction, DeprecationMessage = "Use Stage Zero Subsystem warmup cancellation instead."))
    void ForceTransitionNow();

    /** Events */
    UPROPERTY(BlueprintAssignable, Category = "StageZero|Events")
    FStageZeroEvent OnStageZeroStarted;

    UPROPERTY(BlueprintAssignable, Category = "StageZero|Events")
    FStageZeroEvent OnStageZeroCompleted;

    // Level Configuration
    UPROPERTY(EditAnywhere, Category = "StageZero|Levels", meta = (ToolTip = "Main level opened after manifest asset preload and the recorded PSO-cache stage."))
    TSoftObjectPtr<UWorld> PrimaryLevel;

    UPROPERTY(EditAnywhere, Category = "StageZero|WarmUp", meta = (ToolTip = "Manifest map entries to preload. Stage Zero does not stream or render these maps at runtime."))
    TArray<TSoftObjectPtr<UWorld>> MapsToWarmUp;

    /** Serialized legacy flag. The subsystem's authored manifest is now the only asset preload plan. */
    UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Dependency loading is owned by the Stage Zero subsystem manifest executor."))
    bool bPreloadDependencies = true;

    // Boot Screen
    /** Optional loading spinner texture (leave empty to hide) */
    UPROPERTY(EditAnywhere, Category = "StageZero|Boot Screen")
    TSoftObjectPtr<UTexture2D> LoadingIconTexture;

    /** Background slideshow images shown during boot */
    UPROPERTY(EditAnywhere, Category = "StageZero|Boot Screen")
    TArray<TSoftObjectPtr<UTexture2D>> BackgroundSlides;

    /** Time in seconds between background slide changes */
    UPROPERTY(EditAnywhere, Category = "StageZero|Boot Screen", meta = (ClampMin = "0.1"))
    float BackgroundSlideInterval = 2.0f;

    /** Fade duration (seconds) when changing background slides */
    UPROPERTY(EditAnywhere, Category = "StageZero|Boot Screen", meta = (ClampMin = "0.0"))
    float BackgroundSlideFadeDuration = 0.25f;

    /** Whether to loop background slides */
    UPROPERTY(EditAnywhere, Category = "StageZero|Boot Screen")
    bool bLoopBackgroundSlides = true;

    /** Rotating tips shown during boot */
    UPROPERTY(EditAnywhere, Category = "StageZero|Boot Screen")
    TArray<FText> BootTips;

    /** Time in seconds between tip changes */
    UPROPERTY(EditAnywhere, Category = "StageZero|Boot Screen", meta = (ClampMin = "0.1"))
    float BootTipInterval = 3.0f;

    /** Minimum spinner-only phase duration before revealing progress bar */
    UPROPERTY(EditAnywhere, Category = "StageZero|Boot Screen", meta = (ClampMin = "0.0"))
    float SpinnerOnlyPhaseDuration = 0.3f;

    /** Keep spinner visible while progress is shown */
    UPROPERTY(EditAnywhere, Category = "StageZero|Boot Screen")
    bool bShowSpinnerDuringProgress = true;

    /** Max number of maps to queue per batch when warming all maps (keeps game thread responsive) */
    UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Legacy actor map batching was retired."))
    int32 MaxMapsPerWarmupBatch = 2;

    /** Delay between map queue batches (seconds) */
    UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Legacy actor map batching was retired."))
    float WarmupBatchDelaySeconds = 0.25f;

    /** Warm up the internal worlds of Level Instances when preparing maps */
    UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Runtime level traversal was retired for engine safety."))
    bool bWarmUpLevelInstances = true;

    /** Warm up baked content stored inside Packed Level Actors */
    UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Runtime level traversal was retired for engine safety."))
    bool bWarmUpPackedLevelActors = true;

    /** Discover all map assets in the project and warm them, ignoring the manual warm-up list */
    UPROPERTY(EditAnywhere, Category = "StageZero|Maps", meta = (ToolTip = "If enabled, Stage Zero automatically discovers and warms up all map assets in the project, ignoring the manually configured 'Maps To Warm Up' list."))
    bool bWarmUpAllMaps = false;

    // World Partition warmup settings
    UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Runtime World Partition streaming warmup was retired for engine safety."))
    bool bEnableWorldPartitionWarmup = true;

    UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Runtime World Partition streaming warmup was retired for engine safety."))
    bool bWarmAllWorldPartitionCells = true;

    UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Runtime World Partition streaming warmup was retired for engine safety."))
    float WarmupStreamingRadius = 200000.0f;

    UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Runtime World Partition streaming warmup was retired for engine safety."))
    int32 WarmupStreamingPasses = 1;

    UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Runtime World Partition streaming warmup was retired for engine safety."))
    float WarmupStreamingPassMaxSeconds = 10.0f;

    UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Runtime World Partition streaming warmup was retired for engine safety."))
    int32 WarmupStreamingMemoryBudgetMB = 2048;

    // Control
    UPROPERTY(EditAnywhere, Category = "StageZero|Control", meta = (ToolTip = "Start StageZero automatically in BeginPlay. Disable to start manually via StartStageZero()."))
    bool bAutoStartStageZero = true;
    
    UPROPERTY(EditAnywhere, Category = "StageZero|UI", meta = (ToolTip = "Automatically create the loading widget in BeginPlay/StartStageZero. Disable to create it manually via CreateLoadingWidget()."))
    bool bAutoCreateLoadingWidget = true;

    // UI Configuration
    UPROPERTY(EditAnywhere, Category = "StageZero|UI", meta = (ToolTip = "Widget class to show loading progress"))
    TSubclassOf<UStageZeroLoadingWidget> LoadingWidgetClass;

    UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "The subsystem owns event-driven UI updates."))
    float UIUpdateInterval = 0.1f;

    UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Legacy actor completion messaging was retired."))
    bool bShowCompletionMessage = false;

    /** Optional: allow StageZero to spawn a local player if none exists when creating the loading widget (Standalone only). Defaults to false. */
    UPROPERTY(EditAnywhere, Category = "StageZero|UI")
    bool bAllowCreatePlayerIfMissing = false;

    UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Legacy actor completion messaging was retired."))
    FString CompletionMessage = TEXT("Loading complete");

    /** Hidden serialized compatibility switch. Existing legacy actors can still request their authored transition event. */
    UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Legacy serialized compatibility only; existing values are honored without exposing this option for new authoring."))
    bool bShowTransitionMessage = false;

    /** Hidden serialized compatibility text used only when bShowTransitionMessage was already enabled on a legacy actor. */
    UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Legacy serialized compatibility only; existing values are honored without exposing this option for new authoring."))
    FString TransitionMessage = TEXT("Preparing level...");

    // Audio Configuration
    UPROPERTY(EditAnywhere, Category = "StageZero|Audio", meta = (ToolTip = "Music to play during loading"))
    USoundBase* BackgroundMusic = nullptr;

    UPROPERTY(EditAnywhere, Category = "StageZero|Audio")
    float MusicFadeInTime = 1.f;

    UPROPERTY(EditAnywhere, Category = "StageZero|Audio")
    float MusicFadeOutTime = 1.f;

    // Timing Configuration
    UPROPERTY(EditAnywhere, Category = "StageZero|Timing", meta = (ToolTip = "Minimum time to show loading screen"))
    float MinimumDisplaySeconds = 3.f;

    UPROPERTY(EditAnywhere, Category = "StageZero|Timing", meta = (ToolTip = "Delay after Stage Zero reaches its terminal result before opening the primary level."))
    float DelayAfterComplete = 1.f;

    UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use the subsystem asset-load and pipeline-cache deadlines."))
    float MaxWaitTimeSeconds = 60.0f;

    // Debug Configuration
    UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Simulated progress was retired."))
    bool bProgressOverride = false;

    UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Simulated progress was retired."))
    float SimulatedProgressSpeed = 5.f;

    UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Global shader-counter gating was retired."))
    bool bSkipShaderCheck = false;

    UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Legacy actor build-info rendering was retired."))
    bool bShowBuildInfo = false;

    UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Simulated progress was retired."))
    bool bFallbackToSimulatedProgress = true;

    UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use normal Stage Zero logging and status delegates."))
    bool bShowDebugProgressOnScreen = false;
    
    UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Runtime forced shader recompilation was retired for engine safety."))
    bool bForceShaderRecompilation = false;

    // GPU Warmup Configuration
    UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Runtime GPU/material rendering warmup was retired for engine safety."))
    bool bEnableGPUWarmup = true;
    
    UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use the authored Stage Zero manifest."))
    bool bAutoCollectWarmupAssets = true;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    friend class UStageZeroBlueprintLibrary;
    friend class UStageZeroSubsystem;

    /**
     * This actor is retained only as a serialized/Blueprint compatibility facade.
     * Runtime warmup authority belongs exclusively to UStageZeroSubsystem.
     */
    void InitializeBackgroundMusic();
    void StopBackgroundMusic(bool bImmediate);
    void SynchronizeWithSubsystemState(UStageZeroSubsystem* Subsystem);
    bool ShouldRunOnThisNetMode() const;
    void ApplyProjectSettings();

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> MusicComponent = nullptr;

    TWeakObjectPtr<UStageZeroSubsystem> BoundSubsystem;
    bool bObservedSubsystemWarmup = false;
    bool bProjectSettingsApplied = false;

    // Subsystem bindings
    UFUNCTION()
    void HandleSubsystemStarted();

    UFUNCTION()
    void HandleSubsystemFinished(bool bWasCancelled);

    UStageZeroSubsystem* GetStageZeroSubsystem() const;
    void BindSubsystemDelegates(UStageZeroSubsystem* Subsystem);
    void UnbindSubsystemDelegates();
};
