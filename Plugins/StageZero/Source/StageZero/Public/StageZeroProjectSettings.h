// Copyright (c) 2025 71 Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Blueprint/UserWidget.h"
#include "StageZeroNativeLoadingScreenProvider.h"
#include "StageZeroProjectSettings.generated.h"

STAGEZERO_API DECLARE_LOG_CATEGORY_EXTERN(LogStageZeroSettings, Log, All);

UENUM(BlueprintType)
enum class EStageZeroStartupMode : uint8
{
    Manual,
    AutoOnGameInstanceInit,
    BootMapGate
};

UENUM(BlueprintType)
enum class EStageZeroCompletionPolicy : uint8
{
    Strict UMETA(DisplayName = "Strict"),
    PartialOk UMETA(DisplayName = "Best Effort (Partial Allowed)")
};

UENUM(BlueprintType)
enum class EStageZeroNumberFormat : uint8
{
    Comma UMETA(DisplayName = "Comma (1,000)"),
    Dot UMETA(DisplayName = "Dot (1.000)"),
    Plain UMETA(DisplayName = "Plain (1000)")
};

UENUM(BlueprintType)
enum class EStageZeroLoadingScreenMode : uint8
{
    Disabled UMETA(DisplayName = "Disabled"),
    WarmupOnly UMETA(DisplayName = "Manual / Explicit Travel Only"),
    MapTravelOnly UMETA(DisplayName = "Automatic Map Travel")
};

UENUM(BlueprintType)
enum class EStageZeroTravelMoviePlayback : uint8
{
    PlayOnce UMETA(DisplayName = "Play Once"),
    LoopAll UMETA(DisplayName = "Loop All"),
    LoopLast UMETA(DisplayName = "Loop Last")
};

UENUM(BlueprintType)
enum class EStageZeroTravelImageSelection : uint8
{
    First UMETA(DisplayName = "First Image"),
    Sequential UMETA(DisplayName = "Sequential / Slideshow"),
    Random UMETA(DisplayName = "Random Image")
};

UENUM(BlueprintType)
enum class EStageZeroTravelImageSizeMode : uint8
{
    FillCrop UMETA(DisplayName = "Fill Screen (Crop)"),
    FitLetterbox UMETA(DisplayName = "Fit Screen (Letterbox)"),
    Stretch UMETA(DisplayName = "Stretch To Screen"),
    OriginalCentered UMETA(DisplayName = "Original Size Centered")
};

UENUM(BlueprintType)
enum class EStageZeroTravelLoadingScreenTemplate : uint8
{
    Classic UMETA(DisplayName = "Classic"),
    CinematicWide UMETA(DisplayName = "Cinematic Wide"),
    MinimalStudio UMETA(DisplayName = "Minimal Studio"),
    GameTipCarousel UMETA(DisplayName = "Game Tip Carousel"),
    MapBriefing UMETA(DisplayName = "Map Briefing"),
    VideoSplash UMETA(DisplayName = "Video Splash"),
    ConsoleSafe UMETA(DisplayName = "Console Safe")
};

UENUM(BlueprintType)
enum class EStageZeroLoadingScreenPlatformProfile : uint8
{
    Any UMETA(DisplayName = "Any Platform"),
    Desktop UMETA(DisplayName = "Desktop"),
    Console UMETA(DisplayName = "Console"),
    Mobile UMETA(DisplayName = "Mobile")
};

UENUM(BlueprintType)
enum class EStageZeroPostLoadDismissMode : uint8
{
    Timed UMETA(DisplayName = "Timed"),
    PressAnyKey UMETA(DisplayName = "Press Any Key After Load")
};

USTRUCT(BlueprintType)
struct STAGEZERO_API FStageZeroLoadingScreenProfile
{
    GENERATED_BODY()

    /** Optional name used only for editor readability and logs. */
    UPROPERTY(EditAnywhere, Category = "Stage Zero|Loading Screen|Profile")
    FName ProfileName = NAME_None;

    /** Destination map this profile applies to. Leave empty to use this profile as a platform-specific fallback. */
    UPROPERTY(EditAnywhere, Category = "Stage Zero|Loading Screen|Profile")
    TSoftObjectPtr<UWorld> DestinationMap;

    /** Platform group this profile applies to. Any Platform matches all platforms. */
    UPROPERTY(EditAnywhere, Category = "Stage Zero|Loading Screen|Profile")
    EStageZeroLoadingScreenPlatformProfile Platform = EStageZeroLoadingScreenPlatformProfile::Any;

    /** Override the global native loading-screen template for this destination/platform. */
    UPROPERTY(EditAnywhere, Category = "Stage Zero|Loading Screen|Profile")
    bool bOverrideLoadingScreenTemplate = false;

    UPROPERTY(EditAnywhere, Category = "Stage Zero|Loading Screen|Profile", meta = (EditCondition = "bOverrideLoadingScreenTemplate"))
    EStageZeroTravelLoadingScreenTemplate LoadingScreenTemplate = EStageZeroTravelLoadingScreenTemplate::Classic;

    /** Background images for this destination/platform. If empty, Stage Zero uses the global image settings. */
    UPROPERTY(EditAnywhere, Category = "Stage Zero|Loading Screen|Images")
    TArray<TSoftObjectPtr<UTexture2D>> BackgroundImages;

    UPROPERTY(EditAnywhere, Category = "Stage Zero|Loading Screen|Images")
    EStageZeroTravelImageSelection ImageSelection = EStageZeroTravelImageSelection::Sequential;

    UPROPERTY(EditAnywhere, Category = "Stage Zero|Loading Screen|Images")
    EStageZeroTravelImageSizeMode ImageSizeMode = EStageZeroTravelImageSizeMode::FillCrop;

    UPROPERTY(EditAnywhere, Category = "Stage Zero|Loading Screen|Images", meta = (ClampMin = "0.0"))
    float ImageIntervalSeconds = 2.5f;

    UPROPERTY(EditAnywhere, Category = "Stage Zero|Loading Screen|Images")
    bool bLoopImages = true;

    /** Movie names under Content/Movies for this profile. If empty, Stage Zero uses the global movie list. */
    UPROPERTY(EditAnywhere, Category = "Stage Zero|Loading Screen|Video")
    TArray<FString> MoviePaths;

    UPROPERTY(EditAnywhere, Category = "Stage Zero|Loading Screen|Video")
    EStageZeroTravelMoviePlayback MoviePlayback = EStageZeroTravelMoviePlayback::LoopLast;

    /** Optional title override for this profile. Empty keeps the global text. */
    UPROPERTY(EditAnywhere, Category = "Stage Zero|Loading Screen|Text")
    FText TitleText;

    /** Optional subtitle override for this profile. Empty keeps the global text. */
    UPROPERTY(EditAnywhere, Category = "Stage Zero|Loading Screen|Text")
    FText SubtitleText;

    /** Optional status override for this profile. Empty keeps the global text. */
    UPROPERTY(EditAnywhere, Category = "Stage Zero|Loading Screen|Text")
    FText StatusText;

    /** Optional footer override for this profile. Empty keeps the global text. */
    UPROPERTY(EditAnywhere, Category = "Stage Zero|Loading Screen|Text")
    FText FooterText;

    /** Optional rotating tips for this profile. Empty keeps the global tips. */
    UPROPERTY(EditAnywhere, Category = "Stage Zero|Loading Screen|Text")
    TArray<FText> Tips;

    /** Optional UMG widget used after the destination map has loaded. Blocking travel still uses native Slate. */
    UPROPERTY(EditAnywhere, Category = "Stage Zero|Loading Screen|Custom Widget")
    TSoftClassPtr<UUserWidget> PostLoadWidgetClass;
};

/**
 * Project-wide defaults for Stage Zero
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Stage Zero"))
class STAGEZERO_API UStageZeroProjectSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UStageZeroProjectSettings();

    /** Include Level Instance content in editor manifests and legacy read-only discovery of levels that are already loaded. No world or level is loaded by this setting. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Manifest Build", meta = (DisplayName = "Include Level Instances"))
    bool bWarmUpLevelInstances = true;

    /** Include Packed Level Actor content in editor manifests and legacy read-only discovery of levels that are already loaded. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Manifest Build", meta = (DisplayName = "Include Packed Level Actors"))
    bool bWarmUpPackedLevelActors = true;

    /** Hidden compatibility cap for package and already-loaded-world discovery. New C++ callers should pass MaxItemsOverride explicitly. */
    UPROPERTY(Config, meta = (DeprecatedProperty, DeprecationMessage = "Use the AutoCollectAssets MaxItemsOverride parameter. This hidden field remains for serialized compatibility."))
    int32 WarmupMaxItems = 2048;

    /** Production startup mode. BootMapGate is recommended when Stage Zero must run before the first gameplay map. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Warmup")
    EStageZeroStartupMode StartupMode = EStageZeroStartupMode::Manual;

    /** Overall completion policy. Strict fails required work unless an explicit advanced domain override allows a Partial result. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Warmup")
    EStageZeroCompletionPolicy CompletionPolicy = EStageZeroCompletionPolicy::Strict;

    /** Observe and wait for the project's recorded PSO cache. This never renders materials or creates synthetic PSOs. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Warmup", meta = (DisplayName = "Use Pipeline Cache Warmup (Runtime)", ToolTip = "Open and observe Unreal Engine's recorded PSO cache during Stage Zero. Disable for manifest asset preload only. This does not render materials."))
    bool bUsePipelineCacheWarmup = false;

    /** Maximum seconds to wait for the accepted recorded PSO cache before applying the timeout policy. */
    UPROPERTY(Config, EditAnywhere, AdvancedDisplay, Category = "Stage Zero|Warmup", meta = (ClampMin = "0.0", Units = "s", EditCondition = "bUsePipelineCacheWarmup"))
    float PipelineCacheTimeoutSeconds = 60.0f;

    /** Allow an otherwise strict run to finish as Partial if only the recorded PSO-cache stage times out. */
    UPROPERTY(Config, EditAnywhere, AdvancedDisplay, Category = "Stage Zero|Warmup", meta = (DisplayName = "Continue On Pipeline Cache Timeout", EditCondition = "bUsePipelineCacheWarmup", ToolTip = "When enabled, a recorded PSO-cache timeout produces an explicit Partial result instead of blocking startup. Asset failures still follow their own policy."))
    bool bContinueOnPipelineCacheTimeout = true;

    /** Deprecated compatibility field. Stage Zero never renders materials during runtime warmup. */
    UPROPERTY(Config, meta = (DeprecatedProperty, DeprecationMessage = "Runtime material rendering was removed because offscreen/canvas rendering is unsafe and does not prove gameplay PSO coverage."))
    bool bEnableMaterialRenderWarmup = false;

    /** In Strict completion mode, fail the run when any required manifest asset cannot be loaded. */
    UPROPERTY(Config, EditAnywhere, AdvancedDisplay, Category = "Stage Zero|Warmup", meta = (DisplayName = "Treat Asset Failures As Fatal", ToolTip = "When Strict completion is selected, any required manifest asset failure fails Stage Zero. Disable to report Partial and continue."))
    bool bTreatAssetFailuresAsFatal = true;

    /** Load manifest assets asynchronously in bounded batches. Disable only when a blocking synchronous diagnostic/fallback path is required. */
    UPROPERTY(Config, EditAnywhere, AdvancedDisplay, Category = "Stage Zero|Warmup", meta = (DisplayName = "Use Async Warmup Loading", ToolTip = "Recommended. Disable to synchronously load each bounded manifest batch on the game thread; synchronous loading can cause visible stalls but does not perform rendering."))
    bool bUseAsyncWarmupLoading = true;

    /** Wall-clock deadline for the complete manifest asset preload, independent of world pause or time dilation. */
    UPROPERTY(Config, EditAnywhere, AdvancedDisplay, Category = "Stage Zero|Warmup", meta = (ClampMin = "1.0", Units = "s"))
    float AssetLoadTimeoutSeconds = 300.0f;

    /** Deprecated compatibility field. Runtime hidden-map traversal is permanently disabled. */
    UPROPERTY(Config, meta = (DeprecatedProperty, DeprecationMessage = "Runtime hidden-map traversal was removed. Stage Zero now loads manifest assets only."))
    bool bLoadMapsDuringWarmup = false;

    /** Deprecated compatibility field. Cross-process asset warmup skipping is unsafe and ignored. */
    UPROPERTY(Config, meta = (DeprecatedProperty, DeprecationMessage = "Stage Zero no longer persists asset warmup completion across processes."))
    bool bEnableWarmupCacheSkip = false;

    /** Deprecated compatibility field. The boot warmup is never skipped from a previous process record. */
    UPROPERTY(Config, meta = (DeprecatedProperty, DeprecationMessage = "Cross-process boot-screen skipping was removed."))
    bool bSkipBootScreenWhenWarmupCacheValid = false;

    /** Deprecated compatibility field. */
    UPROPERTY(Config, meta = (DeprecatedProperty, DeprecationMessage = "Cross-process boot-screen skipping was removed."))
    bool bRequirePipelineCacheClearForCacheSkip = false;

    // Boot screen visuals
    /** Built-in loading widget class spawned by the subsystem when enabled */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Boot Screen", meta = (ToolTip = "Widget class spawned automatically when Stage Zero shows its built-in loading screen. Clear this to disable auto UI."))
    TSoftClassPtr<UUserWidget> BuiltInLoadingWidgetClass;

    /** Optional loading icon texture used for the spinner (leave empty to hide) */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Boot Screen")
    TSoftObjectPtr<UTexture2D> LoadingIconTexture;

    /** Background slideshow textures shown during boot */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Boot Screen")
    TArray<TSoftObjectPtr<UTexture2D>> BackgroundSlides;

    /** Optional boot-only movie file under Content/Movies. Examples: Intro, Intro.mp4, or Movies/Intro.mp4. This is shown by the built-in boot widget only, not by between-level travel screens. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Boot Screen|Video", meta = (DisplayName = "Boot Movie"))
    FString BootMoviePath;

    /** Loop the boot movie while Stage Zero warmup is still running. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Boot Screen|Video")
    bool bLoopBootMovie = true;

    /** Allow keyboard, mouse, gamepad, or touch input to skip the boot movie. This never skips Stage Zero warmup itself. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Boot Screen|Video", meta = (DisplayName = "Boot Movie Is Skippable"))
    bool bBootMovieSkippable = true;

    /** Optional delay before skip input is accepted, useful when showing a short branded intro. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Boot Screen|Video", meta = (ClampMin = "0.0", EditCondition = "bBootMovieSkippable", Units = "s"))
    float BootMovieSkipDelaySeconds = 0.0f;

    /** Optional prompt shown by widgets that bind BootMovieSkipTextBlock. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Boot Screen|Video", meta = (EditCondition = "bBootMovieSkippable"))
    FText BootMovieSkipText;

    /** When the movie finishes or is skipped, restore the boot slideshow if the movie used the background image slot. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Boot Screen|Video")
    bool bRestoreSlidesAfterBootMovie = true;

    /** Play the movie's audio during the Stage Zero boot screen. Leave disabled if you already use Boot Screen background music. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Boot Screen|Video")
    bool bPlayBootMovieAudio = false;

    /** Boot movie audio volume when Play Boot Movie Audio is enabled. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Boot Screen|Video", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BootMovieVolume = 1.0f;

    /** Time in seconds before switching to the next background slide */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Boot Screen", meta = (ClampMin = "0.1"))
    float BackgroundSlideInterval = 2.0f;

    /** Fade duration (seconds) when changing background slides */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Boot Screen", meta = (ClampMin = "0.0"))
    float BackgroundSlideFadeDuration = 0.25f;

    /** Whether to loop background slides after the last image */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Boot Screen")
    bool bLoopBackgroundSlides = true;

    /** Tips to rotate during boot */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Boot Screen")
    TArray<FText> BootTips;

    /** Time in seconds before switching to the next boot tip */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Boot Screen", meta = (ClampMin = "0.1"))
    float BootTipInterval = 3.0f;

    /** Optional music to play during the boot/loading screen */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Boot Screen")
    TSoftObjectPtr<class USoundBase> BackgroundMusic;

    /** Fade-in time for boot music (seconds) */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Boot Screen", meta = (ClampMin = "0.0"))
    float MusicFadeInTime = 1.0f;

    /** Fade-out time for boot music (seconds) */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Boot Screen", meta = (ClampMin = "0.0"))
    float MusicFadeOutTime = 1.0f;

    /** Minimum spinner-only phase duration before showing progress */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Boot Screen", meta = (ClampMin = "0.0"))
    float SpinnerOnlyPhaseDuration = 0.3f;

    /** Keep the spinner visible while the progress bar is shown */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Boot Screen")
    bool bShowSpinnerDuringProgress = true;

    /** Minimum time the warmup loading screen remains visible before Stage Zero can hide it or transition. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Boot Screen", meta = (ClampMin = "0.0", DisplayName = "Warmup Minimum Loading Time"))
    float WarmupMinimumDisplaySeconds = 0.5f;

    /** Controls automatic map-travel loading screens only. The first boot/warmup screen ignores this and uses Built In Loading Widget Class under Boot Screen. The explicit Open Level With Stage Zero Loading Screen node still attempts to show a travel screen. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen", meta = (DisplayName = "Automatic Travel Screen Mode"))
    EStageZeroLoadingScreenMode LoadingScreenMode = EStageZeroLoadingScreenMode::WarmupOnly;

    /** Native template used by map-travel loading screens. Advanced style/text settings below can still override the template defaults. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen", meta = (DisplayName = "Travel Loading Screen Template"))
    EStageZeroTravelLoadingScreenTemplate TravelLoadingScreenTemplate = EStageZeroTravelLoadingScreenTemplate::Classic;

    /** Per-map/per-platform overrides. More specific map matches win over platform-only fallback profiles. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Profiles", meta = (DisplayName = "Travel Loading Screen Profiles"))
    TArray<FStageZeroLoadingScreenProfile> TravelLoadingScreenProfiles;

    /** Background images for native map-travel loading screens. These are Slate-safe and can be shown during blocking OpenLevel travel. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Images", meta = (DisplayName = "Travel Background Images"))
    TArray<TSoftObjectPtr<UTexture2D>> TravelBackgroundImages;

    /** Select which configured travel image to show, or cycle through the list as a slideshow. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Images")
    EStageZeroTravelImageSelection TravelImageSelection = EStageZeroTravelImageSelection::Sequential;

    /** How travel images are scaled inside the loading-screen viewport. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Images")
    EStageZeroTravelImageSizeMode TravelImageSizeMode = EStageZeroTravelImageSizeMode::FillCrop;

    /** Seconds before advancing to the next travel image when Sequential / Slideshow is selected. Set to 0 to keep the selected image static. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Images", meta = (ClampMin = "0.0"))
    float TravelImageIntervalSeconds = 2.5f;

    /** Loop travel-image slideshows after the last image. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Images")
    bool bLoopTravelImages = true;

    /** Use the first Boot Screen background slide when Travel Background Image is empty. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Images")
    bool bUseBootSlideAsTravelImageFallback = true;

    /** If false, configured movies are allowed to be the visual background instead of being covered by the image. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Images")
    bool bShowTravelImageWhenMoviesPlay = false;

    /** Movie names under Content/Movies. Stage Zero accepts entries like Intro, Intro.mp4, or Movies/Intro.mp4 and normalizes them for MoviePlayer. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Video", meta = (DisplayName = "Travel Loading Movies"))
    TArray<FString> TravelMoviePaths;

    /** How configured travel movies are played while the level is loading. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Video")
    EStageZeroTravelMoviePlayback TravelMoviePlayback = EStageZeroTravelMoviePlayback::LoopLast;

    /** Allow users to skip travel movies after loading is complete. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Video")
    bool bTravelMoviesAreSkippable = false;

    /** Main title text shown by the native map-travel loading screen. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Text")
    FText TravelTitleText;

    /** Secondary text shown below the title. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Text")
    FText TravelSubtitleText;

    /** Status text shown near the spinner/accent line. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Text")
    FText TravelStatusText;

    /** Append the destination level name to the travel status line. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Text", meta = (DisplayName = "Show Destination Level Name"))
    bool bShowTravelDestinationLevelName = true;

    /** Optional footer/legal/support text shown at the bottom of the map-travel loading screen. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Text")
    FText TravelFooterText;

    /** Rotating tips shown by templates that include a tip area. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Text")
    TArray<FText> TravelTips;

    /** Seconds before switching to the next travel tip. Set to 0 to keep the first tip static. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Text", meta = (ClampMin = "0.0"))
    float TravelTipIntervalSeconds = 4.0f;

    /** Title font size for the native travel loading screen. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Text", meta = (ClampMin = "8", ClampMax = "96"))
    int32 TravelTitleFontSize = 34;

    /** Subtitle font size for the native travel loading screen. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Text", meta = (ClampMin = "8", ClampMax = "64"))
    int32 TravelSubtitleFontSize = 18;

    /** Status/footer font size for the native travel loading screen. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Text", meta = (ClampMin = "8", ClampMax = "48"))
    int32 TravelSmallTextFontSize = 13;

    /** Text shown on the post-load cover when Press Any Key After Load is enabled. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Text")
    FText TravelPressAnyKeyText;

    /** Solid background color used when no image/movie is available. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Style")
    FLinearColor TravelBackgroundColor = FLinearColor(0.015f, 0.018f, 0.022f, 1.0f);

    /** Tint overlay applied over image/video backgrounds so text remains readable. Alpha controls strength. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Style")
    FLinearColor TravelOverlayTint = FLinearColor(0.0f, 0.0f, 0.0f, 0.58f);

    /** Primary text color for the native travel loading screen. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Style")
    FLinearColor TravelPrimaryTextColor = FLinearColor(0.95f, 0.96f, 0.98f, 1.0f);

    /** Secondary text color for subtitle/footer text. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Style")
    FLinearColor TravelSecondaryTextColor = FLinearColor(0.68f, 0.72f, 0.78f, 1.0f);

    /** Accent color used for the spinner and small divider line. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Style")
    FLinearColor TravelAccentColor = FLinearColor(0.16f, 0.58f, 1.0f, 1.0f);

    /** Show a native Slate throbber on map-travel loading screens. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Behavior")
    bool bShowTravelSpinner = true;

    /** Fade in the native travel loading screen. Set to 0 for instant display. Disabled when Reduce Motion is enabled. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Behavior", meta = (ClampMin = "0.0"))
    float TravelFadeInSeconds = 0.0f;

    /** Fade out the post-load viewport cover before removing it. Set to 0 for instant removal. Disabled when Reduce Motion is enabled. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Behavior", meta = (ClampMin = "0.0"))
    float TravelFadeOutSeconds = 0.0f;

    /** Minimum time the MoviePlayer travel loading screen remains visible before it can auto-complete. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Behavior", meta = (ClampMin = "0.0", DisplayName = "Travel Minimum Loading Time"))
    float TravelMinimumDisplaySeconds = 0.35f;

    /** Keep a lightweight Slate cover over the game viewport for this many frames after PostLoadMap to avoid a black flash before the first stable frame. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Behavior", meta = (ClampMin = "0"))
    int32 TravelPostLoadHoldFrames = 2;

    /** Maps that should never use the travel loading screen. Add your Stage Zero boot/warmup map here. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Behavior", meta = (DisplayName = "Travel Screen Excluded Maps"))
    TArray<TSoftObjectPtr<UWorld>> TravelLoadingScreenExcludedMaps;

    /** Additional seconds to keep the post-load Slate cover alive after PostLoadMap. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Behavior", meta = (ClampMin = "0.0"))
    float TravelPostLoadHoldSeconds = 0.10f;

    /** How the post-load cover dismisses after the configured hold frames/time. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Behavior")
    EStageZeroPostLoadDismissMode TravelPostLoadDismissMode = EStageZeroPostLoadDismissMode::Timed;

    /** Allow MoviePlayer-backed travel screens in PIE when available. If disabled or unavailable, Stage Zero still uses the PIE-safe viewport cover fallback. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Behavior")
    bool bEnableTravelLoadingScreenInPIE = true;

    /** Optional UMG widget shown only as the post-load viewport cover. MoviePlayer's blocking phase uses native Slate for stability. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Custom Widget", meta = (DisplayName = "Post-Load Custom Widget"))
    TSoftClassPtr<UUserWidget> TravelPostLoadWidgetClass;

    /** Optional advanced C++ provider that replaces Stage Zero's native Slate blocking travel widget. UMG is not supported during MoviePlayer blocking loads. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Custom Widget", meta = (DisplayName = "Native Slate Loading Screen Provider"))
    TSoftClassPtr<UStageZeroNativeLoadingScreenProvider> NativeLoadingScreenProviderClass;

    /** Disable slideshow advancement, fades, and spinner motion on native travel screens. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Accessibility", meta = (DisplayName = "Reduce Motion"))
    bool bReduceLoadingScreenMotion = false;

    /** Strengthen the overlay and text colors for readability on image/video backgrounds. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Accessibility", meta = (DisplayName = "High Contrast Text"))
    bool bUseAccessibleHighContrastText = false;

    /** Multiplier applied to native travel-screen text. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|Loading Screen|Accessibility", meta = (ClampMin = "0.75", ClampMax = "1.50"))
    float TravelAccessibilityTextScale = 1.0f;

    /** Checks configured images, movies, widgets, and profiles from the editor and reports missing assets that may fail in cooked builds. */
    UFUNCTION(CallInEditor, Category = "Stage Zero|Loading Screen|Validation", meta = (DisplayName = "Validate Loading Screen Assets"))
    void ValidateLoadingScreenAssets() const;

    /** Number formatting used by Stage Zero loading screens and editor manifest count displays. Raw APIs and logs remain unformatted. */
    UPROPERTY(Config, EditAnywhere, Category = "Stage Zero|UI", meta = (DisplayName = "Job Count Number Format"))
    EStageZeroNumberFormat JobCountNumberFormat = EStageZeroNumberFormat::Comma;

    // Place settings under Plugins > Stage Zero
    virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
};
