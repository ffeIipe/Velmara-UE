// Copyright (c) 2025 71 Studios for games Development. All rights reserved.

#include "StageZeroLoader.h"

#include "StageZeroProjectSettings.h"
#include "StageZeroSubsystem.h"
#include "Components/AudioComponent.h"
#include "CoreGlobals.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Sound/SoundBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogStageZeroLoader, Log, All);

AStageZeroLoader::AStageZeroLoader()
{
    PrimaryActorTick.bCanEverTick = false;
}

UStageZeroSubsystem* AStageZeroLoader::GetStageZeroSubsystem() const
{
    const UWorld* World = GetWorld();
    UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
    return GameInstance ? GameInstance->GetSubsystem<UStageZeroSubsystem>() : nullptr;
}

void AStageZeroLoader::BindSubsystemDelegates(UStageZeroSubsystem* Subsystem)
{
    if (!Subsystem)
    {
        return;
    }

    UStageZeroSubsystem* PreviouslyBoundSubsystem = BoundSubsystem.Get();
    if (PreviouslyBoundSubsystem != Subsystem)
    {
        if (PreviouslyBoundSubsystem)
        {
            PreviouslyBoundSubsystem->OnWarmupStarted.RemoveDynamic(this, &AStageZeroLoader::HandleSubsystemStarted);
            PreviouslyBoundSubsystem->OnWarmupFinished.RemoveDynamic(this, &AStageZeroLoader::HandleSubsystemFinished);
        }

        StopBackgroundMusic(true);
        bObservedSubsystemWarmup = false;
    }

    BoundSubsystem = Subsystem;

    Subsystem->OnWarmupStarted.AddUniqueDynamic(this, &AStageZeroLoader::HandleSubsystemStarted);
    Subsystem->OnWarmupFinished.AddUniqueDynamic(this, &AStageZeroLoader::HandleSubsystemFinished);
}

void AStageZeroLoader::UnbindSubsystemDelegates()
{
    if (UStageZeroSubsystem* Subsystem = BoundSubsystem.Get())
    {
        Subsystem->OnWarmupStarted.RemoveDynamic(this, &AStageZeroLoader::HandleSubsystemStarted);
        Subsystem->OnWarmupFinished.RemoveDynamic(this, &AStageZeroLoader::HandleSubsystemFinished);
    }
    BoundSubsystem.Reset();
}

void AStageZeroLoader::HandleSubsystemStarted()
{
    if (bObservedSubsystemWarmup)
    {
        return;
    }

    bObservedSubsystemWarmup = true;
    InitializeBackgroundMusic();
    OnStageZeroStarted.Broadcast();
}

void AStageZeroLoader::HandleSubsystemFinished(bool /*bWasCancelled*/)
{
    StopBackgroundMusic(false);
    bObservedSubsystemWarmup = false;
    OnStageZeroCompleted.Broadcast();
}

void AStageZeroLoader::SynchronizeWithSubsystemState(UStageZeroSubsystem* Subsystem)
{
    if (Subsystem && Subsystem->IsWarmupRunning(false))
    {
        HandleSubsystemStarted();
        return;
    }

    StopBackgroundMusic(true);
    bObservedSubsystemWarmup = false;
}

void AStageZeroLoader::ApplyProjectSettings()
{
    if (bProjectSettingsApplied)
    {
        return;
    }
    bProjectSettingsApplied = true;

    const UStageZeroProjectSettings* Settings = GetDefault<UStageZeroProjectSettings>();
    const AStageZeroLoader* CDO = GetDefault<AStageZeroLoader>();
    if (!Settings || !CDO)
    {
        return;
    }

    auto ResolveBool = [](bool CurrentValue, bool ProjectValue, bool DefaultValue)
    {
        return CurrentValue == DefaultValue ? ProjectValue : CurrentValue;
    };

    bLoopBackgroundSlides = ResolveBool(bLoopBackgroundSlides, Settings->bLoopBackgroundSlides, CDO->bLoopBackgroundSlides);
    bShowSpinnerDuringProgress = ResolveBool(bShowSpinnerDuringProgress, Settings->bShowSpinnerDuringProgress, CDO->bShowSpinnerDuringProgress);

    if (LoadingIconTexture.IsNull() && !Settings->LoadingIconTexture.IsNull())
    {
        LoadingIconTexture = Settings->LoadingIconTexture;
    }
    if (BackgroundSlides.IsEmpty() && !Settings->BackgroundSlides.IsEmpty())
    {
        BackgroundSlides = Settings->BackgroundSlides;
    }
    if (BootTips.IsEmpty() && !Settings->BootTips.IsEmpty())
    {
        BootTips = Settings->BootTips;
    }
    if (!BackgroundMusic && !Settings->BackgroundMusic.IsNull())
    {
        BackgroundMusic = Settings->BackgroundMusic.LoadSynchronous();
    }

    if (FMath::IsNearlyEqual(BackgroundSlideInterval, CDO->BackgroundSlideInterval))
    {
        BackgroundSlideInterval = Settings->BackgroundSlideInterval;
    }
    if (FMath::IsNearlyEqual(BackgroundSlideFadeDuration, CDO->BackgroundSlideFadeDuration))
    {
        BackgroundSlideFadeDuration = Settings->BackgroundSlideFadeDuration;
    }
    if (FMath::IsNearlyEqual(BootTipInterval, CDO->BootTipInterval))
    {
        BootTipInterval = Settings->BootTipInterval;
    }
    if (FMath::IsNearlyEqual(MusicFadeInTime, CDO->MusicFadeInTime))
    {
        MusicFadeInTime = Settings->MusicFadeInTime;
    }
    if (FMath::IsNearlyEqual(MusicFadeOutTime, CDO->MusicFadeOutTime))
    {
        MusicFadeOutTime = Settings->MusicFadeOutTime;
    }
    if (FMath::IsNearlyEqual(SpinnerOnlyPhaseDuration, CDO->SpinnerOnlyPhaseDuration))
    {
        SpinnerOnlyPhaseDuration = Settings->SpinnerOnlyPhaseDuration;
    }
}

void AStageZeroLoader::BeginPlay()
{
    Super::BeginPlay();

    if (IsRunningCommandlet())
    {
        StopBackgroundMusic(true);
        bObservedSubsystemWarmup = false;
        return;
    }
#if WITH_EDITOR
    if (GetWorld() && !GetWorld()->IsGameWorld())
    {
        StopBackgroundMusic(true);
        bObservedSubsystemWarmup = false;
        return;
    }
#endif
    if (!ShouldRunOnThisNetMode())
    {
        StopBackgroundMusic(true);
        bObservedSubsystemWarmup = false;
        return;
    }

    ApplyProjectSettings();
    if (UStageZeroSubsystem* Subsystem = GetStageZeroSubsystem())
    {
        Subsystem->ApplyLegacyActorSettings(this);
        BindSubsystemDelegates(Subsystem);
        SynchronizeWithSubsystemState(Subsystem);
        if (bAutoStartStageZero)
        {
            if (!Subsystem->IsWarmupRunning(false))
            {
                Subsystem->StartWarmup(false);
            }
            SynchronizeWithSubsystemState(Subsystem);
        }
    }
    else
    {
        StopBackgroundMusic(true);
        bObservedSubsystemWarmup = false;
    }
}

void AStageZeroLoader::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UnbindSubsystemDelegates();
    StopBackgroundMusic(true);
    bObservedSubsystemWarmup = false;
    Super::EndPlay(EndPlayReason);
}

void AStageZeroLoader::InitializeBackgroundMusic()
{
    if (IsValid(MusicComponent.Get()))
    {
        return;
    }

    MusicComponent = nullptr;
    if (!BackgroundMusic)
    {
        return;
    }

    MusicComponent = UGameplayStatics::SpawnSound2D(this, BackgroundMusic);
    if (IsValid(MusicComponent.Get()))
    {
        MusicComponent->FadeIn(FMath::Max(0.0f, MusicFadeInTime));
    }
    else
    {
        MusicComponent = nullptr;
    }
}

void AStageZeroLoader::StopBackgroundMusic(bool bImmediate)
{
    UAudioComponent* Component = MusicComponent.Get();
    MusicComponent = nullptr;

    if (!IsValid(Component))
    {
        return;
    }

    const float FadeOutSeconds = bImmediate ? 0.0f : FMath::Max(0.0f, MusicFadeOutTime);
    if (FadeOutSeconds > 0.0f)
    {
        Component->FadeOut(FadeOutSeconds, 0.0f);
        return;
    }

    Component->Stop();
    Component->DestroyComponent();
}

bool AStageZeroLoader::ShouldRunOnThisNetMode() const
{
    const ENetMode NetMode = GetNetMode();
    return NetMode == NM_Standalone || NetMode == NM_Client;
}

void AStageZeroLoader::SetLevelTargets(TSoftObjectPtr<UWorld> InPrimary, const TArray<TSoftObjectPtr<UWorld>>& InWarmupMaps)
{
    PrimaryLevel = InPrimary;
    MapsToWarmUp = InWarmupMaps;
    if (UStageZeroSubsystem* Subsystem = GetStageZeroSubsystem())
    {
        Subsystem->SetPrimaryLevel(InPrimary);
        Subsystem->SetWarmupMaps(InWarmupMaps);
    }
}

void AStageZeroLoader::StartStageZero()
{
    ApplyProjectSettings();
    if (!ShouldRunOnThisNetMode())
    {
        StopBackgroundMusic(true);
        bObservedSubsystemWarmup = false;
        return;
    }

    UStageZeroSubsystem* Subsystem = GetStageZeroSubsystem();
    if (!Subsystem)
    {
        StopBackgroundMusic(true);
        bObservedSubsystemWarmup = false;
        UE_LOG(LogStageZeroLoader, Warning, TEXT("StageZeroSubsystem is unavailable; the compatibility actor cannot start warmup."));
        return;
    }

    Subsystem->ApplyLegacyActorSettings(this);
    BindSubsystemDelegates(Subsystem);
    SynchronizeWithSubsystemState(Subsystem);
    if (!Subsystem->IsWarmupRunning(false))
    {
        Subsystem->StartWarmup(false);
    }
    SynchronizeWithSubsystemState(Subsystem);
}

void AStageZeroLoader::SetPrimaryLevelBP(TSoftObjectPtr<UWorld> InPrimary)
{
    PrimaryLevel = InPrimary;
    if (UStageZeroSubsystem* Subsystem = GetStageZeroSubsystem())
    {
        Subsystem->SetPrimaryLevel(InPrimary);
    }
}

void AStageZeroLoader::SetWarmupMapsBP(const TArray<TSoftObjectPtr<UWorld>>& InWarmupMaps)
{
    MapsToWarmUp = InWarmupMaps;
    if (UStageZeroSubsystem* Subsystem = GetStageZeroSubsystem())
    {
        Subsystem->SetWarmupMaps(InWarmupMaps);
    }
}

void AStageZeroLoader::AddWarmupMapBP(TSoftObjectPtr<UWorld> InWarmupMap)
{
    if (InWarmupMap.IsNull())
    {
        return;
    }

    MapsToWarmUp.AddUnique(InWarmupMap);
    if (UStageZeroSubsystem* Subsystem = GetStageZeroSubsystem())
    {
        Subsystem->AddWarmupMap(InWarmupMap);
    }
}

void AStageZeroLoader::ClearWarmupMapsBP()
{
    MapsToWarmUp.Reset();
    if (UStageZeroSubsystem* Subsystem = GetStageZeroSubsystem())
    {
        Subsystem->ClearWarmupMaps();
    }
}

void AStageZeroLoader::SetWorldPartitionWarmAllCells(bool bEnable)
{
    bWarmAllWorldPartitionCells = bEnable;
    UE_LOG(LogStageZeroLoader, Warning,
        TEXT("SetWorldPartitionWarmAllCells is deprecated and has no runtime effect. Stage Zero no longer streams or renders hidden maps during warmup."));
}

void AStageZeroLoader::SetMinimumDisplaySeconds(float InSeconds)
{
    MinimumDisplaySeconds = FMath::Max(0.0f, InSeconds);
    if (UStageZeroSubsystem* Subsystem = GetStageZeroSubsystem())
    {
        Subsystem->ApplyLegacyActorSettings(this);
    }
}

void AStageZeroLoader::CreateLoadingWidget()
{
    // Kept for Blueprint compatibility. The subsystem owns both presentation and warmup.
    StartStageZero();
}

float AStageZeroLoader::GetStageZeroProgress() const
{
    const UStageZeroSubsystem* Subsystem = GetStageZeroSubsystem();
    return Subsystem ? Subsystem->GetWarmupProgress(false) : 0.0f;
}

void AStageZeroLoader::GetStageZeroProgressCounts(int32& OutCompleted, int32& OutTotal) const
{
    const UStageZeroSubsystem* Subsystem = GetStageZeroSubsystem();
    if (Subsystem)
    {
        Subsystem->GetTaskCounts(OutCompleted, OutTotal, false);
        return;
    }

    OutCompleted = 0;
    OutTotal = 0;
}

float AStageZeroLoader::GetShaderProgress() const
{
    const UStageZeroSubsystem* Subsystem = GetStageZeroSubsystem();
    if (!Subsystem)
    {
        return 0.0f;
    }

    const FStageZeroWarmupStatus Status = Subsystem->GetWarmupStatus(false);
    if (!Status.bPipelineCacheAccepted)
    {
        // Asset preload progress is not shader or PSO progress.
        return 0.0f;
    }
    if (Status.PipelineCacheTotal <= 0)
    {
        return Status.PipelineCacheRemaining <= 0 ? 1.0f : 0.0f;
    }
    return FMath::Clamp(
        static_cast<float>(Status.PipelineCacheCompleted) / static_cast<float>(Status.PipelineCacheTotal),
        0.0f,
        1.0f);
}

float AStageZeroLoader::GetTotalProgress() const
{
    return GetStageZeroProgress();
}

void AStageZeroLoader::ForceTransitionNow()
{
    if (!ShouldRunOnThisNetMode() || PrimaryLevel.IsNull())
    {
        return;
    }

    const FString PackageName = PrimaryLevel.ToSoftObjectPath().GetLongPackageName();
    if (PackageName.IsEmpty())
    {
        UE_LOG(LogStageZeroLoader, Warning, TEXT("Cannot force transition because the primary level is invalid."));
        return;
    }

    if (UStageZeroSubsystem* Subsystem = GetStageZeroSubsystem())
    {
        Subsystem->ApplyLegacyActorSettings(this);
        Subsystem->SetPrimaryLevel(PrimaryLevel, false);
        Subsystem->CancelWarmup(false);
        Subsystem->OpenLevelWithoutTravelLoadingScreen(FName(*PackageName), true, FString(), false);
        return;
    }

    // This deprecated debug API historically meant immediate travel. Keep that
    // exact behavior when no subsystem exists to mediate the compatibility event.
    UGameplayStatics::OpenLevel(this, FName(*PackageName), true);
}
