// Copyright (c) 2025 71 Studios for games Development. All rights reserved.

#include "StageZeroLoadingWidget.h"
#include "TimerManager.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateBrush.h"
#include "StageZeroBlueprintLibrary.h"
#include "StageZeroNumberFormatting.h"
#include "StageZeroSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "FileMediaSource.h"
#include "HAL/FileManager.h"
#include "MediaPlayer.h"
#include "MediaSoundComponent.h"
#include "MediaTexture.h"
#include "HAL/PlatformTime.h"
#include "Misc/Paths.h"
#include "Runtime/Launch/Resources/Version.h"

#define LOCTEXT_NAMESPACE "StageZeroLoadingWidget"

DEFINE_LOG_CATEGORY_STATIC(LogStageZeroLoadingWidget, Log, All);

namespace
{
static bool StageZeroIsTerminalWarmupResult(EStageZeroWarmupResult Result)
{
    return Result != EStageZeroWarmupResult::NotStarted
        && Result != EStageZeroWarmupResult::Running;
}

static FString StageZeroNormalizeMoviePathInput(FString MoviePath)
{
    MoviePath.TrimStartAndEndInline();
    MoviePath.ReplaceInline(TEXT("\\"), TEXT("/"));
    if (MoviePath.IsEmpty())
    {
        return MoviePath;
    }

    FString ProjectContentDir = FPaths::ProjectContentDir();
    ProjectContentDir.ReplaceInline(TEXT("\\"), TEXT("/"));
    const FString ContentMoviesPrefix = TEXT("Content/Movies/");
    const FString MoviesPrefix = TEXT("Movies/");
    if (MoviePath.StartsWith(ProjectContentDir, ESearchCase::IgnoreCase))
    {
        MoviePath.RightChopInline(ProjectContentDir.Len());
    }
    if (MoviePath.StartsWith(ContentMoviesPrefix, ESearchCase::IgnoreCase))
    {
        MoviePath.RightChopInline(ContentMoviesPrefix.Len());
    }
    if (MoviePath.StartsWith(MoviesPrefix, ESearchCase::IgnoreCase))
    {
        MoviePath.RightChopInline(MoviesPrefix.Len());
    }

    while (MoviePath.StartsWith(TEXT("/")))
    {
        MoviePath.RightChopInline(1);
    }

    return MoviePath;
}
}

void UStageZeroLoadingWidget::NativeConstruct()
{
    Super::NativeConstruct();

    UnbindFromStageZeroSubsystem();
    if (UStageZeroSubsystem* Subsystem = UStageZeroBlueprintLibrary::GetStageZeroSubsystem(this))
    {
        Subsystem->RegisterLoadingWidget(this, bHideAfterWarmup);
    }
}

void UStageZeroLoadingWidget::InitializeBootScreen(
    const TArray<TSoftObjectPtr<UTexture2D>>& Slides,
    float SlideIntervalSeconds,
    float SlideFadeSeconds,
    bool bLoopSlidesIn,
    const TArray<FText>& Tips,
    float TipIntervalSeconds,
    UTexture2D* SpinnerTexture,
    bool bSpinnerInitiallyVisible)
{
    SlideTextures = Slides;
    SlideInterval = FMath::Max(0.1f, SlideIntervalSeconds);
    SlideFadeDuration = FMath::Max(0.0f, SlideFadeSeconds);
    this->bLoopSlides = bLoopSlidesIn;

    TipList = Tips;
    TipInterval = FMath::Max(0.1f, TipIntervalSeconds);

    ClearTimers();

    SetLoadingIconTexture(SpinnerTexture);
    SetSpinnerVisibility(bSpinnerInitiallyVisible && SpinnerTexture != nullptr);
    SetProgressVisibility(false);

    if (SlideTextures.Num() > 0)
    {
        if (!BackgroundImage)
        {
            UE_LOG(LogStageZeroLoadingWidget, Warning, TEXT("StageZero: %d boot background slide(s) configured, but no BackgroundImage widget is bound on %s."), SlideTextures.Num(), *GetName());
        }

        CurrentSlideIndex = 0;
        AdvanceBackgroundSlide();
        StartBackgroundSlideshow();
    }

    if (TipList.Num() > 0)
    {
        CurrentTipIndex = 0;
        AdvanceTip();
        StartTipRotation();
    }
    else if (TipText)
    {
        TipText->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UStageZeroLoadingWidget::ConfigureBootMovieOptions(bool bSkippable, const FText& SkipText, float SkipDelaySeconds, bool bRestoreSlidesAfterMovie)
{
    bBootMovieSkippable = bSkippable;
    BootMovieSkipText = SkipText;
    BootMovieSkipDelaySeconds = FMath::Max(0.0f, SkipDelaySeconds);
    bRestoreSlidesAfterBootMovie = bRestoreSlidesAfterMovie;

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(BootMovieSkipPromptTimerHandle);
    }

    if (!bBootMovieSkippable)
    {
        HideBootMovieSkipPrompt();
    }
    else if (BootMediaPlayer)
    {
        const double NowSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : FPlatformTime::Seconds();
        const double RemainingDelaySeconds = FMath::Max(0.0, static_cast<double>(BootMovieSkipDelaySeconds) - (NowSeconds - BootMovieStartTimeSeconds));
        if (GetWorld() && RemainingDelaySeconds > 0.0)
        {
            GetWorld()->GetTimerManager().SetTimer(
                BootMovieSkipPromptTimerHandle,
                this,
                &UStageZeroLoadingWidget::ShowBootMovieSkipPrompt,
                static_cast<float>(RemainingDelaySeconds),
                false);
            HideBootMovieSkipPrompt();
        }
        else
        {
            ShowBootMovieSkipPrompt();
        }
    }
}

void UStageZeroLoadingWidget::InitializeBootMovie(const FString& MoviePath, bool bLoopMovie, bool bPlayAudio, float Volume)
{
    StopBootMovie(false);

    ActiveBootMoviePath = MoviePath;
    bBootMovieLoop = bLoopMovie;
    BootMovieStartTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : FPlatformTime::Seconds();
    HideBootMovieSkipPrompt();

    const FString ResolvedPath = ResolveBootMovieFilePath(MoviePath);
    if (ResolvedPath.IsEmpty())
    {
        if (!MoviePath.TrimStartAndEnd().IsEmpty())
        {
            UE_LOG(LogStageZeroLoadingWidget, Warning, TEXT("StageZero: Boot movie was not found under Content/Movies: %s"), *MoviePath);
            OnBootMovieFailed(MoviePath);
        }
        return;
    }

    UImage* TargetImage = BootMovieImage ? BootMovieImage : BackgroundImage;
    bBootMovieUsingBackgroundImage = BootMovieImage == nullptr && BackgroundImage != nullptr;
    if (!TargetImage)
    {
        UE_LOG(LogStageZeroLoadingWidget, Warning, TEXT("StageZero: Boot movie configured but no BootMovieImage or BackgroundImage is bound on %s."), *GetName());
        OnBootMovieFailed(MoviePath);
        return;
    }

    if (GetWorld())
    {
        FTimerManager& TimerManager = GetWorld()->GetTimerManager();
        TimerManager.ClearTimer(SlideTimerHandle);
        TimerManager.ClearTimer(SlideFadeTimerHandle);
    }

    BootMediaPlayer = NewObject<UMediaPlayer>(this, TEXT("StageZeroBootMediaPlayer"));
    BootMediaTexture = NewObject<UMediaTexture>(this, TEXT("StageZeroBootMediaTexture"));
    BootMediaSource = NewObject<UFileMediaSource>(this, TEXT("StageZeroBootMediaSource"));
    if (!BootMediaPlayer || !BootMediaTexture || !BootMediaSource)
    {
        UE_LOG(LogStageZeroLoadingWidget, Warning, TEXT("StageZero: Failed to create boot movie media objects."));
        StopBootMovie();
        OnBootMovieFailed(MoviePath);
        return;
    }

    BootMediaPlayer->PlayOnOpen = true;
    BootMediaPlayer->SetLooping(bLoopMovie);
    BootMediaPlayer->OnMediaOpened.AddDynamic(this, &UStageZeroLoadingWidget::HandleBootMediaOpened);
    BootMediaPlayer->OnMediaOpenFailed.AddDynamic(this, &UStageZeroLoadingWidget::HandleBootMediaOpenFailed);
    BootMediaPlayer->OnEndReached.AddDynamic(this, &UStageZeroLoadingWidget::HandleBootMediaEndReached);

    BootMediaTexture->SetMediaPlayer(BootMediaPlayer);
    BootMediaTexture->AutoClear = true;
    BootMediaTexture->UpdateResource();

    BootMediaSource->SetFilePath(ResolvedPath);
    BootMediaSource->PrecacheFile = false;

    FSlateBrush Brush;
    Brush.SetResourceObject(BootMediaTexture);
    Brush.ImageSize = FVector2D(1920.0f, 1080.0f);
    TargetImage->SetBrush(Brush);
    TargetImage->SetVisibility(ESlateVisibility::Visible);
    TargetImage->SetRenderOpacity(1.0f);

    if (bBootMovieSkippable)
    {
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 1
        bIsFocusable = true;
#else
        SetIsFocusable(true);
#endif
        SetKeyboardFocus();

        if (GetWorld() && BootMovieSkipDelaySeconds > 0.0f)
        {
            GetWorld()->GetTimerManager().SetTimer(
                BootMovieSkipPromptTimerHandle,
                this,
                &UStageZeroLoadingWidget::ShowBootMovieSkipPrompt,
                BootMovieSkipDelaySeconds,
                false);
        }
        else
        {
            ShowBootMovieSkipPrompt();
        }
    }

    if (bPlayAudio && GetWorld())
    {
        BootMediaSoundComponent = NewObject<UMediaSoundComponent>(this, TEXT("StageZeroBootMediaSound"));
        if (BootMediaSoundComponent)
        {
            BootMediaSoundComponent->SetMediaPlayer(BootMediaPlayer);
            BootMediaSoundComponent->SetVolumeMultiplier(FMath::Clamp(Volume, 0.0f, 1.0f));
            BootMediaSoundComponent->RegisterComponentWithWorld(GetWorld());
        }
    }

    const bool bOpenRequested = BootMediaPlayer->OpenSource(BootMediaSource);
    if (!bOpenRequested)
    {
        UE_LOG(LogStageZeroLoadingWidget, Warning, TEXT("StageZero: Failed to open boot movie source: %s"), *ResolvedPath);
        StopBootMovie();
        OnBootMovieFailed(MoviePath);
    }
}

bool UStageZeroLoadingWidget::SkipBootMovie()
{
    if (!CanSkipBootMovie())
    {
        return false;
    }

    const FString SkippedMoviePath = ActiveBootMoviePath;
    StopBootMovie(bRestoreSlidesAfterBootMovie);
    OnBootMovieSkipped(SkippedMoviePath);
    return true;
}

void UStageZeroLoadingWidget::SetLoadingIconTexture(UTexture2D* Texture)
{
    if (LoadingIconImage)
    {
        if (Texture)
        {
            FSlateBrush Brush;
            Brush.SetResourceObject(Texture);
            Brush.ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
            LoadingIconImage->SetBrush(Brush);
        }
        LoadingIconImage->SetVisibility(Texture ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    if (Texture == nullptr)
    {
        SetSpinnerVisibility(false);
    }
}

FString UStageZeroLoadingWidget::ResolveBootMovieFilePath(const FString& MoviePath) const
{
    FString NormalizedPath = StageZeroNormalizeMoviePathInput(MoviePath);
    if (NormalizedPath.IsEmpty())
    {
        return FString();
    }

    if (FPaths::FileExists(NormalizedPath))
    {
        return FPaths::ConvertRelativePathToFull(NormalizedPath);
    }

    const FString MoviesDir = FPaths::ProjectContentDir() / TEXT("Movies");
    const FString CandidateDirect = MoviesDir / NormalizedPath;
    if (FPaths::FileExists(CandidateDirect))
    {
        return FPaths::ConvertRelativePathToFull(CandidateDirect);
    }

    if (!FPaths::GetExtension(NormalizedPath).IsEmpty())
    {
        return FString();
    }

    static const TCHAR* SupportedExtensions[] =
    {
        TEXT("mp4"),
        TEXT("webm"),
        TEXT("mov"),
        TEXT("avi"),
        TEXT("bk2")
    };

    for (const TCHAR* Extension : SupportedExtensions)
    {
        const FString CandidateWithExtension = MoviesDir / (NormalizedPath + TEXT(".") + Extension);
        if (FPaths::FileExists(CandidateWithExtension))
        {
            return FPaths::ConvertRelativePathToFull(CandidateWithExtension);
        }
    }

    return FString();
}

bool UStageZeroLoadingWidget::CanSkipBootMovie() const
{
    if (!bBootMovieSkippable || !BootMediaPlayer || ActiveBootMoviePath.IsEmpty())
    {
        return false;
    }

    const double NowSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : FPlatformTime::Seconds();
    return (NowSeconds - BootMovieStartTimeSeconds) >= static_cast<double>(BootMovieSkipDelaySeconds);
}

UTextBlock* UStageZeroLoadingWidget::GetBootMovieSkipPrompt()
{
    if (BootMovieSkipTextBlock)
    {
        return BootMovieSkipTextBlock;
    }

    if (DynamicBootMovieSkipTextBlock)
    {
        return DynamicBootMovieSkipTextBlock;
    }

    if (!WidgetTree || !WidgetTree->RootWidget)
    {
        return nullptr;
    }

    UPanelWidget* RootPanel = Cast<UPanelWidget>(WidgetTree->RootWidget);
    if (!RootPanel || (!RootPanel->IsA<UCanvasPanel>() && !RootPanel->IsA<UOverlay>()))
    {
        return nullptr;
    }

    DynamicBootMovieSkipTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StageZeroBootMovieSkipPrompt"));
    if (!DynamicBootMovieSkipTextBlock)
    {
        return nullptr;
    }

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 0
    FSlateFontInfo FontInfo = DynamicBootMovieSkipTextBlock->Font;
#else
    FSlateFontInfo FontInfo = DynamicBootMovieSkipTextBlock->GetFont();
#endif
    FontInfo.Size = 18;
    DynamicBootMovieSkipTextBlock->SetFont(FontInfo);
    DynamicBootMovieSkipTextBlock->SetJustification(ETextJustify::Center);
    DynamicBootMovieSkipTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.92f)));
    DynamicBootMovieSkipTextBlock->SetShadowOffset(FVector2D(0.0f, 1.0f));
    DynamicBootMovieSkipTextBlock->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
    DynamicBootMovieSkipTextBlock->SetVisibility(ESlateVisibility::Collapsed);

    UPanelSlot* AddedSlot = RootPanel->AddChild(DynamicBootMovieSkipTextBlock);
    if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(AddedSlot))
    {
        CanvasSlot->SetAnchors(FAnchors(0.5f, 1.0f));
        CanvasSlot->SetAlignment(FVector2D(0.5f, 1.0f));
        CanvasSlot->SetAutoSize(true);
        CanvasSlot->SetPosition(FVector2D(0.0f, -44.0f));
    }
    else if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(AddedSlot))
    {
        OverlaySlot->SetHorizontalAlignment(HAlign_Center);
        OverlaySlot->SetVerticalAlignment(VAlign_Bottom);
        OverlaySlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 44.0f));
    }

    return DynamicBootMovieSkipTextBlock;
}

void UStageZeroLoadingWidget::ShowBootMovieSkipPrompt()
{
    UTextBlock* PromptTextBlock = GetBootMovieSkipPrompt();
    if (!PromptTextBlock)
    {
        return;
    }

    const bool bShouldShowPrompt = bBootMovieSkippable && BootMediaPlayer && !BootMovieSkipText.IsEmpty();
    if (bShouldShowPrompt)
    {
        PromptTextBlock->SetText(BootMovieSkipText);
        PromptTextBlock->SetVisibility(ESlateVisibility::Visible);
    }
    else
    {
        PromptTextBlock->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UStageZeroLoadingWidget::HideBootMovieSkipPrompt()
{
    UTextBlock* PromptTextBlock = BootMovieSkipTextBlock ? BootMovieSkipTextBlock : DynamicBootMovieSkipTextBlock.Get();
    if (PromptTextBlock)
    {
        PromptTextBlock->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UStageZeroLoadingWidget::StopBootMovie(bool bRestoreSlides)
{
    const bool bShouldRestoreSlides = bRestoreSlides && bBootMovieUsingBackgroundImage && SlideTextures.Num() > 0;
    UImage* DedicatedMovieImage = BootMovieImage;

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(BootMovieSkipPromptTimerHandle);
    }
    HideBootMovieSkipPrompt();

    if (BootMediaPlayer)
    {
        BootMediaPlayer->OnMediaOpened.RemoveDynamic(this, &UStageZeroLoadingWidget::HandleBootMediaOpened);
        BootMediaPlayer->OnMediaOpenFailed.RemoveDynamic(this, &UStageZeroLoadingWidget::HandleBootMediaOpenFailed);
        BootMediaPlayer->OnEndReached.RemoveDynamic(this, &UStageZeroLoadingWidget::HandleBootMediaEndReached);
        BootMediaPlayer->Close();
    }

    if (BootMediaSoundComponent)
    {
        BootMediaSoundComponent->DestroyComponent();
        BootMediaSoundComponent = nullptr;
    }

    BootMediaSource = nullptr;
    BootMediaTexture = nullptr;
    BootMediaPlayer = nullptr;
    ActiveBootMoviePath.Reset();
    BootMovieStartTimeSeconds = 0.0;
    bBootMovieUsingBackgroundImage = false;

    if (DedicatedMovieImage)
    {
        DedicatedMovieImage->SetVisibility(ESlateVisibility::Collapsed);
    }
    else if (bShouldRestoreSlides)
    {
        CurrentSlideIndex = 0;
        AdvanceBackgroundSlide();
        StartBackgroundSlideshow();
    }
}

FReply UStageZeroLoadingWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (SkipBootMovie())
    {
        return FReply::Handled();
    }

    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UStageZeroLoadingWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (SkipBootMovie())
    {
        return FReply::Handled();
    }

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UStageZeroLoadingWidget::NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
    if (SkipBootMovie())
    {
        return FReply::Handled();
    }

    return Super::NativeOnTouchStarted(InGeometry, InGestureEvent);
}

void UStageZeroLoadingWidget::HandleBootMediaOpened(FString OpenedUrl)
{
    UE_LOG(LogStageZeroLoadingWidget, Display, TEXT("StageZero: Boot movie started: %s"), *OpenedUrl);
    OnBootMovieStarted(ActiveBootMoviePath);
}

void UStageZeroLoadingWidget::HandleBootMediaOpenFailed(FString FailedUrl)
{
    UE_LOG(LogStageZeroLoadingWidget, Warning, TEXT("StageZero: Boot movie failed to open: %s"), *FailedUrl);
    const FString FailedMoviePath = ActiveBootMoviePath;
    StopBootMovie();

    OnBootMovieFailed(FailedMoviePath);
}

void UStageZeroLoadingWidget::HandleBootMediaEndReached()
{
    if (bBootMovieLoop && BootMediaPlayer)
    {
        BootMediaPlayer->Rewind();
        BootMediaPlayer->Play();
        return;
    }

    const FString FinishedMoviePath = ActiveBootMoviePath;
    StopBootMovie(bRestoreSlidesAfterBootMovie);
    OnBootMovieFinished(FinishedMoviePath);
}

void UStageZeroLoadingWidget::SetSpinnerVisibility(bool bVisible)
{
    if (SpinnerContainer)
    {
        SpinnerContainer->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
    else if (LoadingIconImage)
    {
        LoadingIconImage->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    OnSpinnerVisibilityChanged(bVisible);
}

void UStageZeroLoadingWidget::SetProgressVisibility(bool bVisible)
{
    if (ProgressContainer)
    {
        ProgressContainer->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    OnProgressVisibilityChanged(bVisible);
}

void UStageZeroLoadingWidget::SetTip(const FText& InTip)
{
    if (TipText)
    {
        TipText->SetVisibility(!InTip.IsEmpty() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        TipText->SetText(InTip);
    }
    OnTipChanged(InTip);
}

void UStageZeroLoadingWidget::BindToStageZeroSubsystem(UStageZeroSubsystem* Subsystem)
{
    if (!IsValid(Subsystem))
    {
        return;
    }

    if (CachedSubsystem.IsValid() && CachedSubsystem.Get() != Subsystem)
    {
        UnbindFromStageZeroSubsystem();
    }

    CachedSubsystem = Subsystem;
    Subsystem->OnWarmupProgress.RemoveDynamic(this, &UStageZeroLoadingWidget::HandleSubsystemProgress);
    Subsystem->OnWarmupFinishedDetailed.RemoveDynamic(this, &UStageZeroLoadingWidget::HandleSubsystemFinishedDetailed);
    Subsystem->OnWarmupProgress.AddDynamic(this, &UStageZeroLoadingWidget::HandleSubsystemProgress);
    Subsystem->OnWarmupFinishedDetailed.AddDynamic(this, &UStageZeroLoadingWidget::HandleSubsystemFinishedDetailed);

    const FStageZeroWarmupStatus Status = Subsystem->GetWarmupStatus();
    if (StageZeroIsTerminalWarmupResult(Status.Result) && !Subsystem->IsWarmupRunning())
    {
        SetProgressVisibility(false);
        SetSpinnerVisibility(false);
        if (bHideAfterWarmup)
        {
            RemoveFromParent();
        }
        return;
    }

    const int32 SafeMapTotal = Status.CurrentMapTotal > 0 ? Status.CurrentMapTotal : FMath::Max(1, Status.TotalTasks);
    const int32 SafeMapCompleted = Status.CurrentMapTotal > 0
        ? Status.CurrentMapCompleted
        : FMath::Clamp(Status.CompletedTasks, 0, SafeMapTotal);

    HandleSubsystemProgress(
        Status.Progress,
        Status.CurrentMap,
        Status.CompletedTasks,
        Status.TotalTasks,
        SafeMapCompleted,
        SafeMapTotal);

    if (Status.Result != EStageZeroWarmupResult::NotStarted && Status.Result != EStageZeroWarmupResult::Running)
    {
        HandleSubsystemFinishedDetailed(Status.Result, Status);
    }
}

void UStageZeroLoadingWidget::UnbindFromStageZeroSubsystem()
{
    if (!CachedSubsystem.IsValid())
    {
        CachedSubsystem.Reset();
        return;
    }

    UStageZeroSubsystem* Subsystem = CachedSubsystem.Get();
    Subsystem->OnWarmupProgress.RemoveDynamic(this, &UStageZeroLoadingWidget::HandleSubsystemProgress);
    Subsystem->OnWarmupFinished.RemoveDynamic(this, &UStageZeroLoadingWidget::HandleSubsystemFinished);
    Subsystem->OnWarmupFinishedDetailed.RemoveDynamic(this, &UStageZeroLoadingWidget::HandleSubsystemFinishedDetailed);
    Subsystem->UnregisterLoadingWidget(this);
    CachedSubsystem.Reset();
}

void UStageZeroLoadingWidget::HandleSubsystemProgress(float Percent, FName MapName, int32 CompletedTasks, int32 TotalTasks, int32 MapCompleted, int32 MapTotal)
{
    if (ProgressBar)
    {
        ProgressBar->SetPercent(Percent);
    }

    if (ProgressText)
    {
        const int32 SafeTotal = FMath::Max(1, TotalTasks);
        const int32 SafeMapTotal = FMath::Max(1, MapTotal);
        const int32 PercentInt = FMath::Clamp(FMath::RoundToInt(Percent * 100.0f), 0, 100);

        FFormatNamedArguments Args;
        Args.Add(TEXT("Percent"), PercentInt);
        Args.Add(TEXT("Completed"), FStageZeroNumberFormatting::FormatConfiguredCountText(CompletedTasks));
        Args.Add(TEXT("Total"), FStageZeroNumberFormatting::FormatConfiguredCountText(SafeTotal));
        Args.Add(TEXT("MapCompleted"), FStageZeroNumberFormatting::FormatConfiguredCountText(MapCompleted));
        Args.Add(TEXT("MapTotal"), FStageZeroNumberFormatting::FormatConfiguredCountText(SafeMapTotal));
        Args.Add(TEXT("MapName"), FText::FromName(MapName));

        const bool bHasMapName = !MapName.IsNone();
        const FText Label = bHasMapName
            ? FText::Format(LOCTEXT("StageZeroProgressMap", "{Percent}% ({Completed}/{Total}) - {MapName} ({MapCompleted}/{MapTotal})"), Args)
            : FText::Format(LOCTEXT("StageZeroProgress", "{Percent}% ({Completed}/{Total})"), Args);

        ProgressText->SetText(Label);
    }

    OnShaderProgress(Percent, CompletedTasks, TotalTasks);
}

void UStageZeroLoadingWidget::HandleSubsystemFinished(bool bWasCancelled)
{
    StopBootMovie();

    if (CachedSubsystem.IsValid())
    {
        const FStageZeroWarmupStatus Status = CachedSubsystem->GetWarmupStatus();
        HandleSubsystemFinishedDetailed(Status.Result, Status);
        return;
    }

    const FString Message = bWasCancelled ? TEXT("Stage Zero warmup was cancelled or failed.") : TEXT("Stage Zero warmup completed.");
    if (ProgressText)
    {
        ProgressText->SetText(FText::FromString(Message));
    }
    OnCompletionMessage(Message);

    if (!bWasCancelled && bHideAfterWarmup)
    {
        RemoveFromParent();
    }
}

void UStageZeroLoadingWidget::HandleSubsystemFinishedDetailed(EStageZeroWarmupResult Result, FStageZeroWarmupStatus Status)
{
    StopBootMovie();

    const bool bSucceeded = Result == EStageZeroWarmupResult::Succeeded;
    const bool bCompletedEnoughToContinue = bSucceeded || Result == EStageZeroWarmupResult::Partial;
    const float DisplayProgress = bCompletedEnoughToContinue ? 1.0f : FMath::Clamp(Status.Progress, 0.0f, 1.0f);
    HandleSubsystemProgress(
        DisplayProgress,
        Status.CurrentMap,
        Status.CompletedTasks,
        FMath::Max(1, Status.TotalTasks),
        Status.CurrentMapCompleted,
        FMath::Max(1, Status.CurrentMapTotal));

    FString Message = Status.StatusMessage;
    if (Message.IsEmpty())
    {
        switch (Result)
        {
        case EStageZeroWarmupResult::Succeeded:
            Message = TEXT("Stage Zero warmup completed.");
            break;
        case EStageZeroWarmupResult::Cancelled:
            Message = TEXT("Stage Zero warmup was cancelled.");
            break;
        case EStageZeroWarmupResult::TimedOut:
            Message = TEXT("Stage Zero warmup timed out.");
            break;
        case EStageZeroWarmupResult::Partial:
            Message = TEXT("Stage Zero warmup completed with skipped or incomplete work.");
            break;
        case EStageZeroWarmupResult::Failed:
            Message = TEXT("Stage Zero warmup failed.");
            break;
        default:
            Message = TEXT("Stage Zero warmup finished.");
            break;
        }
    }

    if (ProgressText)
    {
        ProgressText->SetText(FText::FromString(Message));
    }
    OnCompletionMessage(Message);

    if (bCompletedEnoughToContinue && bHideAfterWarmup)
    {
        RemoveFromParent();
    }
}

void UStageZeroLoadingWidget::StartBackgroundSlideshow()
{
    if (!GetWorld() || SlideTextures.Num() == 0)
    {
        return;
    }

    GetWorld()->GetTimerManager().SetTimer(
        SlideTimerHandle,
        this,
        &UStageZeroLoadingWidget::AdvanceBackgroundSlide,
        SlideInterval,
        true);
}

void UStageZeroLoadingWidget::AdvanceBackgroundSlide()
{
    if (SlideTextures.Num() == 0)
    {
        return;
    }

    int32 Attempts = 0;
    const int32 MaxAttempts = SlideTextures.Num();
    UTexture2D* TextureToUse = nullptr;

    while (Attempts < MaxAttempts && !TextureToUse)
    {
        if (SlideTextures.IsValidIndex(CurrentSlideIndex))
        {
            TextureToUse = SlideTextures[CurrentSlideIndex].LoadSynchronous();
        }

        if (TextureToUse)
        {
            break;
        }

        CurrentSlideIndex = (CurrentSlideIndex + 1) % SlideTextures.Num();
        Attempts++;
    }

    if (BackgroundImage)
    {
        if (TextureToUse)
        {
            BackgroundImage->SetBrushFromTexture(TextureToUse, true);
            BackgroundImage->SetColorAndOpacity(FLinearColor::White);
            BackgroundImage->SetVisibility(ESlateVisibility::Visible);
            BackgroundImage->SetRenderOpacity(0.0f);
            if (GetWorld())
            {
                GetWorld()->GetTimerManager().ClearTimer(SlideFadeTimerHandle);
                SlideFadeStartTime = GetWorld()->GetTimeSeconds();
                if (SlideFadeDuration <= KINDA_SMALL_NUMBER)
                {
                    BackgroundImage->SetRenderOpacity(1.0f);
                }
                else
                {
                    GetWorld()->GetTimerManager().SetTimer(
                        SlideFadeTimerHandle,
                        this,
                        &UStageZeroLoadingWidget::TickBackgroundFade,
                        0.02f,
                        true);
                }
            }
            else
            {
                BackgroundImage->SetRenderOpacity(1.0f);
            }
        }
        else
        {
            UE_LOG(LogStageZeroLoadingWidget, Warning, TEXT("StageZero: Failed to load any configured boot background slide for %s."), *GetName());
            if (GetWorld())
            {
                GetWorld()->GetTimerManager().ClearTimer(SlideTimerHandle);
            }
            BackgroundImage->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    OnBackgroundChanged(TextureToUse);

    if (SlideTextures.Num() > 0)
    {
        if (CurrentSlideIndex + 1 < SlideTextures.Num())
        {
            CurrentSlideIndex++;
        }
        else if (bLoopSlides)
        {
            CurrentSlideIndex = 0;
        }
        else
        {
            if (GetWorld())
            {
                GetWorld()->GetTimerManager().ClearTimer(SlideTimerHandle);
            }
        }
    }
}

void UStageZeroLoadingWidget::StartTipRotation()
{
    if (!GetWorld() || TipList.Num() == 0)
    {
        return;
    }

    GetWorld()->GetTimerManager().SetTimer(
        TipTimerHandle,
        this,
        &UStageZeroLoadingWidget::AdvanceTip,
        TipInterval,
        true);
}

void UStageZeroLoadingWidget::AdvanceTip()
{
    if (TipList.Num() == 0)
    {
        return;
    }

    int32 Attempts = 0;
    const int32 MaxAttempts = TipList.Num();
    FText TipToUse;

    while (Attempts < MaxAttempts)
    {
        if (TipList.IsValidIndex(CurrentTipIndex))
        {
            TipToUse = TipList[CurrentTipIndex];
        }

        CurrentTipIndex = (CurrentTipIndex + 1) % TipList.Num();

        if (!TipToUse.IsEmpty())
        {
            break;
        }
        Attempts++;
    }

    if (!TipToUse.IsEmpty())
    {
        SetTip(TipToUse);
    }
}

void UStageZeroLoadingWidget::TickBackgroundFade()
{
    if (!BackgroundImage || !GetWorld())
    {
        return;
    }

    if (SlideFadeDuration <= KINDA_SMALL_NUMBER)
    {
        BackgroundImage->SetRenderOpacity(1.0f);
        GetWorld()->GetTimerManager().ClearTimer(SlideFadeTimerHandle);
        return;
    }

    const double Now = GetWorld()->GetTimeSeconds();
    const float Alpha = FMath::Clamp(static_cast<float>((Now - SlideFadeStartTime) / SlideFadeDuration), 0.0f, 1.0f);
    BackgroundImage->SetRenderOpacity(Alpha);

    if (Alpha >= 1.0f - KINDA_SMALL_NUMBER)
    {
        BackgroundImage->SetRenderOpacity(1.0f);
        GetWorld()->GetTimerManager().ClearTimer(SlideFadeTimerHandle);
    }
}

void UStageZeroLoadingWidget::ClearTimers()
{
    if (GetWorld())
    {
        FTimerManager& TimerManager = GetWorld()->GetTimerManager();
        TimerManager.ClearTimer(SlideTimerHandle);
        TimerManager.ClearTimer(TipTimerHandle);
        TimerManager.ClearTimer(SlideFadeTimerHandle);
        TimerManager.ClearTimer(BootMovieSkipPromptTimerHandle);
    }
}

void UStageZeroLoadingWidget::NativeDestruct()
{
    UnbindFromStageZeroSubsystem();
    StopBootMovie(false);
    ClearTimers();
    Super::NativeDestruct();
}

#undef LOCTEXT_NAMESPACE
