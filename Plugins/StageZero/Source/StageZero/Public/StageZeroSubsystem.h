// Copyright (c) 2025 71 Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Engine/StreamableManager.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TimerManager.h"
#include "UObject/SoftObjectPath.h"
#include "StageZeroSubsystem.generated.h"

class AStageZeroLoader;
class UStageZeroLoadingWidget;
class UStageZeroProjectSettings;
class UStageZeroWarmupManifest;
class UStageZeroNativeLoadingScreenProvider;
struct FStageZeroManifestJob;
struct FStageZeroManifestMapCoverage;
struct FStageZeroLoadingScreenProfile;
class FStageZeroWorldPartitionManifestSelectionTest;
class FStageZeroPerMapLifecycleAccountingTest;

class UTexture2D;
class UUserWidget;
class USoundBase;
class UAudioComponent;
struct FWorldContext;

UENUM()
enum class EStageZeroManifestJobType : uint8
{
    Material,
    Texture,
    Niagara,
    StaticMesh,
    SkeletalMesh,
    PSO
};

UENUM(BlueprintType)
enum class EStageZeroWarmupResult : uint8
{
    NotStarted,
    Running,
    Succeeded,
    Failed,
    Cancelled,
    TimedOut,
    Partial
};

UENUM(BlueprintType)
enum class EStageZeroWarmupJobTerminalState : uint8
{
    Pending,
    Loading,
    Succeeded,
    Failed,
    TimedOut,
    Cancelled,
    SkippedPolicy
};

UENUM(BlueprintType)
enum class EStageZeroWarmupDomain : uint8
{
    ManifestAssets,
    PipelineCache,
    MaterialRenderWarmup UMETA(Hidden),
    RuntimeDiscovered UMETA(Hidden),
    FailedSkipped
};

UENUM(BlueprintType)
enum class EStageZeroWarmupStartResult : uint8
{
    Started,
    SkippedCached UMETA(Hidden),
    AlreadyRunning,
    Failed
};

struct FStageZeroManifestJob
{
    EStageZeroManifestJobType Type = EStageZeroManifestJobType::Material;
    FSoftObjectPath Path;
};

/**
 * Logical per-map membership layered over the aggregate manifest job queue.
 *
 * This is bookkeeping only: it never loads a world, traverses actors, or owns
 * a separate asset request. Shared paths remain one physical manifest job.
 */
struct FStageZeroManifestMapCoverage
{
    FName MapName = NAME_None;
    TSet<FSoftObjectPath> RequiredPaths;
    TSet<FSoftObjectPath> ProcessedTerminalPaths;
    bool bStarted = false;
    bool bFinished = false;
};

USTRUCT(BlueprintType)
struct STAGEZERO_API FStageZeroWarmupJobInfo
{
    GENERATED_BODY()

    /** Display name of the asset being warmed by Stage Zero. */
    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Debug")
    FName AssetName = NAME_None;

    /** Soft reference to the asset warmed by Stage Zero. */
    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Debug")
    FSoftObjectPath AssetPath;

    /** Type of the asset (Material, Texture, Niagara, etc). */
    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Debug")
    FString AssetType;

    /** True if this asset was found in the Stage Zero manifest. */
    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Debug")
    bool bInManifest = false;

    /** True if Stage Zero has requested warmup for this asset. */
    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Debug")
    bool bWarmupRequested = false;

    /** True if Stage Zero has finished its own warmup work for this asset. */
    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Debug")
    bool bWarmupFinished = false;

    /** Authoritative per-run state for this manifest item. */
    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Debug")
    EStageZeroWarmupJobTerminalState TerminalState = EStageZeroWarmupJobTerminalState::Pending;

    /** Empty on success; otherwise explains the terminal failure, timeout, cancellation, or policy skip. */
    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Debug")
    FString TerminalReason;
};

USTRUCT(BlueprintType)
struct STAGEZERO_API FStageZeroWarmupDomainStatus
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup")
    EStageZeroWarmupDomain Domain = EStageZeroWarmupDomain::ManifestAssets;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup")
    int32 Completed = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup")
    int32 Total = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup")
    int32 Remaining = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup")
    int32 Failed = 0;
};

USTRUCT(BlueprintType)
struct STAGEZERO_API FStageZeroWarmupStatus
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup")
    EStageZeroWarmupResult Result = EStageZeroWarmupResult::NotStarted;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup")
    float Progress = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup")
    FName CurrentMap = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup")
    int32 CompletedTasks = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup")
    int32 TotalTasks = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup")
    int32 CurrentMapCompleted = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup")
    int32 CurrentMapTotal = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup")
    int32 CompletedMaps = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup")
    int32 TotalMaps = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup")
    bool bStrictCompletion = true;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup")
    bool bPipelineTimedOut = false;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup")
    bool bAssetLoadFailuresFatal = true;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup")
    int32 ManifestAssetsCompleted = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup")
    int32 ManifestAssetsTotal = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup")
    int32 PipelineCacheCompleted = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup")
    int32 PipelineCacheTotal = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup")
    int32 PipelineCacheRemaining = 0;

    /** True when this run was configured to use Unreal's recorded pipeline cache. */
    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup")
    bool bPipelineCacheRequested = false;

    /** True only when Stage Zero opened or observed a recorded pipeline cache for this run. */
    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup")
    bool bPipelineCacheAccepted = false;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup", meta = (DeprecatedProperty, DeprecationMessage = "Runtime-discovered heuristic work was retired. Use Domains for ManifestAssets and PipelineCache."))
    int32 RuntimeDiscoveredCompleted = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup", meta = (DeprecatedProperty, DeprecationMessage = "Runtime-discovered heuristic work was retired. Use Domains for ManifestAssets and PipelineCache."))
    int32 RuntimeDiscoveredTotal = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup", meta = (DeprecatedProperty, DeprecationMessage = "Runtime-discovered heuristic work was retired. Use Domains for ManifestAssets and PipelineCache."))
    int32 RuntimeDiscoveredRemaining = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup", meta = (DeprecatedProperty, DeprecationMessage = "Runtime material rendering was retired. Stage Zero never renders assets to warm them."))
    int32 MaterialRenderWarmupCompleted = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup", meta = (DeprecatedProperty, DeprecationMessage = "Runtime material rendering was retired. Stage Zero never renders assets to warm them."))
    int32 MaterialRenderWarmupTotal = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup")
    int32 FailedSkippedTotal = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup")
    TArray<FString> FailedAssets;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup")
    TArray<FStageZeroWarmupDomainStatus> Domains;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup")
    FString StatusMessage;
};

USTRUCT(BlueprintType)
struct STAGEZERO_API FStageZeroWarmupCacheStatus
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup Cache")
    bool bCacheEnabled = false;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup Cache")
    bool bCacheLoaded = false;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup Cache")
    bool bCacheIdentityMatches = false;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup Cache")
    bool bManifestAvailable = false;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup Cache")
    bool bAllManifestAssetsCached = false;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup Cache")
    bool bPipelineCacheClear = true;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup Cache")
    bool bCanSkipWarmup = false;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup Cache")
    int32 RequiredAssets = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup Cache")
    int32 CachedAssets = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup Cache")
    int32 MissingAssets = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup Cache")
    int32 InvalidManifestAssets = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup Cache")
    int32 RemainingPSOs = 0;

    /** Hashed identity for diagnostics. This never contains asset names or object paths. */
    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup Cache")
    FString CacheIdentityHash;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Warmup Cache")
    FString Reason;
};

USTRUCT(BlueprintType)
struct STAGEZERO_API FStageZeroBootViewConfig
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Boot Screen")
    TSoftClassPtr<UUserWidget> WidgetClass;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Boot Screen")
    TSoftObjectPtr<UTexture2D> SpinnerIcon;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Boot Screen")
    TArray<TSoftObjectPtr<UTexture2D>> BackgroundSlides;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Boot Screen")
    FString BootMoviePath;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Boot Screen")
    bool bLoopBootMovie = true;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Boot Screen")
    bool bBootMovieSkippable = true;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Boot Screen")
    float BootMovieSkipDelaySeconds = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Boot Screen")
    FText BootMovieSkipText;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Boot Screen")
    bool bRestoreSlidesAfterBootMovie = true;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Boot Screen")
    bool bPlayBootMovieAudio = false;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Boot Screen")
    float BootMovieVolume = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Boot Screen")
    float SlideInterval = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Boot Screen")
    float SlideFadeDuration = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Boot Screen")
    bool bLoopSlides = true;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Boot Screen")
    TArray<FText> Tips;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Boot Screen")
    float TipInterval = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Boot Screen")
    TSoftObjectPtr<USoundBase> BackgroundMusic;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Boot Screen")
    float MusicFadeInTime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Boot Screen")
    float MusicFadeOutTime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Boot Screen")
    float SpinnerOnlyDuration = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Stage Zero|Boot Screen")
    bool bShowSpinnerDuringProgress = true;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStageZeroWarmupStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FStageZeroWarmupProgress, float, Progress, FName, CurrentMap, int32, CompletedTasks, int32, TotalTasks, int32, CurrentMapCompleted, int32, CurrentMapTotal);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStageZeroWarmupFinished, bool, bWasCancelled);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStageZeroWarmupStatusChanged, FStageZeroWarmupStatus, Status);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FStageZeroWarmupFinishedDetailed, EStageZeroWarmupResult, Result, FStageZeroWarmupStatus, Status);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FStageZeroWarmupMapEvent, FName, MapName, int32, MapIndex, int32, TotalMaps);

/**
 * Central runtime manager for Stage Zero warmup.
 * Owns map selection, warmup execution, boot UI orchestration, and progress delegates.
 */
UCLASS(Config=Game)
class STAGEZERO_API UStageZeroSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UStageZeroSubsystem();

    // UGameInstanceSubsystem
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** Start the Stage Zero warmup process using current settings/overrides. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Warmup")
    void StartWarmup(bool bShowDebug = false);

    /** Compatibility wrapper. Cross-process cache skipping was removed; this now starts a normal warmup. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Warmup", meta = (BlueprintInternalUseOnly = "true", AdvancedDisplay = "bRequirePipelineCacheClear,bShowDebug", DeprecatedFunction, DeprecationMessage = "Cross-process warmup skipping was removed. Use Start Warmup."))
    EStageZeroWarmupStartResult StartWarmupIfNeeded(bool bRequirePipelineCacheClear = true, bool bShowDebug = false);

    /** Cancel any in-flight warmup work and tear down the boot UI. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Warmup")
    void CancelWarmup(bool bShowDebug = false);

    /** True while the subsystem is actively running warmup work. */
    UFUNCTION(BlueprintPure, Category = "Stage Zero|Warmup")
    bool IsWarmupRunning(bool bShowDebug = false) const;

    /** True once warmup has completed successfully. */
    UFUNCTION(BlueprintPure, Category = "Stage Zero|Warmup")
    bool IsWarmupCompleted(bool bShowDebug = false) const;

    /** Warmup progress in range [0..1]. */
    UFUNCTION(BlueprintPure, Category = "Stage Zero|Warmup")
    float GetWarmupProgress(bool bShowDebug = false) const;

    /** Current manifest selection label. Multi-map aggregate loading reports AllMaps. */
    UFUNCTION(BlueprintPure, Category = "Stage Zero|Warmup")
    FName GetCurrentMapName(bool bShowDebug = false) const;

    /** Total vs completed task counts (aggregated across maps). */
    UFUNCTION(BlueprintPure, Category = "Stage Zero|Warmup")
    void GetTaskCounts(int32& OutCompleted, int32& OutTotal, bool bShowDebug = false) const;

    /** Total number of maps queued for warmup. */
    UFUNCTION(BlueprintPure, Category = "Stage Zero|Warmup")
    int32 GetTotalMaps(bool bShowDebug = false) const;

    /** Number of maps completed so far. */
    UFUNCTION(BlueprintPure, Category = "Stage Zero|Warmup")
    int32 GetCompletedMaps(bool bShowDebug = false) const;

    /** Copy of the current warmup map list. */
    UFUNCTION(BlueprintPure, Category = "Stage Zero|Warmup")
    TArray<TSoftObjectPtr<UWorld>> GetWarmupMaps(bool bShowDebug = false) const;

    /** The primary level that will be opened after warmup (if set). */
    UFUNCTION(BlueprintPure, Category = "Stage Zero|Warmup")
    TSoftObjectPtr<UWorld> GetPrimaryLevel(bool bShowDebug = false) const;

    /** Warm up all materials and textures referenced by any map (manifest-driven). */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Warmup")
    void WarmUpAllMaps(bool bShowDebug = false);

    /** Warm up materials and textures for a specific map asset (manifest-driven). */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Warmup", meta = (DisplayName = "Warm Up Map"))
    void WarmUpMap(TSoftObjectPtr<UWorld> MapAsset, bool bShowDebug = false);

    /** Warm up materials and textures for a specific map package/object path (manifest-driven). String path overload (internal use). */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Warmup", meta = (BlueprintInternalUseOnly = "true"))
    void WarmUpMapByPath(const FString& MapPackageOrObjectPath, bool bShowDebug = false);

    /** Warm all baked World Partition cell assets for a map. No runtime world traversal, streaming, or rendering occurs. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Warmup|WorldPartition")
    void WarmUpWorldPartitionCells(const FString& MapPackageOrObjectPath, bool bShowDebug = false);

    /** Warm baked anchor-group assets for a map. Names match GroupName or AnchorId; an empty list selects all anchors for the map. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Warmup|WorldPartition")
    void WarmUpWorldPartitionAnchors(const FString& MapPackageOrObjectPath, const TArray<FName>& AnchorNames, bool bShowDebug = false);

    /** Warm baked spline-route assets for a map. Names match GroupName or RouteId; an empty list selects all routes for the map. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Warmup|WorldPartition")
    void WarmUpWorldPartitionSplineRoutes(const FString& MapPackageOrObjectPath, const TArray<FName>& RouteNames, bool bShowDebug = false);

    /** True if Warm Up All Maps is enabled for this run. */
    UFUNCTION(BlueprintPure, Category = "Stage Zero|Warmup")
    bool IsWarmUpAllMapsEnabled(bool bShowDebug = false) const;

    /** Deprecated compatibility query; runtime traversal is permanently disabled and this always returns false. */
    UFUNCTION(BlueprintPure, Category = "Stage Zero|Deprecated", meta = (BlueprintInternalUseOnly = "true", DeprecatedFunction, DeprecationMessage = "Runtime traversal was removed. Configure Level Instance inclusion when building the manifest."))
    bool IncludesLevelInstances(bool bShowDebug = false) const;

    /** Deprecated compatibility query; runtime traversal is permanently disabled and this always returns false. */
    UFUNCTION(BlueprintPure, Category = "Stage Zero|Deprecated", meta = (BlueprintInternalUseOnly = "true", DeprecatedFunction, DeprecationMessage = "Runtime traversal was removed. Configure Packed Level Actor inclusion when building the manifest."))
    bool IncludesPackedLevelActors(bool bShowDebug = false) const;

    /** True if the subsystem will auto-spawn the built-in widget. */
    UFUNCTION(BlueprintPure, Category = "Stage Zero|UI")
    bool IsWidgetAutoSpawnEnabled(bool bShowDebug = false) const;

    /** Aggregate warmup status snapshot for polling UIs. */
    UFUNCTION(BlueprintPure, Category = "Stage Zero|Warmup")
    FStageZeroWarmupStatus GetWarmupStatus(bool bShowDebug = false) const;

    /** Override the primary level to open after warmup finishes. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Warmup")
    void SetPrimaryLevel(TSoftObjectPtr<UWorld> InPrimaryLevel, bool bShowDebug = false);

    /** Override the warmup map list at runtime. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Warmup")
    void SetWarmupMaps(const TArray<TSoftObjectPtr<UWorld>>& InMaps, bool bShowDebug = false);

    /** Add a single map to the warmup queue. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Warmup")
    void AddWarmupMap(TSoftObjectPtr<UWorld> InMap, bool bShowDebug = false);

    /** Clear any runtime warmup map overrides. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Warmup")
    void ClearWarmupMaps(bool bShowDebug = false);

    /** Enable or disable Warm Up All Maps at runtime. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Warmup")
    void SetWarmUpAllMaps(bool bEnable, bool bShowDebug = false);

    /** Deprecated compatibility setter; runtime traversal is permanently disabled. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Deprecated", meta = (BlueprintInternalUseOnly = "true", DeprecatedFunction, DeprecationMessage = "Runtime traversal was removed. Configure Level Instance inclusion when building the manifest."))
    void SetWarmUpLevelInstances(bool bEnable, bool bShowDebug = false);

    /** Deprecated compatibility setter; runtime traversal is permanently disabled. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Deprecated", meta = (BlueprintInternalUseOnly = "true", DeprecatedFunction, DeprecationMessage = "Runtime traversal was removed. Configure Packed Level Actor inclusion when building the manifest."))
    void SetWarmUpPackedLevelActors(bool bEnable, bool bShowDebug = false);

    /** Configure whether the subsystem will open the primary level after completion. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Warmup")
    void SetAutoTransitionToPrimary(bool bEnable, bool bShowDebug = false);

    /** Configure whether the subsystem spawns the built-in widget. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|UI")
    void SetAutoSpawnWidget(bool bEnable, bool bShowDebug = false);

    /** Configure whether the subsystem may create a player if missing (standalone only). */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|UI")
    void SetAllowCreatePlayerIfMissing(bool bEnable, bool bShowDebug = false);

    /** Configure minimum display time for the boot screen (seconds). */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|UI")
    void SetMinimumDisplaySeconds(float InSeconds, bool bShowDebug = false);

    /** Configure minimum display time for between-level travel loading screens (seconds). */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Loading Screen")
    void SetTravelMinimumDisplaySeconds(float InSeconds, bool bShowDebug = false);

    /** Configure delay after completion before transitioning to the primary level (seconds). */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|UI")
    void SetDelayAfterComplete(float InSeconds, bool bShowDebug = false);

    /** Manually transition to the configured primary level (useful for lobby flows). */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Warmup")
    bool TransitionToPrimaryLevel(bool bUseConfiguredDelay = true, bool bShowDebug = false);

    /** Open a level while using Stage Zero's configured map-travel loading screen. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Loading Screen", meta = (AdvancedDisplay = "bAbsolute, Options, bShowDebug"))
    bool OpenLevelWithLoadingScreen(FName LevelName, bool bAbsolute = true, FString Options = TEXT(""), bool bShowDebug = false);

    /** Expose the boot screen configuration currently in effect. */
    UFUNCTION(BlueprintPure, Category = "Stage Zero|UI")
    FStageZeroBootViewConfig GetBootViewConfig(bool bShowDebug = false) const;

    /** Override the boot screen configuration at runtime. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|UI")
    void SetBootViewConfig(const FStageZeroBootViewConfig& InConfig, bool bShowDebug = false);

    /** Register a loading widget so the subsystem can drive its visuals. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|UI")
    void RegisterLoadingWidget(UUserWidget* Widget, bool bHideAfterComplete = false, bool bShowDebug = false);

    /** Unregister a loading widget (usually handled automatically on destruct). */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|UI")
    void UnregisterLoadingWidget(UUserWidget* Widget, bool bShowDebug = false);

    /** Copy legacy actor configuration into the subsystem for backward compatibility. */
    void ApplyLegacyActorSettings(AStageZeroLoader* Loader);

    /** Runtime override for background slides; applies immediately to the active widget if present. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|UI", meta = (AdvancedDisplay = "bLoopSlides, bShowDebug"))
    void SetBackgroundSlidesOverride(const TArray<TSoftObjectPtr<UTexture2D>>& Slides, bool bLoopSlides = true, bool bShowDebug = false);

    /** Deprecated compatibility API. Cross-process warmup skipping is permanently disabled. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Warmup", meta = (BlueprintInternalUseOnly = "true", DeprecatedFunction, DeprecationMessage = "Cross-process warmup skipping was removed."))
    void SetWarmupCacheEnabled(bool bEnable);

    /** Deprecated compatibility API; always returns false. */
    UFUNCTION(BlueprintPure, Category = "Stage Zero|Warmup", meta = (BlueprintInternalUseOnly = "true", DeprecatedFunction, DeprecationMessage = "Cross-process warmup skipping was removed."))
    bool IsWarmupCacheEnabled() const;

    /** Deprecated compatibility API; always returns false. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Warmup Cache", meta = (BlueprintInternalUseOnly = "true", AdvancedDisplay = "bRequirePipelineCacheClear,bShowDebug", DeprecatedFunction, DeprecationMessage = "Cross-process warmup skipping was removed."))
    bool CanSkipStageZeroWarmup(bool bRequirePipelineCacheClear = true, bool bShowDebug = false);

    /** Deprecated compatibility API; reports that persistent warmup skipping is unavailable. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Warmup Cache", meta = (BlueprintInternalUseOnly = "true", AdvancedDisplay = "bRequirePipelineCacheClear,bShowDebug", DeprecatedFunction, DeprecationMessage = "Cross-process warmup skipping was removed."))
    FStageZeroWarmupCacheStatus GetWarmupCacheStatus(bool bRequirePipelineCacheClear = true, bool bShowDebug = false);

    /** Release assets retained by the completed preload. Automatic Stage Zero travel releases them after PostLoadMap. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Warmup")
    void ReleaseRetainedWarmupAssets();

    /** Returns a debug list of all assets Stage Zero is warming, including whether they are in the manifest, warmup requested, and warmup finished. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Debug", meta = (ToolTip = "Returns a debug list of all assets Stage Zero is warming, including whether they are in the manifest, warmup requested, and warmup finished."))
    void GetStageZeroWarmupJobs(TArray<FStageZeroWarmupJobInfo>& OutJobs) const;

    /** Returns whether Stage Zero warmup debug feed is enabled. */
    UFUNCTION(BlueprintCallable, Category = "Stage Zero|Debug", meta = (ToolTip = "Returns whether Stage Zero warmup debug feed is enabled."))
    bool IsStageZeroWarmupDebugEnabled() const;

    /** Warmup start notification. */
    UPROPERTY(BlueprintAssignable, Category = "Stage Zero|Events")
    FStageZeroWarmupStarted OnWarmupStarted;

    /** Warmup progress notification (global and per-map). */
    UPROPERTY(BlueprintAssignable, Category = "Stage Zero|Events")
    FStageZeroWarmupProgress OnWarmupProgress;

    /** Warmup completion notification. */
    UPROPERTY(BlueprintAssignable, Category = "Stage Zero|Events")
    FStageZeroWarmupFinished OnWarmupFinished;

    /** Detailed status notification with per-domain counters. */
    UPROPERTY(BlueprintAssignable, Category = "Stage Zero|Events")
    FStageZeroWarmupStatusChanged OnWarmupStatusChanged;

    /** Detailed finish notification with success, failure, timeout, partial, or cancel result. */
    UPROPERTY(BlueprintAssignable, Category = "Stage Zero|Events")
    FStageZeroWarmupFinishedDetailed OnWarmupFinishedDetailed;

    /** Logical per-map asset-coverage start. Aggregate loading can start multiple map groups concurrently. */
    UPROPERTY(BlueprintAssignable, Category = "Stage Zero|Events")
    FStageZeroWarmupMapEvent OnWarmupMapStarted;

    /** Logical per-map asset-coverage finish. Run-scoped PSO work is reported by the overall finish event. */
    UPROPERTY(BlueprintAssignable, Category = "Stage Zero|Events")
    FStageZeroWarmupMapEvent OnWarmupMapFinished;

private:
    struct FStageZeroBootConfig
    {
        TSoftClassPtr<UUserWidget> WidgetClass;
        TSoftObjectPtr<UTexture2D> SpinnerIcon;
        TArray<TSoftObjectPtr<UTexture2D>> BackgroundSlides;
        FString BootMoviePath;
        bool bLoopBootMovie = true;
        bool bBootMovieSkippable = true;
        float BootMovieSkipDelaySeconds = 0.0f;
        FText BootMovieSkipText;
        bool bRestoreSlidesAfterBootMovie = true;
        bool bPlayBootMovieAudio = false;
        float BootMovieVolume = 1.0f;
        float SlideInterval = 2.0f;
        float SlideFadeDuration = 0.25f;
        bool bLoopSlides = true;
        TArray<FText> Tips;
        float TipInterval = 3.0f;
        TSoftObjectPtr<USoundBase> BackgroundMusic;
        float MusicFadeInTime = 1.0f;
        float MusicFadeOutTime = 1.0f;
        float SpinnerOnlyDuration = 0.3f;
        bool bShowSpinnerDuringProgress = true;
    };

private:
    // Warmup flow
    void FinishWarmupInternal(bool bCancelled);
    void FinishWarmupInternal(bool bCancelled, EStageZeroWarmupResult Result, const FString& Reason = FString());
    void FinishWarmupAfterMinimumDisplay(bool bCancelled, EStageZeroWarmupResult Result, FString Reason);
    void BroadcastWarmupStartedIfNeeded();
    void FinishWarmupFailure(EStageZeroWarmupResult Result, const FString& Reason);
    void WarnIfStartedAfterGameplayWorld() const;
    bool ShouldSkipWarmupForRuntimeContext(bool bShowDebug) const;
    bool TickAutoStartup(float DeltaSeconds);

    // Widget orchestration
    void SpawnBuiltInWidgetIfConfigured();
    void ApplyBootScreenConfig(UStageZeroLoadingWidget* Widget);
    void ShowProgressAfterSpinnerPhase();
    void RemoveLoadingWidget();
    void ConfigureLegacyTransitionMessage(bool bShowMessage, const FString& Message);
    bool DispatchLegacyTransitionMessage(FName DestinationLevel);
    void ClearLegacyTransitionPresentation();
    void StartBootMusic(const FStageZeroBootConfig& BootConfig);
    void StopBootMusic(bool bImmediate);
    bool ShouldUseSubsystemBootMusic(const FStageZeroBootConfig& BootConfig) const;
    bool ShouldShowWarmupLoadingScreen() const;
    bool ShouldSuppressTravelLoadingScreenForDestination(const FString& DestinationMapName, bool bExplicitRequest = false) const;
    bool ShouldShowTravelLoadingScreenForWorld(const UWorld* World, bool bExplicitRequest = false, const FString& DestinationMapName = FString()) const;
    void HandlePreLoadMapWithContext(const FWorldContext& WorldContext, const FString& MapName);
    void HandlePostLoadMapWithWorld(UWorld* LoadedWorld);
    void HandleTravelMoviePlaybackStarted();
    bool SetupTravelLoadingScreen(const FString& MapName, bool bStageZeroInitiated);
    TSharedRef<class SWidget> BuildTravelLoadingScreenWidget(const FString& MapName, bool bHasMovies, bool bIsPostLoadCover = false);
    bool AddTravelViewportCover(const FString& MapName, bool bIsPostLoadCover, UWorld* LoadedWorld = nullptr);
    class UGameViewportClient* ResolveTravelViewportClient(UWorld* PreferredWorld = nullptr) const;
    TArray<UTexture2D*> LoadTravelBackgroundTextures(bool bHasMovies);
    TArray<FString> BuildNormalizedTravelMoviePaths() const;
    int32 ResolveTravelLoadingScreenProfileIndex(const FString& MapName) const;
    const FStageZeroLoadingScreenProfile* GetActiveTravelLoadingScreenProfile() const;
    bool HasPostLoadTravelDismissInput(UWorld* World) const;
    float GetTravelMinimumDisplaySeconds() const;
    void OpenLevelWithoutTravelLoadingScreen(FName LevelName, bool bAbsolute = true, FString Options = TEXT(""), bool bShowDebug = false);
    void ExecutePendingTravelOpenLevel();
    bool TickPendingTravelOpenLevel(float DeltaSeconds);
    void StartPostLoadTravelCover(UWorld* LoadedWorld);
    bool TickPostLoadTravelCover(float DeltaSeconds);
    void RemovePostLoadTravelCover();

    // Manifest selection helpers
    void ResetWarmupTrackingState();
    FName ResolveMapPackageNameFromInput(const FString& MapId) const;
    bool ApplyFirstLegacyLoaderInWorld(bool bBindDelegates);
    int32 WarmUpMatTexList(const TArray<FSoftObjectPath>& Materials, const TArray<FSoftObjectPath>& Textures, const TArray<FSoftObjectPath>& NiagaraSystems, const TArray<FSoftObjectPath>& StaticMeshes, const TArray<FSoftObjectPath>& SkeletalMeshes, const FName& Label);
    void WarmUpFromManifest(const UStageZeroWarmupManifest& Manifest, const TArray<FName>& MapPackages, bool bShowDebug);
    void StartWarmupWithAssets(const TArray<FSoftObjectPath>& Materials, const TArray<FSoftObjectPath>& Textures, const TArray<FSoftObjectPath>& NiagaraSystems, const TArray<FSoftObjectPath>& StaticMeshes, const TArray<FSoftObjectPath>& SkeletalMeshes, const FName& Label, const TArray<FStageZeroManifestMapCoverage>& MapCoveragePlan, bool bShowDebug);
    bool BuildAssetsFromWPCells(const UStageZeroWarmupManifest& Manifest, const FName& MapPackage, TArray<FSoftObjectPath>& OutMaterials, TArray<FSoftObjectPath>& OutTextures, TArray<FSoftObjectPath>& OutNiagara, TArray<FSoftObjectPath>& OutStaticMeshes, TArray<FSoftObjectPath>& OutSkeletalMeshes, int32& OutCellCount, bool& bOutUsedMapFallback, FString& OutFailureReason) const;
    bool BuildAssetsFromWPAnchors(const UStageZeroWarmupManifest& Manifest, const FName& MapPackage, const TArray<FName>& AnchorNames, TArray<FSoftObjectPath>& OutMaterials, TArray<FSoftObjectPath>& OutTextures, TArray<FSoftObjectPath>& OutNiagara, TArray<FSoftObjectPath>& OutStaticMeshes, TArray<FSoftObjectPath>& OutSkeletalMeshes, int32& OutAnchorCount, FString& OutFailureReason) const;
    bool BuildAssetsFromWPSplines(const UStageZeroWarmupManifest& Manifest, const FName& MapPackage, const TArray<FName>& RouteNames, TArray<FSoftObjectPath>& OutMaterials, TArray<FSoftObjectPath>& OutTextures, TArray<FSoftObjectPath>& OutNiagara, TArray<FSoftObjectPath>& OutStaticMeshes, TArray<FSoftObjectPath>& OutSkeletalMeshes, int32& OutRouteCount, FString& OutFailureReason) const;
    void StartManifestProcessing();
    bool TickManifestRunDeadline(float DeltaSeconds);
    void ProcessManifestJobBatch();
    void HandleManifestAsyncLoadComplete(uint64 RequestSerial);
    void HandleManifestAsyncLoadTimeout(uint64 RequestSerial);
    void CompleteManifestBatchAndContinue();
    void ProcessLoadedManifestBatch(const TArray<FStageZeroManifestJob>& Jobs);
    void AdvanceManifestProgress(int32 Delta);
    void RecordManifestFailure(const FStageZeroManifestJob& Job, const FString& Reason);
    void TrackWarmupJobRequested(const FSoftObjectPath& Path, const FString& AssetType, bool bInManifest);
    void TrackWarmupJobLoading(const FSoftObjectPath& Path);
    void TrackWarmupJobFinished(const FSoftObjectPath& Path, EStageZeroWarmupJobTerminalState State = EStageZeroWarmupJobTerminalState::Succeeded, const FString& Reason = FString());
    bool InitializeWarmupMapCoverage(const TArray<FStageZeroManifestMapCoverage>& MapCoveragePlan, const FName& FallbackMapName, const TSet<FSoftObjectPath>& AggregatePaths, FString& OutFailureReason);
    void BroadcastWarmupMapCoverageStarts();
    void MarkWarmupMapPathProcessed(const FSoftObjectPath& Path, EStageZeroWarmupJobTerminalState State);
    void FinishWarmupMapCoverageIfComplete(int32 MapIndex);
    void MarkOutstandingManifestJobsCancelled(const FString& Reason);
    void MarkOutstandingManifestJobsTimedOut(const FString& Reason);
    bool AreAllManifestJobsTerminal() const;
    bool DidAllManifestJobsSucceed() const;

    // Runtime job tracking (real shader/streaming progress)
    void StartRealJobPolling(int32 MapCount);
    void PollRealJobProgress();
    void SnapshotJobBaselines();
    int32 GetRecordedPipelineCacheWorkRemaining() const;

    // Config resolution
    FStageZeroBootConfig BuildBootConfig() const;
    const UStageZeroProjectSettings* GetSettings() const;

    // Task accounting
    void UpdateProgressBroadcast(int32 MapCompleted, int32 MapTotal);
    FStageZeroWarmupStatus BuildWarmupStatusSnapshot() const;
    void BroadcastWarmupStatus();
    bool IsStrictCompletionEnabled() const;
    bool ShouldTreatAssetFailuresAsFatal() const;
    bool ShouldContinueOnPipelineCacheTimeout() const;
    bool ShouldUseAsyncWarmupLoading() const;
    bool ShouldUsePipelineCacheWarmup() const;
    float GetManifestAssetLoadTimeoutSeconds() const;
    float GetPipelineCacheTimeoutSeconds() const;
    bool TryStartRecordedPipelineCacheWarmup();
    void ClearRuntimeWarmupCallbacks();

private:
    friend class AStageZeroLoader;
    friend class FStageZeroWorldPartitionManifestSelectionTest;
    friend class FStageZeroPerMapLifecycleAccountingTest;
    friend class FStageZeroRuntimeSettingsPolicyTest;
    friend class FStageZeroLegacyTransitionMessageCompatibilityTest;

    // Runtime state
    int32 CompletedMaps = 0;
    int32 TotalTasks = 0;
    int32 CompletedTasks = 0;
    int32 RawCompletedTasks = 0;
    int32 TaskBaseline = 0;
    int32 CurrentMapTaskTotal = 0;
    float WarmupProgress = 0.0f;
    bool bWarmupRunning = false;
    bool bWarmupCompleted = false;
    bool bWasCancelled = false;
    bool bWarmupStartedBroadcast = false;
    EStageZeroWarmupResult WarmupResult = EStageZeroWarmupResult::NotStarted;
    FString LastWarmupStatusMessage;
    FName CurrentMapName = NAME_None;
    double WarmupStartTimeSeconds = 0.0;
    float MinimumDisplaySeconds = 0.0f;
    bool bMinimumDisplaySecondsOverrideSet = false;
    float TravelMinimumDisplaySecondsOverride = 0.0f;
    bool bTravelMinimumDisplaySecondsOverrideSet = false;
    float DelayAfterComplete = 0.0f;
    bool bAutoTransitionToPrimary = false;
    bool bAutoTransitionOverrideSet = false;
    bool bShouldSpawnWidget = true;
    bool bAllowCreatePlayerIfMissing = false;

    // Overrides
    bool bWarmUpAllMapsOverrideSet = false;
    bool bWarmUpAllMapsOverride = false;
    bool bHasLegacySettings = false;
    TArray<TSoftObjectPtr<UWorld>> ManualWarmupOverride;
    TSoftObjectPtr<UWorld> PrimaryLevelOverride;
    FStageZeroBootConfig LegacyBootConfig;
    bool bRuntimeBootOverride = false;
    FStageZeroBootConfig RuntimeBootConfig;

    // Widget tracking
    TWeakObjectPtr<UStageZeroLoadingWidget> ActiveLoadingWidget;
    TWeakObjectPtr<UUserWidget> ActiveGenericWidget;
    FString LegacyTransitionMessage;
    bool bLegacyTransitionMessageEnabled = false;
    bool bLegacyTransitionMessageDispatched = false;
    bool bLegacyTransitionWidgetAwaitingTravelCleanup = false;
    bool bWidgetSpawnedBySubsystem = false;
    bool bHideRegisteredWidgetOnComplete = false;
    FTimerHandle SpinnerPhaseTimer;
    FTimerHandle FinishDelayTimer;
    FTimerHandle JobPollTimer;
    FTimerHandle ManifestJobTimer;
    FTimerHandle ManifestLoadTimeoutTimer;
    UPROPERTY()
    UAudioComponent* BootMusicComponent = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<UTexture2D>> TravelLoadingBackgroundTextures;
    UPROPERTY()
    TObjectPtr<UUserWidget> ActivePostLoadTravelUserWidget = nullptr;
    UPROPERTY()
    TObjectPtr<UStageZeroNativeLoadingScreenProvider> ActiveNativeLoadingScreenProvider = nullptr;
    float BootMusicFadeOutTime = 0.0f;
    FTSTicker::FDelegateHandle StartupTickerHandle;
    FTSTicker::FDelegateHandle ManifestRunDeadlineTickerHandle;
    FTSTicker::FDelegateHandle PendingTravelOpenLevelTickerHandle;
    FTSTicker::FDelegateHandle PostLoadTravelCoverTickerHandle;
    FDelegateHandle TravelMoviePlaybackStartedHandle;
    bool bAutoStartupAttempted = false;
    bool bPendingTravelOpenLevel = false;
    FName PendingTravelOpenLevelName = NAME_None;
    FString PendingTravelOpenLevelOptions;
    bool bPendingTravelOpenLevelAbsolute = true;
    bool bPendingTravelOpenLevelDebug = false;
    bool bSuppressNextAutomaticTravelLoadingScreen = false;
    FString SuppressedTravelMapName;
    bool bTravelLoadingScreenActive = false;
    bool bStageZeroInitiatedTravel = false;
    bool bReleaseRetainedAssetsAfterNextMapLoad = false;
    bool bTravelMoviePlayerAvailable = false;
    bool bTravelMoviePlayerStarted = false;
    bool bPostLoadTravelCoverAdded = false;
    int32 PostLoadTravelCoverFramesRemaining = 0;
    double PostLoadTravelCoverEndTimeSeconds = 0.0;
    double PostLoadTravelCoverFadeOutStartTimeSeconds = 0.0;
    bool bPostLoadTravelFadeOutStarted = false;
    bool bPostLoadTravelWaitingForInput = false;
    int32 TravelBackgroundSelectionCounter = 0;
    int32 ActiveTravelLoadingScreenProfileIndex = INDEX_NONE;
    FString PendingTravelMapName;
    TWeakObjectPtr<UGameViewportClient> ActiveTravelViewportClient;
    TSharedPtr<class SWidget> ActivePostLoadTravelCoverWidget;

    // Cached settings
    const UStageZeroProjectSettings* CachedSettings = nullptr;

    // Real job tracking state
    int32 ManifestTaskTotal = 0;
    int32 ManifestTasksCompleted = 0;
    int32 PipelinePrecompileBaseline = 0;
    int32 TargetMapCount = 0;
    int32 LastProgressLogCompleted = -1;
    int32 LastBroadcastCompletedTasks = 0;
    int32 LastBroadcastTotalTasks = 0;
    float LastBroadcastProgress = 0.0f;
    int32 ManifestBatchSize = 5;
    int32 ManifestFailedLoads = 0;
    bool bManifestProcessingActive = false;
    TArray<FStageZeroManifestJob> ManifestJobQueue;
    int32 ManifestJobCursor = 0;
    TArray<FStageZeroManifestJob> ActiveManifestBatch;
    FStreamableManager ManifestStreamableManager;
    TSharedPtr<FStreamableHandle> ActiveManifestLoadHandle;
    uint64 ManifestLoadRequestSerial = 0;
    uint64 ActiveManifestLoadRequestSerial = 0;
    double ActiveManifestLoadStartTimeSeconds = 0.0;
    double ManifestRunDeadlineTimeSeconds = 0.0;
    UPROPERTY(Transient)
    TArray<TObjectPtr<UObject>> RetainedWarmupAssets;
    bool bPipelineTimedOut = false;
    bool bPipelineCacheStageRequested = false;
    bool bPipelineCacheStageAccepted = false;
    FString PipelineCacheStageFailureReason;
    uint64 PipelineObservationBaselineGeneration = 0;
    FString PipelineExpectedProjectCacheKey;
    FString PipelineExpectedUserCacheKey;
    TSet<FString> PipelineRequiredCacheKeys;
    int32 LastPipelineRemaining = 0;
    double PipelineWaitStartTimeSeconds = 0.0;
    double LastPipelineDecreaseTimeSeconds = 0.0;
    TArray<FString> FailedWarmupAssets;
    TSet<FSoftObjectPath> FailedWarmupAssetPaths;
    bool bWarmupFinishPending = false;
    TArray<FStageZeroManifestMapCoverage> ActiveWarmupMapCoverage;
    TMap<FSoftObjectPath, TArray<int32>> ManifestPathMapCoverageMembership;

    /** Deprecated compatibility field. Per-item terminal accounting is always enabled for correctness. */
    UPROPERTY(Config, meta = (DeprecatedProperty, DeprecationMessage = "Per-item terminal accounting is always enabled."))
    bool bEnableWarmupDebugFeed = false;

    TMap<FSoftObjectPath, FStageZeroWarmupJobInfo> WarmupDebugJobs;

    double LastObservedOutstandingRuntimeWorkTime = 0.0;
};
