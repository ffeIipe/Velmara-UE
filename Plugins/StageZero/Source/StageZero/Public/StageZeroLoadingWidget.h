// Copyright (c) 2025 71 Studios for games Development. All rights reserved. 

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Components/ProgressBar.h"
#include "Input/Events.h"
#include "Input/Reply.h"
#include "StageZeroSubsystem.h"
#include "StageZeroLoadingWidget.generated.h"

class UTexture2D;
class UStageZeroSubsystem;
class UFileMediaSource;
class UMediaPlayer;
class UMediaSoundComponent;
class UMediaTexture;

/**
 * Loading widget for the StageZero shader precompilation system.
 * This widget handles the UI display during loading.
 */
UCLASS(BlueprintType, Blueprintable)
class STAGEZERO_API UStageZeroLoadingWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Optional spinner image (driven at runtime). Add to your widget and bind this variable. */
    UPROPERTY(BlueprintReadWrite, Category = "StageZero|BootScreen", meta = (BindWidgetOptional))
    UImage* LoadingIconImage = nullptr;

    /** Optional background image used for slideshow. */
    UPROPERTY(BlueprintReadWrite, Category = "StageZero|BootScreen", meta = (BindWidgetOptional))
    UImage* BackgroundImage = nullptr;

    /** Optional image used for the boot-only movie. If unbound, Stage Zero uses BackgroundImage. */
    UPROPERTY(BlueprintReadWrite, Category = "StageZero|BootScreen", meta = (BindWidgetOptional))
    UImage* BootMovieImage = nullptr;

    /** Optional text block used for the boot movie skip prompt. */
    UPROPERTY(BlueprintReadWrite, Category = "StageZero|BootScreen", meta = (BindWidgetOptional))
    UTextBlock* BootMovieSkipTextBlock = nullptr;

    /** Optional text block used for rotating tips. */
    UPROPERTY(BlueprintReadWrite, Category = "StageZero|BootScreen", meta = (BindWidgetOptional))
    UTextBlock* TipText = nullptr;

    /** Optional spinner container (if you want to hide/show a wrapper). */
    UPROPERTY(BlueprintReadWrite, Category = "StageZero|BootScreen", meta = (BindWidgetOptional))
    UWidget* SpinnerContainer = nullptr;

    /** Optional progress container (if you want to hide/show a wrapper around your progress bar). */
    UPROPERTY(BlueprintReadWrite, Category = "StageZero|BootScreen", meta = (BindWidgetOptional))
    UWidget* ProgressContainer = nullptr;

    /** Optional progress bar driven by the subsystem's progress delegate. */
    UPROPERTY(BlueprintReadWrite, Category = "StageZero|BootScreen", meta = (BindWidgetOptional))
    UProgressBar* ProgressBar = nullptr;

    /** Optional progress label for displaying detailed counts. */
    UPROPERTY(BlueprintReadWrite, Category = "StageZero|BootScreen", meta = (BindWidgetOptional))
    UTextBlock* ProgressText = nullptr;

    /** If true, the subsystem will remove this widget from the viewport when warmup completes. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StageZero|BootScreen", meta = (ExposeOnSpawn = "true"))
    bool bHideAfterWarmup = false;

    /**
     * Called when shader compilation progress updates
     * @param PercentComplete - Progress as 0-1 float (0=start, 1=complete)
     * @param CompletedJobs - Current number of completed shader/asset jobs (counts UP from 0)
     * @param TotalJobs - Total number of shader/asset jobs to complete
     */
    UFUNCTION(BlueprintImplementableEvent)
    void OnShaderProgress(float PercentComplete, int32 CompletedJobs, int32 TotalJobs);

    /**
     * Called when completion phase is reached
     * @param Message - Completion message to display
     */
    UFUNCTION(BlueprintImplementableEvent)
    void OnCompletionMessage(const FString& Message);

    /**
     * Called right before level transition
     * @param Message - Transition message to display
     */
    UFUNCTION(BlueprintImplementableEvent)
    void OnTransitionMessage(const FString& Message);

    /**
     * Called to display build information
     * @param BuildDateTime - Build date and time string
     */
    UFUNCTION(BlueprintImplementableEvent)
    void OnBuildInfo(const FString& BuildDateTime);

    /** Called when spinner visibility changes (optional override) */
    UFUNCTION(BlueprintImplementableEvent)
    void OnSpinnerVisibilityChanged(bool bVisible);

    /** Called when progress visibility changes (optional override) */
    UFUNCTION(BlueprintImplementableEvent)
    void OnProgressVisibilityChanged(bool bVisible);

    /** Called when background slide changes (optional override) */
    UFUNCTION(BlueprintImplementableEvent)
    void OnBackgroundChanged(UTexture2D* NewTexture);

    /** Called when tip text changes (optional override) */
    UFUNCTION(BlueprintImplementableEvent)
    void OnTipChanged(const FText& NewTip);

    /** Called when the boot-only movie starts playing. */
    UFUNCTION(BlueprintImplementableEvent)
    void OnBootMovieStarted(const FString& MoviePath);

    /** Called when the boot-only movie cannot be opened. */
    UFUNCTION(BlueprintImplementableEvent)
    void OnBootMovieFailed(const FString& MoviePath);

    /** Called when the boot-only movie reaches its natural end and is not looping. */
    UFUNCTION(BlueprintImplementableEvent)
    void OnBootMovieFinished(const FString& MoviePath);

    /** Called when the player skips the boot-only movie. */
    UFUNCTION(BlueprintImplementableEvent)
    void OnBootMovieSkipped(const FString& MoviePath);

    /** Configure visuals at boot */
    UFUNCTION(BlueprintCallable, Category = "StageZero|BootScreen")
    void InitializeBootScreen(const TArray<TSoftObjectPtr<UTexture2D>>& Slides, float SlideIntervalSeconds, float SlideFadeSeconds, bool bLoopSlidesIn, const TArray<FText>& Tips, float TipIntervalSeconds, UTexture2D* SpinnerTexture, bool bSpinnerInitiallyVisible);

    /** Configure boot movie interaction options before InitializeBootMovie. */
    UFUNCTION(BlueprintCallable, Category = "StageZero|BootScreen")
    void ConfigureBootMovieOptions(bool bSkippable, const FText& SkipText, float SkipDelaySeconds, bool bRestoreSlidesAfterMovie);

    /** Configure an optional boot-only movie shown by this widget. Movie files should be placed under Content/Movies. */
    UFUNCTION(BlueprintCallable, Category = "StageZero|BootScreen")
    void InitializeBootMovie(const FString& MoviePath, bool bLoopMovie, bool bPlayAudio, float Volume);

    /** Skip the active boot movie if skipping is enabled and its skip delay has elapsed. Returns true when a movie was skipped. */
    UFUNCTION(BlueprintCallable, Category = "StageZero|BootScreen")
    bool SkipBootMovie();

    /** Toggle spinner visibility safely */
    UFUNCTION(BlueprintCallable, Category = "StageZero|BootScreen")
    void SetSpinnerVisibility(bool bVisible);

    /** Toggle progress visibility safely */
    UFUNCTION(BlueprintCallable, Category = "StageZero|BootScreen")
    void SetProgressVisibility(bool bVisible);

    /** Immediately set the current tip text */
    UFUNCTION(BlueprintCallable, Category = "StageZero|BootScreen")
    void SetTip(const FText& InTip);

    /** Bind this widget to the active Stage Zero subsystem and immediately apply its current progress snapshot. */
    void BindToStageZeroSubsystem(UStageZeroSubsystem* Subsystem);

    /** Remove subsystem delegate bindings and unregister this widget from the active Stage Zero subsystem. */
    void UnbindFromStageZeroSubsystem();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;

    UFUNCTION()
    void HandleSubsystemProgress(float Percent, FName MapName, int32 CompletedTasks, int32 TotalTasks, int32 MapCompleted, int32 MapTotal);

    UFUNCTION()
    void HandleSubsystemFinished(bool bWasCancelled);

    UFUNCTION()
    void HandleSubsystemFinishedDetailed(EStageZeroWarmupResult Result, FStageZeroWarmupStatus Status);

    UFUNCTION()
    void HandleBootMediaOpened(FString OpenedUrl);

    UFUNCTION()
    void HandleBootMediaOpenFailed(FString FailedUrl);

    UFUNCTION()
    void HandleBootMediaEndReached();

private:
    void SetLoadingIconTexture(UTexture2D* Texture);
    FString ResolveBootMovieFilePath(const FString& MoviePath) const;
    bool CanSkipBootMovie() const;
    UTextBlock* GetBootMovieSkipPrompt();
    void ShowBootMovieSkipPrompt();
    void HideBootMovieSkipPrompt();
    void StopBootMovie(bool bRestoreSlides = true);
    void StartBackgroundSlideshow();
    void AdvanceBackgroundSlide();
    void TickBackgroundFade();
    void StartTipRotation();
    void AdvanceTip();
    void ClearTimers();

private:
    TArray<TSoftObjectPtr<UTexture2D>> SlideTextures;
    float SlideInterval = 2.0f;
    float SlideFadeDuration = 0.25f;
    bool bLoopSlides = true;
    int32 CurrentSlideIndex = 0;
    FTimerHandle SlideTimerHandle;
    FTimerHandle SlideFadeTimerHandle;
    double SlideFadeStartTime = 0.0;

    TArray<FText> TipList;
    float TipInterval = 3.0f;
    int32 CurrentTipIndex = 0;
    FTimerHandle TipTimerHandle;

    TWeakObjectPtr<UStageZeroSubsystem> CachedSubsystem;

    UPROPERTY(Transient)
    TObjectPtr<UMediaPlayer> BootMediaPlayer = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UMediaTexture> BootMediaTexture = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UFileMediaSource> BootMediaSource = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UMediaSoundComponent> BootMediaSoundComponent = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> DynamicBootMovieSkipTextBlock = nullptr;

    FString ActiveBootMoviePath;
    bool bBootMovieLoop = true;
    bool bBootMovieUsingBackgroundImage = false;
    bool bBootMovieSkippable = true;
    bool bRestoreSlidesAfterBootMovie = true;
    float BootMovieSkipDelaySeconds = 0.0f;
    double BootMovieStartTimeSeconds = 0.0;
    FText BootMovieSkipText;
    FTimerHandle BootMovieSkipPromptTimerHandle;
};
