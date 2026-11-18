// Copyright (c) 2025 71 Studios. All rights reserved.

#include "StageZeroSubsystem.h"
#include "StageZeroLoader.h"
#include "StageZeroLoadingWidget.h"
#include "StageZeroNativeLoadingScreenProvider.h"
#include "StageZeroProjectSettings.h"
#include "StageZeroWarmupManifest.h"
#include "StageZeroWarmupUtils.h"
#include "StageZeroPipelineCacheObserver.h"
#include "Components/AudioComponent.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
#include "Blueprint/UserWidget.h"
#include "Misc/App.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "MoviePlayer.h"
#include "RHI.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Sound/SoundBase.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#if __has_include("ShaderPipelineCache.h")
#include "ShaderPipelineCache.h"
#define STAGEZERO_HAS_SHADER_PIPELINE_CACHE 1
#else
#define STAGEZERO_HAS_SHADER_PIPELINE_CACHE 0
#endif
#include "UObject/SoftObjectPath.h"

DEFINE_LOG_CATEGORY_STATIC(LogStageZeroSubsystem, Log, All);

namespace
{
constexpr float StageZeroManifestBatchLoadTimeoutSeconds = 30.0f;

FString GetExpectedPipelineCacheBaseName(bool bUserCache)
{
    // Keep this identity calculation aligned with FShaderPipelineCache's
    // public Open* entrypoints (UE 5.3-5.8). Cache lifecycle evidence is only
    // accepted for the project/user identities those entrypoints select.
    FString Name = FApp::GetProjectName();
    const TCHAR* OverrideKey = bUserCache
        ? TEXT("UserCacheFileNameOverride")
        : TEXT("CacheFileNameOverride");
    FString OverrideName;
    if (GConfig
        && (GConfig->GetString(TEXT("ShaderPipelineCache.CacheFile"), OverrideKey, OverrideName, GGameUserSettingsIni)
            || GConfig->GetString(TEXT("ShaderPipelineCache.CacheFile"), OverrideKey, OverrideName, GGameIni))
        && !OverrideName.IsEmpty())
    {
        Name = OverrideName;
    }
    return Name;
}

static TSoftObjectPtr<UWorld> SZ_Subsystem_MapFromPackageName(const FName& PackageName)
{
    const FString LongPackageName = PackageName.ToString();
    if (LongPackageName.IsEmpty())
    {
        return TSoftObjectPtr<UWorld>();
    }

    const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *LongPackageName, *FPackageName::GetShortName(LongPackageName));
    return TSoftObjectPtr<UWorld>(FSoftObjectPath(ObjectPath));
}
void StageZeroDebugMessage(const UObject* Context, bool bShowDebug, const FString& Message, const FColor& Color = FColor::Green)
{
    if (!bShowDebug || Message.IsEmpty())
    {
        return;
    }

    UE_LOG(LogStageZeroSubsystem, Display, TEXT("%s"), *Message);
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, Color, Message);
    }
}

FString GetPackageNameFromMap(const TSoftObjectPtr<UWorld>& Map)
{
    const FSoftObjectPath Path = Map.ToSoftObjectPath();
    return Path.IsValid() ? Path.GetLongPackageName() : FString();
}

FName GetMapNameFromPath(const FString& PackageName)
{
    return PackageName.IsEmpty() ? NAME_None : FName(*FPackageName::GetShortName(PackageName));
}

FString NormalizeMapPackageForComparison(FString PackageName)
{
    PackageName.TrimStartAndEndInline();
    PackageName.ReplaceInline(TEXT("\\"), TEXT("/"));
    if (PackageName.IsEmpty())
    {
        return PackageName;
    }

    if (FPackageName::IsValidObjectPath(PackageName))
    {
        PackageName = FPackageName::ObjectPathToPackageName(PackageName);
    }

    const FString GamePiePrefix(TEXT("/Game/UEDPIE_"));
    if (PackageName.StartsWith(GamePiePrefix, ESearchCase::IgnoreCase))
    {
        FString PieSuffix = PackageName.Mid(GamePiePrefix.Len());
        int32 InstanceSeparator = INDEX_NONE;
        if (PieSuffix.FindChar(TEXT('_'), InstanceSeparator) && InstanceSeparator + 1 < PieSuffix.Len())
        {
            PackageName = FString(TEXT("/Game/")) + PieSuffix.Mid(InstanceSeparator + 1);
        }
    }

    return PackageName;
}

FString GetWorldPackageNameForComparison(const UWorld* World)
{
    if (!World)
    {
        return FString();
    }

    if (World->PersistentLevel && World->PersistentLevel->GetOutermost())
    {
        return NormalizeMapPackageForComparison(World->PersistentLevel->GetOutermost()->GetName());
    }

    return World->GetOutermost() ? NormalizeMapPackageForComparison(World->GetOutermost()->GetName()) : FString();
}

bool DoesWorldMatchRequestedMap(const UWorld* World, const FString& RequestedMap)
{
    const FString WorldPackage = GetWorldPackageNameForComparison(World);
    const FString TargetPackage = NormalizeMapPackageForComparison(RequestedMap);
    if (WorldPackage.IsEmpty() || TargetPackage.IsEmpty())
    {
        return false;
    }

    if (WorldPackage.Equals(TargetPackage, ESearchCase::IgnoreCase))
    {
        return true;
    }

    const FString WorldShortName = FPackageName::GetShortName(WorldPackage);
    const FString TargetShortName = FPackageName::GetShortName(TargetPackage);
    return !WorldShortName.IsEmpty()
        && !TargetShortName.IsEmpty()
        && WorldShortName.Equals(TargetShortName, ESearchCase::IgnoreCase);
}

bool DoesMapPackageMatch(const FString& CandidateMap, const FString& RequestedMap)
{
    const FString CandidatePackage = NormalizeMapPackageForComparison(CandidateMap);
    const FString RequestedPackage = NormalizeMapPackageForComparison(RequestedMap);
    if (CandidatePackage.IsEmpty() || RequestedPackage.IsEmpty())
    {
        return false;
    }

    if (CandidatePackage.Equals(RequestedPackage, ESearchCase::IgnoreCase))
    {
        return true;
    }

    const FString CandidateShortName = FPackageName::GetShortName(CandidatePackage);
    const FString RequestedShortName = FPackageName::GetShortName(RequestedPackage);
    return !CandidateShortName.IsEmpty()
        && !RequestedShortName.IsEmpty()
        && CandidateShortName.Equals(RequestedShortName, ESearchCase::IgnoreCase);
}

bool IsBuiltInStageZeroBootMap(const FString& RequestedMap)
{
    const FString RequestedPackage = NormalizeMapPackageForComparison(RequestedMap);
    if (RequestedPackage.IsEmpty())
    {
        return false;
    }

    return DoesMapPackageMatch(TEXT("/StageZero/StageZero/StageZero-BootLevel"), RequestedPackage)
        || FPackageName::GetShortName(RequestedPackage).Equals(TEXT("StageZero-BootLevel"), ESearchCase::IgnoreCase);
}

EStageZeroLoadingScreenPlatformProfile GetRuntimeLoadingScreenPlatformProfile()
{
#if PLATFORM_ANDROID || PLATFORM_IOS
    return EStageZeroLoadingScreenPlatformProfile::Mobile;
#elif PLATFORM_WINDOWS || PLATFORM_MAC || PLATFORM_LINUX
    return EStageZeroLoadingScreenPlatformProfile::Desktop;
#else
    return EStageZeroLoadingScreenPlatformProfile::Console;
#endif
}

bool DoesLoadingScreenPlatformProfileMatch(EStageZeroLoadingScreenPlatformProfile ProfilePlatform)
{
    return ProfilePlatform == EStageZeroLoadingScreenPlatformProfile::Any
        || ProfilePlatform == GetRuntimeLoadingScreenPlatformProfile();
}

bool DoesLoadingScreenProfileMapMatch(const FStageZeroLoadingScreenProfile& Profile, const FString& RequestedMap)
{
    const FSoftObjectPath ProfileMapPath = Profile.DestinationMap.ToSoftObjectPath();
    if (!ProfileMapPath.IsValid())
    {
        return RequestedMap.IsEmpty();
    }

    if (RequestedMap.IsEmpty())
    {
        return false;
    }

    const FString ProfilePackage = NormalizeMapPackageForComparison(ProfileMapPath.GetLongPackageName());
    const FString TargetPackage = NormalizeMapPackageForComparison(RequestedMap);
    if (ProfilePackage.IsEmpty() || TargetPackage.IsEmpty())
    {
        return false;
    }

    if (ProfilePackage.Equals(TargetPackage, ESearchCase::IgnoreCase))
    {
        return true;
    }

    return FPackageName::GetShortName(ProfilePackage).Equals(FPackageName::GetShortName(TargetPackage), ESearchCase::IgnoreCase);
}

FString GetWarmupCacheFilePath()
{
    return FPaths::ProjectSavedDir() / TEXT("StageZero") / TEXT("WarmupCache.cache");
}

FString GetLegacyWarmupCacheFilePath()
{
    return FPaths::ProjectSavedDir() / TEXT("StageZero") / TEXT("WarmupCache.txt");
}

void DeleteLegacyPlaintextWarmupCache()
{
    const FString DeprecatedPaths[] =
    {
        GetWarmupCacheFilePath(),
        GetLegacyWarmupCacheFilePath()
    };

    for (const FString& DeprecatedPath : DeprecatedPaths)
    {
        if (!IFileManager::Get().FileExists(*DeprecatedPath))
        {
            continue;
        }

        if (IFileManager::Get().Delete(*DeprecatedPath, false, true))
        {
            UE_LOG(LogStageZeroSubsystem, Log,
                TEXT("StageZero: Removed deprecated cross-process warmup cache: %s"),
                *DeprecatedPath);
        }
        else
        {
            UE_LOG(LogStageZeroSubsystem, Error,
                TEXT("StageZero: Failed to delete deprecated cross-process warmup cache: %s. The file remains ignored and cannot enable warmup skipping."),
                *DeprecatedPath);
        }
    }
}

bool IsLoadableManifestAssetPath(const FSoftObjectPath& Path)
{
    return FStageZeroWarmupUtils::IsRuntimeWarmupAssetPathAllowed(Path);
}

const TCHAR* StageZeroManifestJobTypeName(EStageZeroManifestJobType Type)
{
    switch (Type)
    {
        case EStageZeroManifestJobType::Material:
            return TEXT("Material");
        case EStageZeroManifestJobType::Texture:
            return TEXT("Texture");
        case EStageZeroManifestJobType::Niagara:
            return TEXT("Niagara");
        case EStageZeroManifestJobType::StaticMesh:
            return TEXT("StaticMesh");
        case EStageZeroManifestJobType::SkeletalMesh:
            return TEXT("SkeletalMesh");
        case EStageZeroManifestJobType::PSO:
            return TEXT("PSO");
        default:
            return TEXT("Unknown");
    }
}

static EVisibility StageZeroTextVisibility(const FText& Text)
{
    return Text.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
}

static EStretch::Type StageZeroTravelImageStretch(EStageZeroTravelImageSizeMode SizeMode)
{
    switch (SizeMode)
    {
        case EStageZeroTravelImageSizeMode::FitLetterbox:
            return EStretch::ScaleToFit;
        case EStageZeroTravelImageSizeMode::Stretch:
            return EStretch::Fill;
        case EStageZeroTravelImageSizeMode::OriginalCentered:
            return EStretch::None;
        case EStageZeroTravelImageSizeMode::FillCrop:
        default:
            return EStretch::ScaleToFill;
    }
}

static constexpr float StageZeroTravelDesignCanvasWidth = 1920.0f;
static constexpr float StageZeroTravelDesignCanvasHeight = 1080.0f;

class SStageZeroTravelLoadingScreen final : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SStageZeroTravelLoadingScreen)
        : _BackgroundColor(FLinearColor::Black)
        , _OverlayTint(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f))
        , _PrimaryTextColor(FLinearColor::White)
        , _SecondaryTextColor(FLinearColor(0.70f, 0.72f, 0.78f, 1.0f))
        , _AccentColor(FLinearColor(0.16f, 0.58f, 1.0f, 1.0f))
        , _LoadingScreenTemplate(EStageZeroTravelLoadingScreenTemplate::Classic)
        , _ImageSizeMode(EStageZeroTravelImageSizeMode::FillCrop)
        , _ImageIntervalSeconds(2.5f)
        , _TipIntervalSeconds(4.0f)
        , _bLoopImages(true)
        , _bAllowMovieBehindWidget(false)
        , _TitleFontSize(34)
        , _SubtitleFontSize(18)
        , _SmallTextFontSize(13)
        , _bShowSpinner(true)
        , _FadeInSeconds(0.0f)
        , _bReduceMotion(false)
    {}
        SLATE_ARGUMENT(TArray<UTexture2D*>, BackgroundTextures)
        SLATE_ARGUMENT(FText, TitleText)
        SLATE_ARGUMENT(FText, SubtitleText)
        SLATE_ARGUMENT(FText, StatusText)
        SLATE_ARGUMENT(FText, FooterText)
        SLATE_ARGUMENT(FLinearColor, BackgroundColor)
        SLATE_ARGUMENT(FLinearColor, OverlayTint)
        SLATE_ARGUMENT(FLinearColor, PrimaryTextColor)
        SLATE_ARGUMENT(FLinearColor, SecondaryTextColor)
        SLATE_ARGUMENT(FLinearColor, AccentColor)
        SLATE_ARGUMENT(EStageZeroTravelLoadingScreenTemplate, LoadingScreenTemplate)
        SLATE_ARGUMENT(EStageZeroTravelImageSizeMode, ImageSizeMode)
        SLATE_ARGUMENT(float, ImageIntervalSeconds)
        SLATE_ARGUMENT(TArray<FText>, TipTexts)
        SLATE_ARGUMENT(float, TipIntervalSeconds)
        SLATE_ARGUMENT(bool, bLoopImages)
        SLATE_ARGUMENT(bool, bAllowMovieBehindWidget)
        SLATE_ARGUMENT(int32, TitleFontSize)
        SLATE_ARGUMENT(int32, SubtitleFontSize)
        SLATE_ARGUMENT(int32, SmallTextFontSize)
        SLATE_ARGUMENT(bool, bShowSpinner)
        SLATE_ARGUMENT(float, FadeInSeconds)
        SLATE_ARGUMENT(bool, bReduceMotion)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        for (UTexture2D* BackgroundTexture : InArgs._BackgroundTextures)
        {
            if (!BackgroundTexture)
            {
                continue;
            }

            FSlateBrush& BackgroundBrush = BackgroundBrushes.AddDefaulted_GetRef();
            BackgroundBrush.DrawAs = ESlateBrushDrawType::Image;
            BackgroundBrush.SetResourceObject(BackgroundTexture);
            BackgroundBrush.ImageSize = FVector2D(StageZeroTravelDesignCanvasWidth, StageZeroTravelDesignCanvasHeight);
        }

        ImageIntervalSeconds = InArgs._bReduceMotion ? 0.0f : FMath::Max(0.0f, InArgs._ImageIntervalSeconds);
        TipIntervalSeconds = InArgs._bReduceMotion ? 0.0f : FMath::Max(0.0f, InArgs._TipIntervalSeconds);
        TipTexts = InArgs._TipTexts;
        TipTexts.RemoveAll([](const FText& Tip)
        {
            return Tip.IsEmpty();
        });
        bLoopImages = InArgs._bLoopImages;
        FadeInSeconds = InArgs._bReduceMotion ? 0.0f : FMath::Max(0.0f, InArgs._FadeInSeconds);
        if (FadeInSeconds > 0.0f)
        {
            SetRenderOpacity(0.0f);
        }

        const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
        const int32 TitleSize = FMath::Clamp(InArgs._TitleFontSize, 8, 96);
        const int32 SubtitleSize = FMath::Clamp(InArgs._SubtitleFontSize, 8, 64);
        const int32 SmallSize = FMath::Clamp(InArgs._SmallTextFontSize, 8, 48);
        const FLinearColor BackgroundColor = InArgs._bAllowMovieBehindWidget ? FLinearColor(0.0f, 0.0f, 0.0f, 0.0f) : InArgs._BackgroundColor;

        TSharedRef<SOverlay> Root = SNew(SOverlay);

        Root->AddSlot()
        .HAlign(HAlign_Fill)
        .VAlign(VAlign_Fill)
        [
            SNew(SBorder)
            .BorderImage(WhiteBrush)
            .BorderBackgroundColor(BackgroundColor)
        ];

        if (BackgroundBrushes.Num() > 0)
        {
            Root->AddSlot()
            .HAlign(HAlign_Fill)
            .VAlign(VAlign_Fill)
            [
                SNew(SScaleBox)
                .Stretch(StageZeroTravelImageStretch(InArgs._ImageSizeMode))
                .StretchDirection(EStretchDirection::Both)
                .HAlign(HAlign_Center)
                .VAlign(VAlign_Center)
                [
                    SNew(SImage)
                    .Image(this, &SStageZeroTravelLoadingScreen::GetCurrentBackgroundBrush)
                    .ColorAndOpacity(FLinearColor::White)
                ]
            ];
        }

        Root->AddSlot()
        .HAlign(HAlign_Fill)
        .VAlign(VAlign_Fill)
        [
            SNew(SBorder)
            .BorderImage(WhiteBrush)
            .BorderBackgroundColor(InArgs._OverlayTint)
        ];

        Root->AddSlot()
        .HAlign(HAlign_Fill)
        .VAlign(VAlign_Fill)
        [
            SNew(SScaleBox)
            .Stretch(EStretch::ScaleToFit)
            .StretchDirection(EStretchDirection::Both)
            [
                SNew(SBox)
                .WidthOverride(StageZeroTravelDesignCanvasWidth)
                .HeightOverride(StageZeroTravelDesignCanvasHeight)
                [
                    BuildTemplateContent(InArgs, WhiteBrush, TitleSize, SubtitleSize, SmallSize)
                ]
            ]
        ];

        ChildSlot
        [
            Root
        ];
    }

    virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override
    {
        SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

        if (FadeInSeconds > 0.0f)
        {
            if (FadeStartTime <= 0.0)
            {
                FadeStartTime = InCurrentTime;
            }

            const float FadeAlpha = FMath::Clamp(static_cast<float>((InCurrentTime - FadeStartTime) / FadeInSeconds), 0.0f, 1.0f);
            SetRenderOpacity(FadeAlpha);
        }

        if (TipTexts.Num() > 1 && TipIntervalSeconds > 0.0f)
        {
            if (LastTipSwitchTime <= 0.0)
            {
                LastTipSwitchTime = InCurrentTime;
            }
            else if ((InCurrentTime - LastTipSwitchTime) >= TipIntervalSeconds)
            {
                CurrentTipIndex = (CurrentTipIndex + 1) % TipTexts.Num();
                LastTipSwitchTime = InCurrentTime;
            }
        }

        if (BackgroundBrushes.Num() <= 1 || ImageIntervalSeconds <= 0.0f)
        {
            return;
        }

        if (LastImageSwitchTime <= 0.0)
        {
            LastImageSwitchTime = InCurrentTime;
            return;
        }

        if ((InCurrentTime - LastImageSwitchTime) < ImageIntervalSeconds)
        {
            return;
        }

        if (CurrentBackgroundIndex + 1 < BackgroundBrushes.Num())
        {
            ++CurrentBackgroundIndex;
        }
        else if (bLoopImages)
        {
            CurrentBackgroundIndex = 0;
        }

        LastImageSwitchTime = InCurrentTime;
    }

private:
    TSharedRef<SWidget> BuildAccentLine(const FSlateBrush* WhiteBrush, const FLinearColor& AccentColor, float Width, float Height) const
    {
        return SNew(SBox)
            .WidthOverride(Width)
            .HeightOverride(Height)
            [
                SNew(SBorder)
                .BorderImage(WhiteBrush)
                .BorderBackgroundColor(AccentColor)
            ];
    }

    TSharedRef<SWidget> BuildStatusRow(const FArguments& InArgs, int32 SmallSize) const
    {
        return SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(FMargin(0.0f, 0.0f, 12.0f, 0.0f))
            [
                SNew(SThrobber)
                .Visibility((InArgs._bShowSpinner && !InArgs._bReduceMotion) ? EVisibility::Visible : EVisibility::Collapsed)
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Visibility(StageZeroTextVisibility(InArgs._StatusText))
                .Text(InArgs._StatusText)
                .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), SmallSize))
                .ColorAndOpacity(InArgs._SecondaryTextColor)
            ];
    }

    TSharedRef<SWidget> BuildTipText(const FArguments& InArgs, int32 SmallSize, ETextJustify::Type Justification) const
    {
        if (TipTexts.Num() > 0)
        {
            return SNew(STextBlock)
                .Visibility(EVisibility::Visible)
                .Text(this, &SStageZeroTravelLoadingScreen::GetCurrentTipText)
                .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), SmallSize))
                .ColorAndOpacity(InArgs._SecondaryTextColor)
                .Justification(Justification)
                .AutoWrapText(true);
        }

        return SNew(STextBlock)
            .Visibility(StageZeroTextVisibility(InArgs._FooterText))
            .Text(InArgs._FooterText)
            .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), SmallSize))
            .ColorAndOpacity(InArgs._SecondaryTextColor)
            .Justification(Justification)
            .AutoWrapText(true);
    }

    TSharedRef<SWidget> BuildClassicContent(const FArguments& InArgs, const FSlateBrush* WhiteBrush, int32 TitleSize, int32 SubtitleSize, int32 SmallSize) const
    {
        return SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            [
                SNew(SSpacer)
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(FMargin(72.0f, 0.0f, 72.0f, 0.0f))
            [
                SNew(SBox)
                .MaxDesiredWidth(760.0f)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(FMargin(0.0f, 0.0f, 0.0f, 8.0f))
                    [
                        SNew(STextBlock)
                        .Visibility(StageZeroTextVisibility(InArgs._TitleText))
                        .Text(InArgs._TitleText)
                        .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), TitleSize))
                        .ColorAndOpacity(InArgs._PrimaryTextColor)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(FMargin(0.0f, 0.0f, 0.0f, 24.0f))
                    [
                        SNew(STextBlock)
                        .Visibility(StageZeroTextVisibility(InArgs._SubtitleText))
                        .Text(InArgs._SubtitleText)
                        .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), SubtitleSize))
                        .ColorAndOpacity(InArgs._SecondaryTextColor)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(FMargin(0.0f, 0.0f, 0.0f, 14.0f))
                    [
                        BuildAccentLine(WhiteBrush, InArgs._AccentColor, 96.0f, 3.0f)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        BuildStatusRow(InArgs, SmallSize)
                    ]
                ]
            ]
            + SVerticalBox::Slot()
            .FillHeight(0.22f)
            [
                SNew(SSpacer)
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(FMargin(72.0f, 0.0f, 72.0f, 64.0f))
            [
                BuildTipText(InArgs, SmallSize, ETextJustify::Left)
            ];
    }

    TSharedRef<SWidget> BuildCinematicWideContent(const FArguments& InArgs, const FSlateBrush* WhiteBrush, int32 TitleSize, int32 SubtitleSize, int32 SmallSize) const
    {
        return SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            [
                SNew(SSpacer)
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(FMargin(84.0f, 0.0f, 84.0f, 86.0f))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(FMargin(0.0f, 0.0f, 0.0f, 8.0f))
                [
                    SNew(STextBlock)
                    .Visibility(StageZeroTextVisibility(InArgs._TitleText))
                    .Text(InArgs._TitleText)
                    .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), FMath::Max(TitleSize, 42)))
                    .ColorAndOpacity(InArgs._PrimaryTextColor)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(FMargin(0.0f, 0.0f, 0.0f, 28.0f))
                [
                    SNew(STextBlock)
                    .Visibility(StageZeroTextVisibility(InArgs._SubtitleText))
                    .Text(InArgs._SubtitleText)
                    .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), FMath::Max(SubtitleSize, 20)))
                    .ColorAndOpacity(InArgs._SecondaryTextColor)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(FMargin(0.0f, 0.0f, 0.0f, 18.0f))
                [
                    BuildAccentLine(WhiteBrush, InArgs._AccentColor, 1720.0f, 3.0f)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    BuildStatusRow(InArgs, SmallSize)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(FMargin(0.0f, 20.0f, 0.0f, 0.0f))
                [
                    SNew(SBox)
                    .MaxDesiredWidth(980.0f)
                    [
                        BuildTipText(InArgs, SmallSize, ETextJustify::Left)
                    ]
                ]
            ];
    }

    TSharedRef<SWidget> BuildMinimalStudioContent(const FArguments& InArgs, const FSlateBrush* WhiteBrush, int32 TitleSize, int32 SubtitleSize, int32 SmallSize) const
    {
        return SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            [
                SNew(SSpacer)
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .HAlign(HAlign_Center)
            .Padding(FMargin(96.0f, 0.0f))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                .Padding(FMargin(0.0f, 0.0f, 0.0f, 20.0f))
                [
                    BuildAccentLine(WhiteBrush, InArgs._AccentColor, 54.0f, 3.0f)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                [
                    SNew(STextBlock)
                    .Visibility(StageZeroTextVisibility(InArgs._TitleText))
                    .Text(InArgs._TitleText)
                    .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), TitleSize))
                    .ColorAndOpacity(InArgs._PrimaryTextColor)
                    .Justification(ETextJustify::Center)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                .Padding(FMargin(0.0f, 10.0f, 0.0f, 26.0f))
                [
                    SNew(STextBlock)
                    .Visibility(StageZeroTextVisibility(InArgs._SubtitleText))
                    .Text(InArgs._SubtitleText)
                    .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), SubtitleSize))
                    .ColorAndOpacity(InArgs._SecondaryTextColor)
                    .Justification(ETextJustify::Center)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                [
                    BuildStatusRow(InArgs, SmallSize)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                .Padding(FMargin(0.0f, 24.0f, 0.0f, 0.0f))
                [
                    SNew(SBox)
                    .MaxDesiredWidth(760.0f)
                    [
                        BuildTipText(InArgs, SmallSize, ETextJustify::Center)
                    ]
                ]
            ]
            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            [
                SNew(SSpacer)
            ];
    }

    TSharedRef<SWidget> BuildGameTipCarouselContent(const FArguments& InArgs, const FSlateBrush* WhiteBrush, int32 TitleSize, int32 SubtitleSize, int32 SmallSize) const
    {
        return SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            [
                SNew(SSpacer)
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(FMargin(84.0f, 0.0f, 84.0f, 80.0f))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(FMargin(0.0f, 0.0f, 0.0f, 12.0f))
                [
                    SNew(STextBlock)
                    .Visibility(StageZeroTextVisibility(InArgs._TitleText))
                    .Text(InArgs._TitleText)
                    .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), TitleSize))
                    .ColorAndOpacity(InArgs._PrimaryTextColor)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(FMargin(0.0f, 0.0f, 0.0f, 22.0f))
                [
                    BuildTipText(InArgs, FMath::Max(SubtitleSize, SmallSize + 4), ETextJustify::Left)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(FMargin(0.0f, 0.0f, 0.0f, 14.0f))
                [
                    BuildAccentLine(WhiteBrush, InArgs._AccentColor, 420.0f, 3.0f)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    BuildStatusRow(InArgs, SmallSize)
                ]
            ];
    }

    TSharedRef<SWidget> BuildMapBriefingContent(const FArguments& InArgs, const FSlateBrush* WhiteBrush, int32 TitleSize, int32 SubtitleSize, int32 SmallSize) const
    {
        return SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(SBox)
                .WidthOverride(8.0f)
                [
                    SNew(SBorder)
                    .BorderImage(WhiteBrush)
                    .BorderBackgroundColor(InArgs._AccentColor)
                ]
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .Padding(FMargin(76.0f, 72.0f))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    BuildStatusRow(InArgs, SmallSize)
                ]
                + SVerticalBox::Slot()
                .FillHeight(1.0f)
                [
                    SNew(SSpacer)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(SBox)
                    .MaxDesiredWidth(820.0f)
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(FMargin(0.0f, 0.0f, 0.0f, 12.0f))
                        [
                            SNew(STextBlock)
                            .Visibility(StageZeroTextVisibility(InArgs._TitleText))
                            .Text(InArgs._TitleText)
                            .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), FMath::Max(TitleSize, 40)))
                            .ColorAndOpacity(InArgs._PrimaryTextColor)
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(FMargin(0.0f, 0.0f, 0.0f, 22.0f))
                        [
                            SNew(STextBlock)
                            .Visibility(StageZeroTextVisibility(InArgs._SubtitleText))
                            .Text(InArgs._SubtitleText)
                            .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), SubtitleSize))
                            .ColorAndOpacity(InArgs._SecondaryTextColor)
                            .AutoWrapText(true)
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            BuildTipText(InArgs, SmallSize, ETextJustify::Left)
                        ]
                    ]
                ]
            ];
    }

    TSharedRef<SWidget> BuildVideoSplashContent(const FArguments& InArgs, const FSlateBrush* WhiteBrush, int32 TitleSize, int32 SubtitleSize, int32 SmallSize) const
    {
        return SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(FMargin(72.0f, 62.0f, 72.0f, 0.0f))
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .Padding(FMargin(0.0f, 0.0f, 18.0f, 0.0f))
                [
                    BuildAccentLine(WhiteBrush, InArgs._AccentColor, 48.0f, 3.0f)
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Visibility(StageZeroTextVisibility(InArgs._TitleText))
                    .Text(InArgs._TitleText)
                    .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), FMath::Min(TitleSize, 30)))
                    .ColorAndOpacity(InArgs._PrimaryTextColor)
                ]
            ]
            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            [
                SNew(SSpacer)
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .HAlign(HAlign_Center)
            .Padding(FMargin(96.0f, 0.0f, 96.0f, 74.0f))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                .Padding(FMargin(0.0f, 0.0f, 0.0f, 12.0f))
                [
                    BuildStatusRow(InArgs, SmallSize)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                [
                    SNew(STextBlock)
                    .Visibility(StageZeroTextVisibility(InArgs._SubtitleText))
                    .Text(InArgs._SubtitleText)
                    .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), SubtitleSize))
                    .ColorAndOpacity(InArgs._SecondaryTextColor)
                    .Justification(ETextJustify::Center)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                .Padding(FMargin(0.0f, 16.0f, 0.0f, 0.0f))
                [
                    SNew(SBox)
                    .MaxDesiredWidth(900.0f)
                    [
                        BuildTipText(InArgs, SmallSize, ETextJustify::Center)
                    ]
                ]
            ];
    }

    TSharedRef<SWidget> BuildConsoleSafeContent(const FArguments& InArgs, const FSlateBrush* WhiteBrush, int32 TitleSize, int32 SubtitleSize, int32 SmallSize) const
    {
        return SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            [
                SNew(SSpacer)
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .HAlign(HAlign_Center)
            .Padding(FMargin(120.0f, 0.0f))
            [
                SNew(SBox)
                .MaxDesiredWidth(980.0f)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .HAlign(HAlign_Center)
                    .Padding(FMargin(0.0f, 0.0f, 0.0f, 18.0f))
                    [
                        SNew(STextBlock)
                        .Visibility(StageZeroTextVisibility(InArgs._TitleText))
                        .Text(InArgs._TitleText)
                        .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), FMath::Max(TitleSize, 44)))
                        .ColorAndOpacity(InArgs._PrimaryTextColor)
                        .Justification(ETextJustify::Center)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .HAlign(HAlign_Center)
                    .Padding(FMargin(0.0f, 0.0f, 0.0f, 28.0f))
                    [
                        SNew(STextBlock)
                        .Visibility(StageZeroTextVisibility(InArgs._SubtitleText))
                        .Text(InArgs._SubtitleText)
                        .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), FMath::Max(SubtitleSize, 24)))
                        .ColorAndOpacity(InArgs._SecondaryTextColor)
                        .Justification(ETextJustify::Center)
                        .AutoWrapText(true)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .HAlign(HAlign_Center)
                    .Padding(FMargin(0.0f, 0.0f, 0.0f, 24.0f))
                    [
                        BuildAccentLine(WhiteBrush, InArgs._AccentColor, 320.0f, 5.0f)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .HAlign(HAlign_Center)
                    [
                        BuildStatusRow(InArgs, FMath::Max(SmallSize, 18))
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .HAlign(HAlign_Center)
                    .Padding(FMargin(0.0f, 28.0f, 0.0f, 0.0f))
                    [
                        BuildTipText(InArgs, FMath::Max(SmallSize, 18), ETextJustify::Center)
                    ]
                ]
            ]
            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            [
                SNew(SSpacer)
            ];
    }

    TSharedRef<SWidget> BuildTemplateContent(const FArguments& InArgs, const FSlateBrush* WhiteBrush, int32 TitleSize, int32 SubtitleSize, int32 SmallSize) const
    {
        switch (InArgs._LoadingScreenTemplate)
        {
            case EStageZeroTravelLoadingScreenTemplate::CinematicWide:
                return BuildCinematicWideContent(InArgs, WhiteBrush, TitleSize, SubtitleSize, SmallSize);
            case EStageZeroTravelLoadingScreenTemplate::MinimalStudio:
                return BuildMinimalStudioContent(InArgs, WhiteBrush, TitleSize, SubtitleSize, SmallSize);
            case EStageZeroTravelLoadingScreenTemplate::GameTipCarousel:
                return BuildGameTipCarouselContent(InArgs, WhiteBrush, TitleSize, SubtitleSize, SmallSize);
            case EStageZeroTravelLoadingScreenTemplate::MapBriefing:
                return BuildMapBriefingContent(InArgs, WhiteBrush, TitleSize, SubtitleSize, SmallSize);
            case EStageZeroTravelLoadingScreenTemplate::VideoSplash:
                return BuildVideoSplashContent(InArgs, WhiteBrush, TitleSize, SubtitleSize, SmallSize);
            case EStageZeroTravelLoadingScreenTemplate::ConsoleSafe:
                return BuildConsoleSafeContent(InArgs, WhiteBrush, TitleSize, SubtitleSize, SmallSize);
            case EStageZeroTravelLoadingScreenTemplate::Classic:
            default:
                return BuildClassicContent(InArgs, WhiteBrush, TitleSize, SubtitleSize, SmallSize);
        }
    }

    FText GetCurrentTipText() const
    {
        if (TipTexts.Num() == 0)
        {
            return FText::GetEmpty();
        }

        return TipTexts[FMath::Clamp(CurrentTipIndex, 0, TipTexts.Num() - 1)];
    }

    const FSlateBrush* GetCurrentBackgroundBrush() const
    {
        if (BackgroundBrushes.Num() == 0)
        {
            return FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
        }

        return &BackgroundBrushes[FMath::Clamp(CurrentBackgroundIndex, 0, BackgroundBrushes.Num() - 1)];
    }

    TArray<FSlateBrush> BackgroundBrushes;
    TArray<FText> TipTexts;
    int32 CurrentBackgroundIndex = 0;
    int32 CurrentTipIndex = 0;
    double LastImageSwitchTime = 0.0;
    double LastTipSwitchTime = 0.0;
    double FadeStartTime = 0.0;
    float ImageIntervalSeconds = 0.0f;
    float TipIntervalSeconds = 0.0f;
    float FadeInSeconds = 0.0f;
    bool bLoopImages = true;
};

UStageZeroWarmupManifest* LoadManifestForWarmup()
{
    return FStageZeroWarmupUtils::LoadAuthoredManifest();
}

} // namespace

UStageZeroSubsystem::UStageZeroSubsystem()
{
}

void UStageZeroSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CachedSettings = GetDefault<UStageZeroProjectSettings>();
    bAutoStartupAttempted = false;
    FCoreUObjectDelegates::PreLoadMapWithContext.AddUObject(this, &UStageZeroSubsystem::HandlePreLoadMapWithContext);
    FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UStageZeroSubsystem::HandlePostLoadMapWithWorld);
    DeleteLegacyPlaintextWarmupCache();

    if (CachedSettings && CachedSettings->StartupMode != EStageZeroStartupMode::Manual)
    {
        StartupTickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UStageZeroSubsystem::TickAutoStartup), 0.0f);
    }
}

void UStageZeroSubsystem::Deinitialize()
{
    FCoreUObjectDelegates::PreLoadMapWithContext.RemoveAll(this);
    FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
    bPendingTravelOpenLevel = false;
    PendingTravelOpenLevelName = NAME_None;
    PendingTravelOpenLevelOptions.Reset();
    bSuppressNextAutomaticTravelLoadingScreen = false;
    SuppressedTravelMapName.Reset();
    if (StartupTickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(StartupTickerHandle);
        StartupTickerHandle.Reset();
    }
    if (PendingTravelOpenLevelTickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(PendingTravelOpenLevelTickerHandle);
        PendingTravelOpenLevelTickerHandle.Reset();
    }
    CancelWarmup();
    ClearRuntimeWarmupCallbacks();
    ClearLegacyTransitionPresentation();
    RetainedWarmupAssets.Reset();
    RemovePostLoadTravelCover();
    StopBootMusic(true);
    Super::Deinitialize();
}

const UStageZeroProjectSettings* UStageZeroSubsystem::GetSettings() const
{
    return CachedSettings ? CachedSettings : GetDefault<UStageZeroProjectSettings>();
}

bool UStageZeroSubsystem::TickAutoStartup(float DeltaSeconds)
{
    (void)DeltaSeconds;

    if (bAutoStartupAttempted || bWarmupRunning || bWarmupCompleted)
    {
        StartupTickerHandle.Reset();
        return false;
    }

    const UStageZeroProjectSettings* Settings = GetSettings();
    if (!Settings || Settings->StartupMode == EStageZeroStartupMode::Manual)
    {
        StartupTickerHandle.Reset();
        return false;
    }

    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld() || World->bIsTearingDown)
    {
        return true;
    }

    if (!World->AreActorsInitialized())
    {
        return true;
    }

    ApplyFirstLegacyLoaderInWorld(/*bBindDelegates*/ true);

    bAutoStartupAttempted = true;
    UE_LOG(LogStageZeroSubsystem, Log, TEXT("StageZero: Auto startup warmup requested (%s)."),
        Settings->StartupMode == EStageZeroStartupMode::BootMapGate ? TEXT("BootMapGate") : TEXT("AutoOnGameInstanceInit"));
    StartWarmup(false);
    StartupTickerHandle.Reset();
    return false;
}

bool UStageZeroSubsystem::IsStrictCompletionEnabled() const
{
    const UStageZeroProjectSettings* Settings = GetSettings();
    return !Settings || Settings->CompletionPolicy == EStageZeroCompletionPolicy::Strict;
}

bool UStageZeroSubsystem::ShouldTreatAssetFailuresAsFatal() const
{
    const UStageZeroProjectSettings* Settings = GetSettings();
    return IsStrictCompletionEnabled() && (!Settings || Settings->bTreatAssetFailuresAsFatal);
}

bool UStageZeroSubsystem::ShouldContinueOnPipelineCacheTimeout() const
{
    const UStageZeroProjectSettings* Settings = GetSettings();
    return !IsStrictCompletionEnabled() || !Settings || Settings->bContinueOnPipelineCacheTimeout;
}

bool UStageZeroSubsystem::ShouldUseAsyncWarmupLoading() const
{
    const UStageZeroProjectSettings* Settings = GetSettings();
    return !Settings || Settings->bUseAsyncWarmupLoading;
}

bool UStageZeroSubsystem::ShouldUsePipelineCacheWarmup() const
{
    const UStageZeroProjectSettings* Settings = GetSettings();
    return Settings && Settings->bUsePipelineCacheWarmup;
}

float UStageZeroSubsystem::GetManifestAssetLoadTimeoutSeconds() const
{
    const UStageZeroProjectSettings* Settings = GetSettings();
    return FMath::Max(1.0f, Settings ? Settings->AssetLoadTimeoutSeconds : 300.0f);
}

float UStageZeroSubsystem::GetPipelineCacheTimeoutSeconds() const
{
    const UStageZeroProjectSettings* Settings = GetSettings();
    return FMath::Max(0.0f, Settings ? Settings->PipelineCacheTimeoutSeconds : 60.0f);
}

bool UStageZeroSubsystem::TryStartRecordedPipelineCacheWarmup()
{
    bPipelineCacheStageRequested = false;
    bPipelineCacheStageAccepted = false;
    PipelineCacheStageFailureReason.Reset();
    PipelinePrecompileBaseline = 0;
    PipelineObservationBaselineGeneration = 0;
    PipelineExpectedProjectCacheKey.Reset();
    PipelineExpectedUserCacheKey.Reset();
    PipelineRequiredCacheKeys.Reset();

    if (!ShouldUsePipelineCacheWarmup())
    {
        UE_LOG(LogStageZeroSubsystem, Display,
            TEXT("StageZero: Recorded PSO-cache warmup is disabled; this run will preload manifest assets only."));
        return false;
    }

    bPipelineCacheStageRequested = true;

#if STAGEZERO_HAS_SHADER_PIPELINE_CACHE
    PipelineExpectedProjectCacheKey = GetExpectedPipelineCacheBaseName(false);
    PipelineExpectedUserCacheKey = GetExpectedPipelineCacheBaseName(true) + TEXT("_usr");
    const FStageZeroPipelineCacheObservation ObservationBeforeOpen = StageZeroPipelineCacheObserver::GetSnapshot();
    PipelineObservationBaselineGeneration = ObservationBeforeOpen.OpenGeneration;
    const bool bOpenedProjectCache = FShaderPipelineCache::OpenPipelineFileCache(GMaxRHIShaderPlatform);
#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2)
    const bool bOpenedUserCache = FShaderPipelineCache::OpenUserPipelineFileCache(GMaxRHIShaderPlatform);
#else
    const bool bOpenedUserCache = false;
#endif
    if (bOpenedProjectCache)
    {
        PipelineRequiredCacheKeys.Add(PipelineExpectedProjectCacheKey);
    }
    if (bOpenedUserCache)
    {
        PipelineRequiredCacheKeys.Add(PipelineExpectedUserCacheKey);
    }
    const FStageZeroPipelineCacheObservation Observation = StageZeroPipelineCacheObserver::GetSnapshot();
    const FStageZeroPipelineCacheKeyAssessment Assessment = StageZeroPipelineCacheObserver::AssessSnapshotForCacheKeys(
        Observation,
        PipelineExpectedProjectCacheKey,
        PipelineExpectedUserCacheKey,
        PipelineRequiredCacheKeys,
        PipelineObservationBaselineGeneration);
    const bool bObservedActiveCache = Assessment.bHasActiveRelevantRecord
        && !Observation.bProtocolError;
    const bool bObservedCompletedCache = Assessment.bHasCompletedRelevantRecord
        && Assessment.bAllRelevantRecordsCompleted
        && Assessment.bAllRequiredRecordsCompleted
        && !Observation.bProtocolError
        && !Assessment.bHasIncompleteRelevantRecord;
    const bool bAcceptedScheduledOpen = PipelineRequiredCacheKeys.Num() > 0
        && !Observation.bProtocolError
        && !Assessment.bHasIncompleteRelevantRecord;
    bPipelineCacheStageAccepted = bAcceptedScheduledOpen || bObservedActiveCache || bObservedCompletedCache;

    if (!bPipelineCacheStageAccepted)
    {
        if (Observation.bProtocolError)
        {
            PipelineCacheStageFailureReason = Observation.ProtocolErrorReason.IsEmpty()
                ? TEXT("Recorded PSO cache observer detected unmatched lifecycle events; PSO coverage is not established.")
                : Observation.ProtocolErrorReason;
        }
        else if (Assessment.bHasIncompleteRelevantRecord)
        {
            PipelineCacheStageFailureReason = TEXT("A recorded PSO cache closed before a matched completion; PSO coverage is not established.");
        }
        else
        {
            PipelineCacheStageFailureReason = TEXT("No matched recorded PSO cache open/completion was observed in this process; PSO coverage is not established.");
        }
        UE_LOG(LogStageZeroSubsystem, Warning,
            TEXT("StageZero: Recorded PSO cache unavailable (ProjectKey=%s UserKey=%s ProjectOpenCall=%s UserOpenCall=%s RelevantRecord=%s RelevantComplete=%s RelevantActive=%s BaselineGeneration=%" UINT64_FMT " OpenGeneration=%" UINT64_FMT " ProtocolError=%s)."),
            *PipelineExpectedProjectCacheKey,
            *PipelineExpectedUserCacheKey,
            bOpenedProjectCache ? TEXT("true") : TEXT("false"),
            bOpenedUserCache ? TEXT("true") : TEXT("false"),
            Assessment.bHasRelevantRecord ? TEXT("true") : TEXT("false"),
            bObservedCompletedCache ? TEXT("true") : TEXT("false"),
            Assessment.bHasActiveRelevantRecord ? TEXT("true") : TEXT("false"),
            PipelineObservationBaselineGeneration,
            Observation.OpenGeneration,
            Observation.bProtocolError ? TEXT("true") : TEXT("false"));
        return false;
    }

    // The public delegates expose expected and final counts, not a live cache-
    // scoped remaining count. Model this truthfully as one binary work stage.
    PipelinePrecompileBaseline = 1;
    UE_LOG(LogStageZeroSubsystem, Display,
        TEXT("StageZero: Accepted cache-identity-scoped recorded pipeline lifecycle (ProjectKey=%s UserKey=%s ProjectOpenCall=%s UserOpenCall=%s Scheduled=%s Active=%s Completed=%s OpenedPSOs=%u CompletedPSOs=%u)."),
        *PipelineExpectedProjectCacheKey,
        *PipelineExpectedUserCacheKey,
        bOpenedProjectCache ? TEXT("true") : TEXT("false"),
        bOpenedUserCache ? TEXT("true") : TEXT("false"),
        bAcceptedScheduledOpen ? TEXT("true") : TEXT("false"),
        bObservedActiveCache ? TEXT("true") : TEXT("false"),
        bObservedCompletedCache ? TEXT("true") : TEXT("false"),
        Assessment.LatestOpenedPSOs,
        Assessment.LatestCompletedPSOs);
    return true;
#else
    PipelineCacheStageFailureReason = TEXT("Asset preload can continue, but this engine build does not expose ShaderPipelineCache; PSO coverage is not established.");
    UE_LOG(LogStageZeroSubsystem, Warning, TEXT("StageZero: ShaderPipelineCache is unavailable in this engine build."));
    return false;
#endif
}

void UStageZeroSubsystem::ClearRuntimeWarmupCallbacks()
{
    ++ManifestLoadRequestSerial;
    ActiveManifestLoadRequestSerial = 0;
    ActiveManifestLoadStartTimeSeconds = 0.0;

    if (UWorld* World = GetWorld())
    {
        FTimerManager& TimerManager = World->GetTimerManager();
        TimerManager.ClearTimer(SpinnerPhaseTimer);
        TimerManager.ClearTimer(FinishDelayTimer);
        TimerManager.ClearTimer(JobPollTimer);
        TimerManager.ClearTimer(ManifestJobTimer);
        TimerManager.ClearTimer(ManifestLoadTimeoutTimer);
    }

    if (ManifestRunDeadlineTickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(ManifestRunDeadlineTickerHandle);
        ManifestRunDeadlineTickerHandle.Reset();
    }
    ManifestRunDeadlineTimeSeconds = 0.0;

    if (ActiveManifestLoadHandle.IsValid())
    {
        ActiveManifestLoadHandle->CancelHandle();
        ActiveManifestLoadHandle.Reset();
    }
}

void UStageZeroSubsystem::WarnIfStartedAfterGameplayWorld() const
{
    const UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld())
    {
        return;
    }

    if (World->HasBegunPlay())
    {
        UE_LOG(LogStageZeroSubsystem, Warning, TEXT("StageZero: Warmup started after the current game world began play. It cannot prevent first-frame PSO discovery that already happened in this world; start Stage Zero before loading the target menu/gameplay map for production startup."));
    }
}

bool UStageZeroSubsystem::ShouldSkipWarmupForRuntimeContext(bool bShowDebug) const
{
    const UWorld* World = GetWorld();
    const bool bDedicatedServer = IsRunningDedicatedServer() || (World && World->GetNetMode() == NM_DedicatedServer);
    if (!bDedicatedServer)
    {
        return false;
    }

    const FString Message = TEXT("StageZero: Warmup skipped on dedicated server because shader, PSO, texture, Niagara, and loading-screen warmup are client rendering work.");
    UE_LOG(LogStageZeroSubsystem, Warning, TEXT("%s"), *Message);
    StageZeroDebugMessage(this, bShowDebug, Message, FColor::Yellow);
    return true;
}

void UStageZeroSubsystem::BroadcastWarmupStartedIfNeeded()
{
    if (bWarmupStartedBroadcast)
    {
        return;
    }

    WarnIfStartedAfterGameplayWorld();
    WarmupResult = EStageZeroWarmupResult::Running;
    bWarmupStartedBroadcast = true;
    OnWarmupStarted.Broadcast();
    BroadcastWarmupStatus();
}

void UStageZeroSubsystem::FinishWarmupFailure(EStageZeroWarmupResult Result, const FString& Reason)
{
    if (!bWarmupRunning)
    {
        ResetWarmupTrackingState();
    }

    if (!bWarmupRunning)
    {
        bWarmupRunning = true;
    }
    LastWarmupStatusMessage = Reason;
    FinishWarmupInternal(false, Result, Reason);
}

TArray<TSoftObjectPtr<UWorld>> UStageZeroSubsystem::GetWarmupMaps(bool bShowDebug) const
{
    TArray<TSoftObjectPtr<UWorld>> Result;

    const bool bWarmAll = bWarmUpAllMapsOverrideSet ? bWarmUpAllMapsOverride : false;
    const FString CurrentMapPath = (GetWorld() && GetWorld()->GetOutermost()) ? GetWorld()->GetOutermost()->GetName() : FString();

    auto AddMap = [&](const TSoftObjectPtr<UWorld>& Map)
    {
        if (Map.IsNull())
        {
            return;
        }

        const FSoftObjectPath Path = Map.ToSoftObjectPath();
        if (!Path.IsValid())
        {
            return;
        }

        if (!CurrentMapPath.IsEmpty() && Path.GetLongPackageName() == CurrentMapPath)
        {
            return;
        }

        Result.AddUnique(Map);
    };

    if (bWarmAll)
    {
        if (const UStageZeroWarmupManifest* Manifest = LoadManifestForWarmup())
        {
            for (const FStageZeroMapMatTexEntry& Entry : Manifest->MapEntries)
            {
                AddMap(SZ_Subsystem_MapFromPackageName(Entry.MapPackageName));
            }
        }
    }
    else if (ManualWarmupOverride.Num() > 0)
    {
        for (const TSoftObjectPtr<UWorld>& Map : ManualWarmupOverride)
        {
            AddMap(Map);
        }
    }

    StageZeroDebugMessage(this, bShowDebug, FString::Printf(TEXT("StageZero: WarmupMaps count=%d"), Result.Num()));
    return Result;
}

bool UStageZeroSubsystem::IsWarmUpAllMapsEnabled(bool bShowDebug) const
{
    const bool bValue = bWarmUpAllMapsOverrideSet ? bWarmUpAllMapsOverride : false;
    StageZeroDebugMessage(this, bShowDebug, FString::Printf(TEXT("StageZero: WarmUpAllMaps=%s"), bValue ? TEXT("true") : TEXT("false")));
    return bValue;
}

FStageZeroWarmupStatus UStageZeroSubsystem::GetWarmupStatus(bool bShowDebug) const
{
    FStageZeroWarmupStatus Status = BuildWarmupStatusSnapshot();
    StageZeroDebugMessage(this, bShowDebug, FString::Printf(TEXT("StageZero: Status Progress=%.2f%% Map=%s (%d/%d) Tasks=%d/%d"),
        Status.Progress * 100.0f,
        *CurrentMapName.ToString(),
        Status.CompletedMaps,
        Status.TotalMaps,
        Status.CompletedTasks,
        Status.TotalTasks));
    return Status;
}

FStageZeroWarmupStatus UStageZeroSubsystem::BuildWarmupStatusSnapshot() const
{
    FStageZeroWarmupStatus Status;
    Status.Result = WarmupResult;
    Status.Progress = WarmupProgress;
    Status.CurrentMap = CurrentMapName;
    Status.CompletedTasks = CompletedTasks;
    Status.TotalTasks = TotalTasks;
    Status.CurrentMapCompleted = FMath::Clamp(CompletedTasks - TaskBaseline, 0, FMath::Max(CurrentMapTaskTotal, 0));
    Status.CurrentMapTotal = CurrentMapTaskTotal;
    Status.CompletedMaps = CompletedMaps;
    Status.TotalMaps = TargetMapCount > 0 ? TargetMapCount : GetWarmupMaps(false).Num();
    Status.bStrictCompletion = IsStrictCompletionEnabled();
    Status.bPipelineTimedOut = bPipelineTimedOut;
    Status.bAssetLoadFailuresFatal = ShouldTreatAssetFailuresAsFatal();
    Status.ManifestAssetsCompleted = ManifestTasksCompleted;
    Status.ManifestAssetsTotal = ManifestTaskTotal;

    const int32 PipelineRemaining = GetRecordedPipelineCacheWorkRemaining();
    Status.bPipelineCacheRequested = bPipelineCacheStageRequested;
    Status.bPipelineCacheAccepted = bPipelineCacheStageAccepted;
    if (bPipelineCacheStageRequested)
    {
        Status.PipelineCacheTotal = FMath::Max(1, PipelinePrecompileBaseline);
        Status.PipelineCacheRemaining = bPipelineCacheStageAccepted ? FMath::Max(0, PipelineRemaining) : 1;
        Status.PipelineCacheCompleted = bPipelineCacheStageAccepted
            ? (PipelineRemaining <= 0
                ? Status.PipelineCacheTotal
                : FMath::Clamp(Status.PipelineCacheTotal - PipelineRemaining, 0, Status.PipelineCacheTotal))
            : 0;
    }
    Status.RuntimeDiscoveredCompleted = 0;
    Status.RuntimeDiscoveredTotal = 0;
    Status.RuntimeDiscoveredRemaining = 0;
    Status.MaterialRenderWarmupCompleted = 0;
    Status.MaterialRenderWarmupTotal = 0;
    Status.FailedSkippedTotal = FailedWarmupAssets.Num();
    Status.FailedAssets = FailedWarmupAssets;
    Status.StatusMessage = LastWarmupStatusMessage;

    auto AddDomain = [&Status](EStageZeroWarmupDomain Domain, int32 Completed, int32 Total, int32 Remaining, int32 Failed)
    {
        FStageZeroWarmupDomainStatus& DomainStatus = Status.Domains.AddDefaulted_GetRef();
        DomainStatus.Domain = Domain;
        DomainStatus.Completed = Completed;
        DomainStatus.Total = Total;
        DomainStatus.Remaining = Remaining;
        DomainStatus.Failed = Failed;
    };

    AddDomain(EStageZeroWarmupDomain::ManifestAssets, Status.ManifestAssetsCompleted, Status.ManifestAssetsTotal, FMath::Max(0, Status.ManifestAssetsTotal - Status.ManifestAssetsCompleted), ManifestFailedLoads);
    const int32 PipelineFailureCount = (bPipelineCacheStageRequested && !bPipelineCacheStageAccepted)
        ? 1
        : (bPipelineTimedOut ? Status.PipelineCacheRemaining : 0);
    AddDomain(EStageZeroWarmupDomain::PipelineCache, Status.PipelineCacheCompleted, Status.PipelineCacheTotal, Status.PipelineCacheRemaining, PipelineFailureCount);
    AddDomain(EStageZeroWarmupDomain::FailedSkipped, 0, Status.FailedSkippedTotal, Status.FailedSkippedTotal, Status.FailedSkippedTotal);

    return Status;
}

void UStageZeroSubsystem::BroadcastWarmupStatus()
{
    OnWarmupStatusChanged.Broadcast(BuildWarmupStatusSnapshot());
}

FStageZeroBootViewConfig UStageZeroSubsystem::GetBootViewConfig(bool bShowDebug) const
{
    FStageZeroBootViewConfig View;
    const FStageZeroBootConfig Config = BuildBootConfig();
    View.WidgetClass = Config.WidgetClass;
    View.SpinnerIcon = Config.SpinnerIcon;
    View.BackgroundSlides = Config.BackgroundSlides;
    View.BootMoviePath = Config.BootMoviePath;
    View.bLoopBootMovie = Config.bLoopBootMovie;
    View.bBootMovieSkippable = Config.bBootMovieSkippable;
    View.BootMovieSkipDelaySeconds = Config.BootMovieSkipDelaySeconds;
    View.BootMovieSkipText = Config.BootMovieSkipText;
    View.bRestoreSlidesAfterBootMovie = Config.bRestoreSlidesAfterBootMovie;
    View.bPlayBootMovieAudio = Config.bPlayBootMovieAudio;
    View.BootMovieVolume = Config.BootMovieVolume;
    View.SlideInterval = Config.SlideInterval;
    View.SlideFadeDuration = Config.SlideFadeDuration;
    View.bLoopSlides = Config.bLoopSlides;
    View.Tips = Config.Tips;
    View.TipInterval = Config.TipInterval;
    View.BackgroundMusic = Config.BackgroundMusic;
    View.MusicFadeInTime = Config.MusicFadeInTime;
    View.MusicFadeOutTime = Config.MusicFadeOutTime;
    View.SpinnerOnlyDuration = Config.SpinnerOnlyDuration;
    View.bShowSpinnerDuringProgress = Config.bShowSpinnerDuringProgress;
    StageZeroDebugMessage(this, bShowDebug, TEXT("StageZero: BootViewConfig retrieved"));
    return View;
}

void UStageZeroSubsystem::SetBootViewConfig(const FStageZeroBootViewConfig& InConfig, bool bShowDebug)
{
    bRuntimeBootOverride = true;
    RuntimeBootConfig.WidgetClass = InConfig.WidgetClass;
    RuntimeBootConfig.SpinnerIcon = InConfig.SpinnerIcon;
    RuntimeBootConfig.BackgroundSlides = InConfig.BackgroundSlides;
    RuntimeBootConfig.BootMoviePath = InConfig.BootMoviePath;
    RuntimeBootConfig.bLoopBootMovie = InConfig.bLoopBootMovie;
    RuntimeBootConfig.bBootMovieSkippable = InConfig.bBootMovieSkippable;
    RuntimeBootConfig.BootMovieSkipDelaySeconds = InConfig.BootMovieSkipDelaySeconds;
    RuntimeBootConfig.BootMovieSkipText = InConfig.BootMovieSkipText;
    RuntimeBootConfig.bRestoreSlidesAfterBootMovie = InConfig.bRestoreSlidesAfterBootMovie;
    RuntimeBootConfig.bPlayBootMovieAudio = InConfig.bPlayBootMovieAudio;
    RuntimeBootConfig.BootMovieVolume = InConfig.BootMovieVolume;
    RuntimeBootConfig.SlideInterval = InConfig.SlideInterval;
    RuntimeBootConfig.SlideFadeDuration = InConfig.SlideFadeDuration;
    RuntimeBootConfig.bLoopSlides = InConfig.bLoopSlides;
    RuntimeBootConfig.Tips = InConfig.Tips;
    RuntimeBootConfig.TipInterval = InConfig.TipInterval;
    RuntimeBootConfig.BackgroundMusic = InConfig.BackgroundMusic;
    RuntimeBootConfig.MusicFadeInTime = InConfig.MusicFadeInTime;
    RuntimeBootConfig.MusicFadeOutTime = InConfig.MusicFadeOutTime;
    RuntimeBootConfig.SpinnerOnlyDuration = InConfig.SpinnerOnlyDuration;
    RuntimeBootConfig.bShowSpinnerDuringProgress = InConfig.bShowSpinnerDuringProgress;
    StageZeroDebugMessage(this, bShowDebug, TEXT("StageZero: BootViewConfig overridden at runtime"));
}

void UStageZeroSubsystem::SetBackgroundSlidesOverride(const TArray<TSoftObjectPtr<UTexture2D>>& Slides, bool bLoopSlides, bool bShowDebug)
{
    // Start from current resolved config so other values persist.
    FStageZeroBootConfig Config = BuildBootConfig();
    Config.BackgroundSlides = Slides;
    Config.bLoopSlides = bLoopSlides;

    RuntimeBootConfig = Config;
    bRuntimeBootOverride = true;

    StageZeroDebugMessage(this, bShowDebug, FString::Printf(TEXT("StageZero: Background slides overridden (%d slides, loop=%s)"), Slides.Num(), bLoopSlides ? TEXT("true") : TEXT("false")));

    if (ActiveLoadingWidget.IsValid())
    {
        ApplyBootScreenConfig(ActiveLoadingWidget.Get());
    }
}

void UStageZeroSubsystem::SetWarmupCacheEnabled(bool bEnable)
{
    DeleteLegacyPlaintextWarmupCache();
    if (bEnable)
    {
        UE_LOG(LogStageZeroSubsystem, Warning,
            TEXT("StageZero: SetWarmupCacheEnabled(true) was ignored. Cross-process asset warmup records cannot prove current-process resource or PSO readiness and have been retired."));
    }
}

bool UStageZeroSubsystem::IsWarmupCacheEnabled() const
{
    return false;
}

void UStageZeroSubsystem::ReleaseRetainedWarmupAssets()
{
    const int32 ReleasedCount = RetainedWarmupAssets.Num();
    RetainedWarmupAssets.Reset();
    bReleaseRetainedAssetsAfterNextMapLoad = false;
    if (ReleasedCount > 0)
    {
        UE_LOG(LogStageZeroSubsystem, Log,
            TEXT("StageZero: Released %d retained preload asset reference(s) after destination ownership was established."),
            ReleasedCount);
    }
}

FStageZeroWarmupCacheStatus UStageZeroSubsystem::GetWarmupCacheStatus(bool bRequirePipelineCacheClear, bool bShowDebug)
{
    (void)bRequirePipelineCacheClear;
    FStageZeroWarmupCacheStatus Status;
    Status.bCacheEnabled = false;
    Status.bPipelineCacheClear = false;
    Status.bCanSkipWarmup = false;
    Status.Reason = TEXT("Persistent warmup skipping was retired because a previous process cannot prove that assets or PSOs are ready in the current process. Start a normal Stage Zero warmup.");
    StageZeroDebugMessage(this, bShowDebug, Status.Reason, FColor::Yellow);
    return Status;
}

bool UStageZeroSubsystem::CanSkipStageZeroWarmup(bool bRequirePipelineCacheClear, bool bShowDebug)
{
    return GetWarmupCacheStatus(bRequirePipelineCacheClear, bShowDebug).bCanSkipWarmup;
}

EStageZeroWarmupStartResult UStageZeroSubsystem::StartWarmupIfNeeded(bool bRequirePipelineCacheClear, bool bShowDebug)
{
    (void)bRequirePipelineCacheClear;
    if (bWarmupRunning)
    {
        StageZeroDebugMessage(this, bShowDebug, TEXT("StageZero: Warmup already running"), FColor::Yellow);
        return EStageZeroWarmupStartResult::AlreadyRunning;
    }

    StartWarmup(bShowDebug);
    return WarmupResult == EStageZeroWarmupResult::Failed || WarmupResult == EStageZeroWarmupResult::Cancelled
        ? EStageZeroWarmupStartResult::Failed
        : EStageZeroWarmupStartResult::Started;
}

void UStageZeroSubsystem::GetStageZeroWarmupJobs(TArray<FStageZeroWarmupJobInfo>& OutJobs) const
{
    OutJobs.Reset();
    WarmupDebugJobs.GenerateValueArray(OutJobs);
    OutJobs.Sort([](const FStageZeroWarmupJobInfo& A, const FStageZeroWarmupJobInfo& B)
    {
        return A.AssetPath.ToString() < B.AssetPath.ToString();
    });
}

bool UStageZeroSubsystem::IsStageZeroWarmupDebugEnabled() const
{
    return true;
}

void UStageZeroSubsystem::GetTaskCounts(int32& OutCompleted, int32& OutTotal, bool bShowDebug) const
{
    OutCompleted = CompletedTasks;
    OutTotal = TotalTasks;
    StageZeroDebugMessage(this, bShowDebug, FString::Printf(TEXT("StageZero: TaskCounts %d/%d"), OutCompleted, OutTotal));
}

void UStageZeroSubsystem::SetPrimaryLevel(TSoftObjectPtr<UWorld> InPrimaryLevel, bool bShowDebug)
{
    PrimaryLevelOverride = InPrimaryLevel;
    bAutoTransitionToPrimary = !InPrimaryLevel.IsNull();
    bAutoTransitionOverrideSet = true;
    StageZeroDebugMessage(this, bShowDebug, FString::Printf(TEXT("StageZero: PrimaryLevel set to %s"), *InPrimaryLevel.ToString()));
}

void UStageZeroSubsystem::SetWarmupMaps(const TArray<TSoftObjectPtr<UWorld>>& InMaps, bool bShowDebug)
{
    ManualWarmupOverride = InMaps;
    StageZeroDebugMessage(this, bShowDebug, FString::Printf(TEXT("StageZero: WarmupMaps overridden (%d maps)"), ManualWarmupOverride.Num()));
}

void UStageZeroSubsystem::AddWarmupMap(TSoftObjectPtr<UWorld> InMap, bool bShowDebug)
{
    if (!InMap.IsNull())
    {
        ManualWarmupOverride.AddUnique(InMap);
        StageZeroDebugMessage(this, bShowDebug, FString::Printf(TEXT("StageZero: Added warmup map %s"), *InMap.ToString()));
    }
}

void UStageZeroSubsystem::ClearWarmupMaps(bool bShowDebug)
{
    ManualWarmupOverride.Empty();
    StageZeroDebugMessage(this, bShowDebug, TEXT("StageZero: Cleared warmup maps override"));
}

void UStageZeroSubsystem::SetWarmUpAllMaps(bool bEnable, bool bShowDebug)
{
    bWarmUpAllMapsOverrideSet = true;
    bWarmUpAllMapsOverride = bEnable;
    StageZeroDebugMessage(this, bShowDebug, FString::Printf(TEXT("StageZero: WarmUpAllMaps override set to %s"), bEnable ? TEXT("true") : TEXT("false")));
}

void UStageZeroSubsystem::SetWarmUpLevelInstances(bool bEnable, bool bShowDebug)
{
    (void)bEnable;
    StageZeroDebugMessage(this, bShowDebug, TEXT("StageZero: Runtime Level Instance traversal is retired; configure manifest authoring instead."), FColor::Yellow);
}

void UStageZeroSubsystem::SetWarmUpPackedLevelActors(bool bEnable, bool bShowDebug)
{
    (void)bEnable;
    StageZeroDebugMessage(this, bShowDebug, TEXT("StageZero: Runtime Packed Level Actor traversal is retired; configure manifest authoring instead."), FColor::Yellow);
}

void UStageZeroSubsystem::RegisterLoadingWidget(UUserWidget* Widget, bool bHideAfterComplete, bool bShowDebug)
{
    if (!IsValid(Widget))
    {
        bHideRegisteredWidgetOnComplete = false;
        return;
    }

    UStageZeroLoadingWidget* TypedWidget = Cast<UStageZeroLoadingWidget>(Widget);
    const bool bTerminalWarmupSnapshot =
        !bWarmupRunning
        && WarmupResult != EStageZeroWarmupResult::NotStarted
        && WarmupResult != EStageZeroWarmupResult::Running;

    if (TypedWidget && bTerminalWarmupSnapshot)
    {
        TypedWidget->bHideAfterWarmup = TypedWidget->bHideAfterWarmup || bHideAfterComplete;
        TypedWidget->SetProgressVisibility(false);
        TypedWidget->SetSpinnerVisibility(false);
        if (TypedWidget->bHideAfterWarmup)
        {
            TypedWidget->RemoveFromParent();
        }
        StageZeroDebugMessage(this, bShowDebug, FString::Printf(TEXT("StageZero: Ignored stale loading widget registration after warmup completion (%s)."), *Widget->GetName()));
        return;
    }

    bHideRegisteredWidgetOnComplete = bHideRegisteredWidgetOnComplete || bHideAfterComplete;
    ActiveGenericWidget = Widget;
    StageZeroDebugMessage(this, bShowDebug, FString::Printf(TEXT("StageZero: Registered loading widget %s"), *Widget->GetName()));

    if (!TypedWidget)
    {
        return;
    }

    bWidgetSpawnedBySubsystem = false;
    ActiveLoadingWidget = TypedWidget;
    TypedWidget->bHideAfterWarmup = TypedWidget->bHideAfterWarmup || bHideAfterComplete;
    ApplyBootScreenConfig(TypedWidget);
    TypedWidget->BindToStageZeroSubsystem(this);
}

void UStageZeroSubsystem::UnregisterLoadingWidget(UUserWidget* Widget, bool bShowDebug)
{
    if (ActiveLoadingWidget.IsValid() && ActiveLoadingWidget.Get() == Widget)
    {
        ActiveLoadingWidget.Reset();
        bWidgetSpawnedBySubsystem = false;
    }

    if (ActiveGenericWidget.IsValid() && ActiveGenericWidget.Get() == Widget)
    {
        ActiveGenericWidget.Reset();
    }

    StageZeroDebugMessage(this, bShowDebug, FString::Printf(TEXT("StageZero: Unregistered loading widget %s"), IsValid(Widget) ? *Widget->GetName() : TEXT("Invalid")));
}

void UStageZeroSubsystem::SetAutoTransitionToPrimary(bool bEnable, bool bShowDebug)
{
    bAutoTransitionToPrimary = bEnable;
    bAutoTransitionOverrideSet = true;
    StageZeroDebugMessage(this, bShowDebug, FString::Printf(TEXT("StageZero: AutoTransitionToPrimary set to %s"), bEnable ? TEXT("true") : TEXT("false")));
}

void UStageZeroSubsystem::SetAutoSpawnWidget(bool bEnable, bool bShowDebug)
{
    bShouldSpawnWidget = bEnable;
    StageZeroDebugMessage(this, bShowDebug, FString::Printf(TEXT("StageZero: AutoSpawnWidget set to %s"), bEnable ? TEXT("true") : TEXT("false")));
}

void UStageZeroSubsystem::SetAllowCreatePlayerIfMissing(bool bEnable, bool bShowDebug)
{
    bAllowCreatePlayerIfMissing = bEnable;
    StageZeroDebugMessage(this, bShowDebug, FString::Printf(TEXT("StageZero: AllowCreatePlayerIfMissing set to %s"), bEnable ? TEXT("true") : TEXT("false")));
}

void UStageZeroSubsystem::SetMinimumDisplaySeconds(float InSeconds, bool bShowDebug)
{
    MinimumDisplaySeconds = FMath::Max(0.0f, InSeconds);
    bMinimumDisplaySecondsOverrideSet = true;
    StageZeroDebugMessage(this, bShowDebug, FString::Printf(TEXT("StageZero: MinimumDisplaySeconds set to %.2f"), MinimumDisplaySeconds));
}

void UStageZeroSubsystem::SetTravelMinimumDisplaySeconds(float InSeconds, bool bShowDebug)
{
    TravelMinimumDisplaySecondsOverride = FMath::Max(0.0f, InSeconds);
    bTravelMinimumDisplaySecondsOverrideSet = true;
    StageZeroDebugMessage(this, bShowDebug, FString::Printf(TEXT("StageZero: TravelMinimumDisplaySeconds set to %.2f"), TravelMinimumDisplaySecondsOverride));
}

void UStageZeroSubsystem::SetDelayAfterComplete(float InSeconds, bool bShowDebug)
{
    DelayAfterComplete = FMath::Max(0.0f, InSeconds);
    StageZeroDebugMessage(this, bShowDebug, FString::Printf(TEXT("StageZero: DelayAfterComplete set to %.2f"), DelayAfterComplete));
}

void UStageZeroSubsystem::ApplyLegacyActorSettings(AStageZeroLoader* Loader)
{
    if (!IsValid(Loader))
    {
        return;
    }

    bHasLegacySettings = true;
    PrimaryLevelOverride = Loader->PrimaryLevel;
    ManualWarmupOverride = Loader->MapsToWarmUp;

    bWarmUpAllMapsOverrideSet = true;
    bWarmUpAllMapsOverride = Loader->bWarmUpAllMaps;

    LegacyBootConfig.WidgetClass = Loader->LoadingWidgetClass;
    LegacyBootConfig.SpinnerIcon = Loader->LoadingIconTexture;
    LegacyBootConfig.BackgroundSlides = Loader->BackgroundSlides;
    LegacyBootConfig.SlideInterval = Loader->BackgroundSlideInterval;
    LegacyBootConfig.SlideFadeDuration = Loader->BackgroundSlideFadeDuration;
    LegacyBootConfig.bLoopSlides = Loader->bLoopBackgroundSlides;
    LegacyBootConfig.Tips = Loader->BootTips;
    LegacyBootConfig.TipInterval = Loader->BootTipInterval;
    LegacyBootConfig.BackgroundMusic = Loader->BackgroundMusic;
    LegacyBootConfig.MusicFadeInTime = Loader->MusicFadeInTime;
    LegacyBootConfig.MusicFadeOutTime = Loader->MusicFadeOutTime;
    LegacyBootConfig.SpinnerOnlyDuration = Loader->SpinnerOnlyPhaseDuration;
    LegacyBootConfig.bShowSpinnerDuringProgress = Loader->bShowSpinnerDuringProgress;
    ConfigureLegacyTransitionMessage(Loader->bShowTransitionMessage, Loader->TransitionMessage);

    if (!Loader->bAutoCreateLoadingWidget)
    {
        LegacyBootConfig.WidgetClass.Reset();
    }

    MinimumDisplaySeconds = Loader->MinimumDisplaySeconds;
    bMinimumDisplaySecondsOverrideSet = true;
    DelayAfterComplete = Loader->DelayAfterComplete;
    bAutoTransitionToPrimary = !Loader->PrimaryLevel.IsNull();
    bAutoTransitionOverrideSet = true;
    bShouldSpawnWidget = Loader->bAutoCreateLoadingWidget;
    bAllowCreatePlayerIfMissing = Loader->bAllowCreatePlayerIfMissing;
}

UStageZeroSubsystem::FStageZeroBootConfig UStageZeroSubsystem::BuildBootConfig() const
{
    FStageZeroBootConfig Result = bRuntimeBootOverride ? RuntimeBootConfig : (bHasLegacySettings ? LegacyBootConfig : FStageZeroBootConfig{});
    const UStageZeroProjectSettings* Settings = GetSettings();

    if (Settings)
    {
        if (bHasLegacySettings)
        {
            if (!bRuntimeBootOverride && Result.WidgetClass.IsNull())
            {
                Result.WidgetClass = Settings->BuiltInLoadingWidgetClass;
            }
            if (!bRuntimeBootOverride && Result.SpinnerIcon.IsNull())
            {
                Result.SpinnerIcon = Settings->LoadingIconTexture;
            }
            if (!bRuntimeBootOverride && Result.BackgroundSlides.Num() == 0)
            {
                Result.BackgroundSlides = Settings->BackgroundSlides;
            }
            if (!bRuntimeBootOverride && Result.BootMoviePath.IsEmpty())
            {
                Result.BootMoviePath = Settings->BootMoviePath;
                Result.bLoopBootMovie = Settings->bLoopBootMovie;
                Result.bPlayBootMovieAudio = Settings->bPlayBootMovieAudio;
                Result.BootMovieVolume = Settings->BootMovieVolume;
            }
            if (!bRuntimeBootOverride)
            {
                Result.bBootMovieSkippable = Settings->bBootMovieSkippable;
                Result.BootMovieSkipDelaySeconds = Settings->BootMovieSkipDelaySeconds;
                Result.BootMovieSkipText = Settings->BootMovieSkipText;
                Result.bRestoreSlidesAfterBootMovie = Settings->bRestoreSlidesAfterBootMovie;
            }
            if (!bRuntimeBootOverride && Result.SlideInterval <= 0.0f)
            {
                Result.SlideInterval = Settings->BackgroundSlideInterval;
            }
            if (!bRuntimeBootOverride && Result.SlideFadeDuration < 0.0f)
            {
                Result.SlideFadeDuration = Settings->BackgroundSlideFadeDuration;
            }
            if (!bRuntimeBootOverride && Result.Tips.Num() == 0)
            {
                Result.Tips = Settings->BootTips;
            }
            if (!bRuntimeBootOverride && Result.TipInterval <= 0.0f)
            {
                Result.TipInterval = Settings->BootTipInterval;
            }
            if (!bRuntimeBootOverride && Result.BackgroundMusic.IsNull())
            {
                Result.BackgroundMusic = Settings->BackgroundMusic;
            }
            if (!bRuntimeBootOverride && Result.MusicFadeInTime <= 0.0f)
            {
                Result.MusicFadeInTime = Settings->MusicFadeInTime;
            }
            if (!bRuntimeBootOverride && Result.MusicFadeOutTime <= 0.0f)
            {
                Result.MusicFadeOutTime = Settings->MusicFadeOutTime;
            }
            if (!bRuntimeBootOverride && Result.SpinnerOnlyDuration <= 0.0f)
            {
                Result.SpinnerOnlyDuration = Settings->SpinnerOnlyPhaseDuration;
            }

            Result.bShowSpinnerDuringProgress = Result.bShowSpinnerDuringProgress || Settings->bShowSpinnerDuringProgress;
            Result.bLoopSlides = Result.bLoopSlides || Settings->bLoopBackgroundSlides;
        }
        else
        {
            Result.WidgetClass = Settings->BuiltInLoadingWidgetClass;
            Result.SpinnerIcon = Settings->LoadingIconTexture;
            Result.BackgroundSlides = Settings->BackgroundSlides;
            Result.BootMoviePath = Settings->BootMoviePath;
            Result.bLoopBootMovie = Settings->bLoopBootMovie;
            Result.bBootMovieSkippable = Settings->bBootMovieSkippable;
            Result.BootMovieSkipDelaySeconds = Settings->BootMovieSkipDelaySeconds;
            Result.BootMovieSkipText = Settings->BootMovieSkipText;
            Result.bRestoreSlidesAfterBootMovie = Settings->bRestoreSlidesAfterBootMovie;
            Result.bPlayBootMovieAudio = Settings->bPlayBootMovieAudio;
            Result.BootMovieVolume = Settings->BootMovieVolume;
            Result.SlideInterval = Settings->BackgroundSlideInterval;
            Result.SlideFadeDuration = Settings->BackgroundSlideFadeDuration;
            Result.Tips = Settings->BootTips;
            Result.TipInterval = Settings->BootTipInterval;
            Result.BackgroundMusic = Settings->BackgroundMusic;
            Result.MusicFadeInTime = Settings->MusicFadeInTime;
            Result.MusicFadeOutTime = Settings->MusicFadeOutTime;
            Result.SpinnerOnlyDuration = Settings->SpinnerOnlyPhaseDuration;
            Result.bShowSpinnerDuringProgress = Settings->bShowSpinnerDuringProgress;
            Result.bLoopSlides = Settings->bLoopBackgroundSlides;
        }
    }

    return Result;
}

bool UStageZeroSubsystem::ApplyFirstLegacyLoaderInWorld(bool bBindDelegates)
{
    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld())
    {
        return false;
    }

    for (TActorIterator<AStageZeroLoader> It(World); It; ++It)
    {
        AStageZeroLoader* Loader = *It;
        if (!IsValid(Loader))
        {
            continue;
        }

        Loader->ApplyProjectSettings();
        ApplyLegacyActorSettings(Loader);
        if (bBindDelegates)
        {
            Loader->BindSubsystemDelegates(this);
        }

        UE_LOG(LogStageZeroSubsystem, Verbose, TEXT("StageZero: Applied legacy loader settings before warmup/cache evaluation (%s)."), *Loader->GetName());
        return true;
    }

    return false;
}

void UStageZeroSubsystem::ResetWarmupTrackingState()
{
    ClearRuntimeWarmupCallbacks();
    ++ManifestLoadRequestSerial;
    ActiveManifestLoadRequestSerial = 0;
    ActiveManifestLoadStartTimeSeconds = 0.0;
    bWarmupFinishPending = false;
    RetainedWarmupAssets.Reset();
    bReleaseRetainedAssetsAfterNextMapLoad = false;
    TotalTasks = 0;
    CompletedTasks = 0;
    RawCompletedTasks = 0;
    TaskBaseline = 0;
    CurrentMapTaskTotal = 0;
    CompletedMaps = 0;
    CurrentMapName = NAME_None;
    ManifestTaskTotal = 0;
    ManifestTasksCompleted = 0;
    PipelinePrecompileBaseline = 0;
    TargetMapCount = 0;
    LastProgressLogCompleted = -1;
    LastBroadcastCompletedTasks = 0;
    LastBroadcastTotalTasks = 0;
    LastBroadcastProgress = 0.0f;
    ManifestFailedLoads = 0;
    bManifestProcessingActive = false;
    ManifestJobQueue.Reset();
    ManifestJobCursor = 0;
    ActiveManifestBatch.Reset();
    if (ActiveManifestLoadHandle.IsValid())
    {
        ActiveManifestLoadHandle->CancelHandle();
        ActiveManifestLoadHandle.Reset();
    }
    bPipelineTimedOut = false;
    bPipelineCacheStageRequested = false;
    bPipelineCacheStageAccepted = false;
    PipelineCacheStageFailureReason.Reset();
    PipelineObservationBaselineGeneration = 0;
    PipelineExpectedProjectCacheKey.Reset();
    PipelineExpectedUserCacheKey.Reset();
    PipelineRequiredCacheKeys.Reset();
    LastPipelineRemaining = 0;
    PipelineWaitStartTimeSeconds = 0.0;
    LastPipelineDecreaseTimeSeconds = 0.0;
    FailedWarmupAssets.Reset();
    FailedWarmupAssetPaths.Reset();
    WarmupDebugJobs.Reset();
    bWarmupStartedBroadcast = false;
    bLegacyTransitionMessageDispatched = false;
    ClearLegacyTransitionPresentation();
    ActiveWarmupMapCoverage.Reset();
    ManifestPathMapCoverageMembership.Reset();
    WarmupResult = EStageZeroWarmupResult::NotStarted;
    LastWarmupStatusMessage.Reset();
    WarmupProgress = 0.0f;
    LastObservedOutstandingRuntimeWorkTime = 0.0;
}

void UStageZeroSubsystem::StartWarmup(bool bShowDebug)
{
    if (bWarmupRunning)
    {
        StageZeroDebugMessage(this, bShowDebug, TEXT("StageZero: Warmup already running"));
        return;
    }

#if WITH_EDITOR
    if (GetWorld() && !GetWorld()->IsGameWorld())
    {
        StageZeroDebugMessage(this, bShowDebug, TEXT("StageZero: Warmup skipped (non-game editor world)"));
        return;
    }
#endif

    if (ShouldSkipWarmupForRuntimeContext(bShowDebug))
    {
        FinishWarmupFailure(EStageZeroWarmupResult::Cancelled, TEXT("Warmup skipped on dedicated server."));
        return;
    }

    ApplyFirstLegacyLoaderInWorld(/*bBindDelegates*/ true);

    const UStageZeroProjectSettings* Settings = GetSettings();

    const FString PrimaryPackageName = GetPackageNameFromMap(PrimaryLevelOverride);
    if (bWarmupCompleted
        && WarmupResult == EStageZeroWarmupResult::Succeeded
        && bAutoTransitionToPrimary
        && !PrimaryPackageName.IsEmpty()
        && DoesWorldMatchRequestedMap(GetWorld(), PrimaryPackageName))
    {
        UE_LOG(LogStageZeroSubsystem, Display, TEXT("StageZero: Warmup start ignored because the completed primary level is already loaded (%s)."),
            *PrimaryPackageName);
        StageZeroDebugMessage(this, bShowDebug, TEXT("StageZero: Warmup already completed for the loaded primary level."));
        return;
    }

    StageZeroDebugMessage(this, bShowDebug, TEXT("StageZero: Starting warmup"));
    ResetWarmupTrackingState();
    bWarmupRunning = true;
    bWasCancelled = false;
    bWarmupCompleted = false;
    WarmupResult = EStageZeroWarmupResult::Running;
    WarmupProgress = 0.0f;
    WarmupStartTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    if (!bMinimumDisplaySecondsOverrideSet)
    {
        MinimumDisplaySeconds = Settings ? FMath::Max(0.0f, Settings->WarmupMinimumDisplaySeconds) : 0.5f;
    }

    const bool bWarmAll = bWarmUpAllMapsOverrideSet ? bWarmUpAllMapsOverride : false;

    TArray<FName> MapPackages;
    if (!bWarmAll)
    {
        const TArray<TSoftObjectPtr<UWorld>> MapsToUse = GetWarmupMaps(false);
        for (const TSoftObjectPtr<UWorld>& Map : MapsToUse)
        {
            const FSoftObjectPath Path = Map.ToSoftObjectPath();
            const FString PackageName = Path.GetLongPackageName();
            if (!PackageName.IsEmpty())
            {
                MapPackages.AddUnique(FName(*PackageName));
            }
        }
        if (MapPackages.Num() == 0)
        {
            UE_LOG(LogStageZeroSubsystem, Warning, TEXT("StageZero: No maps specified for warmup override; falling back to all manifest maps."));
        }
    }

    UStageZeroWarmupManifest* Manifest = LoadManifestForWarmup();

    if (Manifest)
    {
        WarmUpFromManifest(*Manifest, MapPackages, bShowDebug);
    }
    else
    {
        UE_LOG(LogStageZeroSubsystem, Warning, TEXT("StageZero: Mat/tex warmup manifest not found. Warmup skipped."));
        FinishWarmupFailure(EStageZeroWarmupResult::Failed, TEXT("Warmup manifest was not found."));
    }
}

void UStageZeroSubsystem::WarmUpAllMaps(bool bShowDebug)
{
    if (bWarmupRunning)
    {
        UE_LOG(LogStageZeroSubsystem, Warning, TEXT("StageZero: Warmup already running; ignoring WarmUpAllMaps request."));
        StageZeroDebugMessage(this, bShowDebug, TEXT("StageZero: Warmup already running; WarmUpAllMaps ignored"), FColor::Yellow);
        return;
    }
    if (ShouldSkipWarmupForRuntimeContext(bShowDebug))
    {
        FinishWarmupFailure(EStageZeroWarmupResult::Cancelled, TEXT("Warmup skipped on dedicated server."));
        return;
    }
    if (UStageZeroWarmupManifest* Manifest = LoadManifestForWarmup())
    {
        WarmUpFromManifest(*Manifest, TArray<FName>(), bShowDebug);
    }
    else
    {
        UE_LOG(LogStageZeroSubsystem, Warning, TEXT("StageZero: Mat/tex warmup manifest not found. WarmUpAllMaps skipped."));
        FinishWarmupFailure(EStageZeroWarmupResult::Failed, TEXT("Warmup manifest was not found."));
    }
}

void UStageZeroSubsystem::WarmUpMap(TSoftObjectPtr<UWorld> MapAsset, bool bShowDebug)
{
    const FSoftObjectPath Path = MapAsset.ToSoftObjectPath();
    const FString PackagePath = Path.IsValid() ? Path.GetLongPackageName() : FString();
    WarmUpMapByPath(PackagePath, bShowDebug);
}

void UStageZeroSubsystem::WarmUpMapByPath(const FString& MapPackageOrObjectPath, bool bShowDebug)
{
    if (bWarmupRunning)
    {
        UE_LOG(LogStageZeroSubsystem, Warning, TEXT("StageZero: Warmup already running; ignoring WarmUpMap request (%s)."), *MapPackageOrObjectPath);
        StageZeroDebugMessage(this, bShowDebug, TEXT("StageZero: Warmup already running; WarmUpMap ignored"), FColor::Yellow);
        return;
    }
    if (ShouldSkipWarmupForRuntimeContext(bShowDebug))
    {
        FinishWarmupFailure(EStageZeroWarmupResult::Cancelled, TEXT("Warmup skipped on dedicated server."));
        return;
    }
    const FName PackageName = ResolveMapPackageNameFromInput(MapPackageOrObjectPath);
    if (PackageName == NAME_None)
    {
        UE_LOG(LogStageZeroSubsystem, Warning, TEXT("StageZero: WarmUpMap failed - invalid map identifier '%s'"), *MapPackageOrObjectPath);
        FinishWarmupFailure(EStageZeroWarmupResult::Failed, FString::Printf(TEXT("Invalid warmup map identifier: %s"), *MapPackageOrObjectPath));
        return;
    }

    if (UStageZeroWarmupManifest* Manifest = LoadManifestForWarmup())
    {
        WarmUpFromManifest(*Manifest, { PackageName }, bShowDebug);
    }
    else
    {
        UE_LOG(LogStageZeroSubsystem, Warning, TEXT("StageZero: Mat/tex warmup manifest not found. WarmUpMap skipped."));
        FinishWarmupFailure(EStageZeroWarmupResult::Failed, TEXT("Warmup manifest was not found."));
    }
}

void UStageZeroSubsystem::WarmUpWorldPartitionCells(const FString& MapPackageOrObjectPath, bool bShowDebug)
{
    if (bWarmupRunning)
    {
        UE_LOG(LogStageZeroSubsystem, Warning, TEXT("StageZero: Warmup already running; ignoring WarmUpWorldPartitionCells request (%s)."), *MapPackageOrObjectPath);
        StageZeroDebugMessage(this, bShowDebug, TEXT("StageZero: Warmup already running; WarmUpWorldPartitionCells ignored"), FColor::Yellow);
        return;
    }
    if (ShouldSkipWarmupForRuntimeContext(bShowDebug))
    {
        FinishWarmupFailure(EStageZeroWarmupResult::Cancelled, TEXT("Warmup skipped on dedicated server."));
        return;
    }

    const FName PackageName = ResolveMapPackageNameFromInput(MapPackageOrObjectPath);
    if (PackageName == NAME_None)
    {
        const FString FailureReason = FString::Printf(TEXT("Invalid World Partition map identifier: %s"), *MapPackageOrObjectPath);
        UE_LOG(LogStageZeroSubsystem, Warning, TEXT("StageZero: WarmUpWorldPartitionCells failed - %s"), *FailureReason);
        FinishWarmupFailure(EStageZeroWarmupResult::Failed, FailureReason);
        return;
    }

    UStageZeroWarmupManifest* Manifest = LoadManifestForWarmup();
    if (!Manifest)
    {
        UE_LOG(LogStageZeroSubsystem, Warning, TEXT("StageZero: WarmUpWorldPartitionCells failed - warmup manifest was not found."));
        FinishWarmupFailure(EStageZeroWarmupResult::Failed, TEXT("Warmup manifest was not found."));
        return;
    }

    TArray<FSoftObjectPath> Materials;
    TArray<FSoftObjectPath> Textures;
    TArray<FSoftObjectPath> NiagaraSystems;
    TArray<FSoftObjectPath> StaticMeshes;
    TArray<FSoftObjectPath> SkeletalMeshes;
    int32 CellCount = 0;
    bool bUsedMapFallback = false;
    FString FailureReason;
    if (!BuildAssetsFromWPCells(*Manifest, PackageName, Materials, Textures, NiagaraSystems, StaticMeshes, SkeletalMeshes, CellCount, bUsedMapFallback, FailureReason))
    {
        UE_LOG(LogStageZeroSubsystem, Warning, TEXT("StageZero: WarmUpWorldPartitionCells failed - %s"), *FailureReason);
        FinishWarmupFailure(EStageZeroWarmupResult::Failed, FailureReason);
        return;
    }

    if (bUsedMapFallback)
    {
        UE_LOG(LogStageZeroSubsystem, Warning,
            TEXT("StageZero: No baked World Partition cells were present for %s; using that map's baked manifest entry as a compatibility fallback."),
            *PackageName.ToString());
    }

    StartWarmupWithAssets(Materials, Textures, NiagaraSystems, StaticMeshes, SkeletalMeshes, PackageName, {}, bShowDebug);
}


void UStageZeroSubsystem::WarmUpWorldPartitionAnchors(const FString& MapPackageOrObjectPath, const TArray<FName>& AnchorNames, bool bShowDebug)
{
    if (bWarmupRunning)
    {
        UE_LOG(LogStageZeroSubsystem, Warning, TEXT("StageZero: Warmup already running; ignoring WarmUpWorldPartitionAnchors request (%s)."), *MapPackageOrObjectPath);
        StageZeroDebugMessage(this, bShowDebug, TEXT("StageZero: Warmup already running; WarmUpWorldPartitionAnchors ignored"), FColor::Yellow);
        return;
    }
    if (ShouldSkipWarmupForRuntimeContext(bShowDebug))
    {
        FinishWarmupFailure(EStageZeroWarmupResult::Cancelled, TEXT("Warmup skipped on dedicated server."));
        return;
    }

    const FName PackageName = ResolveMapPackageNameFromInput(MapPackageOrObjectPath);
    if (PackageName == NAME_None)
    {
        const FString FailureReason = FString::Printf(TEXT("Invalid World Partition map identifier: %s"), *MapPackageOrObjectPath);
        UE_LOG(LogStageZeroSubsystem, Warning, TEXT("StageZero: WarmUpWorldPartitionAnchors failed - %s"), *FailureReason);
        FinishWarmupFailure(EStageZeroWarmupResult::Failed, FailureReason);
        return;
    }

    UStageZeroWarmupManifest* Manifest = LoadManifestForWarmup();
    if (!Manifest)
    {
        UE_LOG(LogStageZeroSubsystem, Warning, TEXT("StageZero: WarmUpWorldPartitionAnchors failed - warmup manifest was not found."));
        FinishWarmupFailure(EStageZeroWarmupResult::Failed, TEXT("Warmup manifest was not found."));
        return;
    }

    TArray<FSoftObjectPath> Materials;
    TArray<FSoftObjectPath> Textures;
    TArray<FSoftObjectPath> NiagaraSystems;
    TArray<FSoftObjectPath> StaticMeshes;
    TArray<FSoftObjectPath> SkeletalMeshes;
    int32 AnchorCount = 0;
    FString FailureReason;
    if (!BuildAssetsFromWPAnchors(*Manifest, PackageName, AnchorNames, Materials, Textures, NiagaraSystems, StaticMeshes, SkeletalMeshes, AnchorCount, FailureReason))
    {
        UE_LOG(LogStageZeroSubsystem, Warning, TEXT("StageZero: WarmUpWorldPartitionAnchors failed - %s"), *FailureReason);
        FinishWarmupFailure(EStageZeroWarmupResult::Failed, FailureReason);
        return;
    }

    StartWarmupWithAssets(Materials, Textures, NiagaraSystems, StaticMeshes, SkeletalMeshes, PackageName, {}, bShowDebug);
}


void UStageZeroSubsystem::WarmUpWorldPartitionSplineRoutes(const FString& MapPackageOrObjectPath, const TArray<FName>& RouteNames, bool bShowDebug)
{
    if (bWarmupRunning)
    {
        UE_LOG(LogStageZeroSubsystem, Warning, TEXT("StageZero: Warmup already running; ignoring WarmUpWorldPartitionSplineRoutes request (%s)."), *MapPackageOrObjectPath);
        StageZeroDebugMessage(this, bShowDebug, TEXT("StageZero: Warmup already running; WarmUpWorldPartitionSplineRoutes ignored"), FColor::Yellow);
        return;
    }
    if (ShouldSkipWarmupForRuntimeContext(bShowDebug))
    {
        FinishWarmupFailure(EStageZeroWarmupResult::Cancelled, TEXT("Warmup skipped on dedicated server."));
        return;
    }

    const FName PackageName = ResolveMapPackageNameFromInput(MapPackageOrObjectPath);
    if (PackageName == NAME_None)
    {
        const FString FailureReason = FString::Printf(TEXT("Invalid World Partition map identifier: %s"), *MapPackageOrObjectPath);
        UE_LOG(LogStageZeroSubsystem, Warning, TEXT("StageZero: WarmUpWorldPartitionSplineRoutes failed - %s"), *FailureReason);
        FinishWarmupFailure(EStageZeroWarmupResult::Failed, FailureReason);
        return;
    }

    UStageZeroWarmupManifest* Manifest = LoadManifestForWarmup();
    if (!Manifest)
    {
        UE_LOG(LogStageZeroSubsystem, Warning, TEXT("StageZero: WarmUpWorldPartitionSplineRoutes failed - warmup manifest was not found."));
        FinishWarmupFailure(EStageZeroWarmupResult::Failed, TEXT("Warmup manifest was not found."));
        return;
    }

    TArray<FSoftObjectPath> Materials;
    TArray<FSoftObjectPath> Textures;
    TArray<FSoftObjectPath> NiagaraSystems;
    TArray<FSoftObjectPath> StaticMeshes;
    TArray<FSoftObjectPath> SkeletalMeshes;
    int32 RouteCount = 0;
    FString FailureReason;
    if (!BuildAssetsFromWPSplines(*Manifest, PackageName, RouteNames, Materials, Textures, NiagaraSystems, StaticMeshes, SkeletalMeshes, RouteCount, FailureReason))
    {
        UE_LOG(LogStageZeroSubsystem, Warning, TEXT("StageZero: WarmUpWorldPartitionSplineRoutes failed - %s"), *FailureReason);
        FinishWarmupFailure(EStageZeroWarmupResult::Failed, FailureReason);
        return;
    }

    StartWarmupWithAssets(Materials, Textures, NiagaraSystems, StaticMeshes, SkeletalMeshes, PackageName, {}, bShowDebug);
}


FName UStageZeroSubsystem::ResolveMapPackageNameFromInput(const FString& MapId) const
{
    if (MapId.IsEmpty())
    {
        return NAME_None;
    }

    FString PackageName = MapId;
    if (FPackageName::IsValidObjectPath(MapId))
    {
        PackageName = FPackageName::ObjectPathToPackageName(MapId);
    }
    else if (!FPackageName::IsValidLongPackageName(MapId))
    {
        FString Converted;
        if (FPackageName::TryConvertFilenameToLongPackageName(MapId, Converted))
        {
            PackageName = Converted;
        }
    }

    return PackageName.IsEmpty() ? NAME_None : FName(*PackageName);
}

void UStageZeroSubsystem::WarmUpFromManifest(const UStageZeroWarmupManifest& Manifest, const TArray<FName>& MapPackages, bool bShowDebug)
{
    if (!bWarmupRunning)
    {
        ResetWarmupTrackingState();
        bWarmupRunning = true;
        bWarmupCompleted = false;
        bWasCancelled = false;
        WarmupResult = EStageZeroWarmupResult::Running;
        WarmupProgress = 0.0f;
        WarmupStartTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    }

    TArray<FSoftObjectPath> Materials;
    TArray<FSoftObjectPath> Textures;
    TArray<FSoftObjectPath> NiagaraSystems;
    TArray<FSoftObjectPath> StaticMeshes;
    TArray<FSoftObjectPath> SkeletalMeshes;
    FName Label = MapPackages.Num() == 1 ? MapPackages[0] : FName(TEXT("AllMaps"));
    TArray<FStageZeroManifestMapCoverage> MapCoveragePlan;

    const auto AddPathsToCoverage = [](TSet<FSoftObjectPath>& Destination, const TArray<FSoftObjectPath>& Source)
    {
        for (const FSoftObjectPath& Path : Source)
        {
            Destination.Add(Path);
        }
    };
    const auto AddMapCoverage = [&MapCoveragePlan, &AddPathsToCoverage](const FStageZeroMapMatTexEntry& Entry)
    {
        if (Entry.MapPackageName == NAME_None)
        {
            return;
        }

        FStageZeroManifestMapCoverage* Coverage = MapCoveragePlan.FindByPredicate([&Entry](const FStageZeroManifestMapCoverage& Candidate)
        {
            return Candidate.MapName == Entry.MapPackageName;
        });
        if (!Coverage)
        {
            Coverage = &MapCoveragePlan.AddDefaulted_GetRef();
            Coverage->MapName = Entry.MapPackageName;
        }

        AddPathsToCoverage(Coverage->RequiredPaths, Entry.Materials);
        AddPathsToCoverage(Coverage->RequiredPaths, Entry.Textures);
        AddPathsToCoverage(Coverage->RequiredPaths, Entry.NiagaraSystems);
        AddPathsToCoverage(Coverage->RequiredPaths, Entry.StaticMeshes);
        AddPathsToCoverage(Coverage->RequiredPaths, Entry.SkeletalMeshes);
    };

    if (MapPackages.Num() == 0)
    {
        // Preserve logical per-map membership before flattening to the manifest's
        // aggregate arrays. The executor still performs one globally deduplicated run.
        for (const FStageZeroMapMatTexEntry& Entry : Manifest.MapEntries)
        {
            AddMapCoverage(Entry);
        }

        Materials = Manifest.AllMaterials;
        Textures = Manifest.AllTextures;
        NiagaraSystems = Manifest.AllNiagaraSystems;
        StaticMeshes = Manifest.AllStaticMeshes;
        SkeletalMeshes = Manifest.AllSkeletalMeshes;
    }
    else
    {
        TArray<const FStageZeroMapMatTexEntry*> SelectedEntries;
        SelectedEntries.Reserve(MapPackages.Num());
        for (const FName& Package : MapPackages)
        {
            const FStageZeroMapMatTexEntry* Found = Manifest.MapEntries.FindByPredicate([&](const FStageZeroMapMatTexEntry& Entry)
            {
                return Entry.MapPackageName == Package;
            });

            if (Found)
            {
                SelectedEntries.Add(Found);
            }
            else
            {
                UE_LOG(LogStageZeroSubsystem, Warning, TEXT("StageZero: No manifest entry for map %s"), *Package.ToString());
            }
        }

        if (SelectedEntries.Num() != MapPackages.Num())
        {
            FinishWarmupFailure(EStageZeroWarmupResult::Failed,
                FString::Printf(TEXT("The Stage Zero manifest contained %d of %d requested map entries. Warmup was not started with partial map coverage."),
                    SelectedEntries.Num(),
                    MapPackages.Num()));
            return;
        }

        for (const FStageZeroMapMatTexEntry* Entry : SelectedEntries)
        {
            AddMapCoverage(*Entry);
        }
        for (const FStageZeroMapMatTexEntry* Entry : SelectedEntries)
        {
            Materials.Append(Entry->Materials);
            Textures.Append(Entry->Textures);
            NiagaraSystems.Append(Entry->NiagaraSystems);
            StaticMeshes.Append(Entry->StaticMeshes);
            SkeletalMeshes.Append(Entry->SkeletalMeshes);
        }
    }

    StartWarmupWithAssets(Materials, Textures, NiagaraSystems, StaticMeshes, SkeletalMeshes, Label, MapCoveragePlan, bShowDebug);
}

void UStageZeroSubsystem::StartWarmupWithAssets(const TArray<FSoftObjectPath>& Materials, const TArray<FSoftObjectPath>& Textures, const TArray<FSoftObjectPath>& NiagaraSystems, const TArray<FSoftObjectPath>& StaticMeshes, const TArray<FSoftObjectPath>& SkeletalMeshes, const FName& Label, const TArray<FStageZeroManifestMapCoverage>& MapCoveragePlan, bool bShowDebug)
{
    const bool bContinuingPreparedRun = bWarmupRunning;
    if (!bContinuingPreparedRun)
    {
        ResetWarmupTrackingState();
    }

    bWarmupRunning = true;
    bWarmupCompleted = false;
    bWasCancelled = false;
    WarmupResult = EStageZeroWarmupResult::Running;
    WarmupProgress = 0.0f;
    WarmupStartTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    CompletedMaps = 0;
    if (bShouldSpawnWidget)
    {
        SpawnBuiltInWidgetIfConfigured();
    }
    StopBootMusic(true);
    const FStageZeroBootConfig BootConfig = BuildBootConfig();
    if (ShouldUseSubsystemBootMusic(BootConfig))
    {
        StartBootMusic(BootConfig);
    }

    TArray<FSoftObjectPath> DedupedMaterials = Materials;
    TArray<FSoftObjectPath> DedupedTextures = Textures;
    TArray<FSoftObjectPath> DedupedNiagara = NiagaraSystems;
    TArray<FSoftObjectPath> DedupedStaticMeshes = StaticMeshes;
    TArray<FSoftObjectPath> DedupedSkeletalMeshes = SkeletalMeshes;
    const auto StableDeduplicate = [](TArray<FSoftObjectPath>& Paths)
    {
        TSet<FSoftObjectPath> Seen;
        Paths.RemoveAll([&Seen](const FSoftObjectPath& Path)
        {
            if (Seen.Contains(Path))
            {
                return true;
            }
            Seen.Add(Path);
            return false;
        });
    };
    StableDeduplicate(DedupedMaterials);
    StableDeduplicate(DedupedTextures);
    StableDeduplicate(DedupedNiagara);
    StableDeduplicate(DedupedStaticMeshes);
    StableDeduplicate(DedupedSkeletalMeshes);

    TSet<FSoftObjectPath> AggregatePaths;
    const auto AddAggregatePaths = [&AggregatePaths](const TArray<FSoftObjectPath>& Paths)
    {
        for (const FSoftObjectPath& Path : Paths)
        {
            AggregatePaths.Add(Path);
        }
    };
    AddAggregatePaths(DedupedMaterials);
    AddAggregatePaths(DedupedTextures);
    AddAggregatePaths(DedupedNiagara);
    AddAggregatePaths(DedupedStaticMeshes);
    AddAggregatePaths(DedupedSkeletalMeshes);

    CurrentMapName = Label;
    FString MapCoverageFailureReason;
    if (!InitializeWarmupMapCoverage(MapCoveragePlan, Label, AggregatePaths, MapCoverageFailureReason))
    {
        FinishWarmupFailure(EStageZeroWarmupResult::Failed, MapCoverageFailureReason);
        return;
    }
    const int32 EffectiveMapCount = FMath::Max(1, ActiveWarmupMapCoverage.Num());
    BroadcastWarmupStartedIfNeeded();
    BroadcastWarmupMapCoverageStarts();

    ManifestTaskTotal = WarmUpMatTexList(DedupedMaterials, DedupedTextures, DedupedNiagara, DedupedStaticMeshes, DedupedSkeletalMeshes, Label);
    const bool bNoManifestAssets = DedupedMaterials.Num() == 0 && DedupedTextures.Num() == 0 && DedupedNiagara.Num() == 0 && DedupedStaticMeshes.Num() == 0 && DedupedSkeletalMeshes.Num() == 0;
    if (ManifestTaskTotal <= 0 && bNoManifestAssets && !bPipelineCacheStageAccepted)
    {
        FinishWarmupFailure(EStageZeroWarmupResult::Failed, TEXT("No manifest assets or observed recorded PSO cache work were available."));
        return;
    }

    SnapshotJobBaselines();

    StartRealJobPolling(EffectiveMapCount);

    const int32 ManifestAssetCount = DedupedMaterials.Num() + DedupedTextures.Num() + DedupedNiagara.Num() + DedupedStaticMeshes.Num() + DedupedSkeletalMeshes.Num();
    if (ManifestAssetCount < 50)
    {
        UE_LOG(LogStageZeroSubsystem, Warning, TEXT("StageZero: Manifest contains only %d assets. Consider rebuilding StageZeroWarmupManifest if totals look too low."), ManifestAssetCount);
    }
    StageZeroDebugMessage(this, bShowDebug, FString::Printf(TEXT("StageZero: Warmup enqueued. Manifest=%d (Mat=%d Tex=%d Niagara=%d StaticMesh=%d SkeletalMesh=%d)"), ManifestAssetCount, DedupedMaterials.Num(), DedupedTextures.Num(), DedupedNiagara.Num(), DedupedStaticMeshes.Num(), DedupedSkeletalMeshes.Num()));
}

bool UStageZeroSubsystem::BuildAssetsFromWPCells(
    const UStageZeroWarmupManifest& Manifest,
    const FName& MapPackage,
    TArray<FSoftObjectPath>& OutMaterials,
    TArray<FSoftObjectPath>& OutTextures,
    TArray<FSoftObjectPath>& OutNiagara,
    TArray<FSoftObjectPath>& OutStaticMeshes,
    TArray<FSoftObjectPath>& OutSkeletalMeshes,
    int32& OutCellCount,
    bool& bOutUsedMapFallback,
    FString& OutFailureReason) const
{
    OutMaterials.Reset();
    OutTextures.Reset();
    OutNiagara.Reset();
    OutStaticMeshes.Reset();
    OutSkeletalMeshes.Reset();
    OutCellCount = 0;
    bOutUsedMapFallback = false;
    OutFailureReason.Reset();

    for (const FStageZeroWPCellEntry& Cell : Manifest.WPCells)
    {
        if (Cell.MapPackageName != MapPackage)
        {
            continue;
        }

        ++OutCellCount;
        OutMaterials.Append(Cell.Materials);
        OutTextures.Append(Cell.Textures);
        OutNiagara.Append(Cell.NiagaraSystems);
        OutStaticMeshes.Append(Cell.StaticMeshes);
        OutSkeletalMeshes.Append(Cell.SkeletalMeshes);
    }

    if (OutCellCount > 0)
    {
        return true;
    }

    // Older/lightweight manifests may contain complete per-map coverage but no
    // cell records. This API has no subset filter, so the map entry is the only
    // compatibility fallback that cannot broaden a caller's requested subset.
    const FStageZeroMapMatTexEntry* MapEntry = Manifest.MapEntries.FindByPredicate([&MapPackage](const FStageZeroMapMatTexEntry& Entry)
    {
        return Entry.MapPackageName == MapPackage;
    });
    if (MapEntry)
    {
        OutMaterials.Append(MapEntry->Materials);
        OutTextures.Append(MapEntry->Textures);
        OutNiagara.Append(MapEntry->NiagaraSystems);
        OutStaticMeshes.Append(MapEntry->StaticMeshes);
        OutSkeletalMeshes.Append(MapEntry->SkeletalMeshes);
        bOutUsedMapFallback = true;
        return true;
    }

    OutFailureReason = FString::Printf(
        TEXT("No baked World Partition cell data or map entry exists for '%s'. Rebuild the Stage Zero manifest for this map."),
        *MapPackage.ToString());
    return false;
}

bool UStageZeroSubsystem::BuildAssetsFromWPAnchors(
    const UStageZeroWarmupManifest& Manifest,
    const FName& MapPackage,
    const TArray<FName>& AnchorNames,
    TArray<FSoftObjectPath>& OutMaterials,
    TArray<FSoftObjectPath>& OutTextures,
    TArray<FSoftObjectPath>& OutNiagara,
    TArray<FSoftObjectPath>& OutStaticMeshes,
    TArray<FSoftObjectPath>& OutSkeletalMeshes,
    int32& OutAnchorCount,
    FString& OutFailureReason) const
{
    OutMaterials.Reset();
    OutTextures.Reset();
    OutNiagara.Reset();
    OutStaticMeshes.Reset();
    OutSkeletalMeshes.Reset();
    OutAnchorCount = 0;
    OutFailureReason.Reset();

    TArray<FName> RequestedFilters;
    for (const FName& AnchorName : AnchorNames)
    {
        RequestedFilters.AddUnique(AnchorName);
    }

    const bool bFilter = RequestedFilters.Num() > 0;
    TSet<FName> MatchedFilters;
    TArray<FSoftObjectPath> SelectedMaterials;
    TArray<FSoftObjectPath> SelectedTextures;
    TArray<FSoftObjectPath> SelectedNiagara;
    TArray<FSoftObjectPath> SelectedStaticMeshes;
    TArray<FSoftObjectPath> SelectedSkeletalMeshes;
    int32 SelectedAnchorCount = 0;

    for (const FStageZeroWPAnchorGroup& Anchor : Manifest.WPAnchorGroups)
    {
        if (Anchor.MapPackageName != MapPackage)
        {
            continue;
        }

        const FName AnchorIdName = Anchor.AnchorId.IsEmpty() ? NAME_None : FName(*Anchor.AnchorId);
        bool bSelected = !bFilter;
        if (bFilter)
        {
            for (const FName& RequestedFilter : RequestedFilters)
            {
                if (RequestedFilter == NAME_None)
                {
                    continue;
                }

                if (Anchor.GroupName == RequestedFilter || AnchorIdName == RequestedFilter)
                {
                    bSelected = true;
                    MatchedFilters.Add(RequestedFilter);
                }
            }
        }

        if (!bSelected)
        {
            continue;
        }

        ++SelectedAnchorCount;
        SelectedMaterials.Append(Anchor.Materials);
        SelectedTextures.Append(Anchor.Textures);
        SelectedNiagara.Append(Anchor.NiagaraSystems);
        SelectedStaticMeshes.Append(Anchor.StaticMeshes);
        SelectedSkeletalMeshes.Append(Anchor.SkeletalMeshes);
    }

    TArray<FString> MissingFilterNames;
    if (bFilter)
    {
        for (const FName& RequestedFilter : RequestedFilters)
        {
            if (RequestedFilter == NAME_None || !MatchedFilters.Contains(RequestedFilter))
            {
                MissingFilterNames.Add(RequestedFilter.IsNone() ? TEXT("<None>") : RequestedFilter.ToString());
            }
        }
    }

    if (MissingFilterNames.Num() > 0)
    {
        OutFailureReason = FString::Printf(
            TEXT("World Partition anchor selector(s) [%s] were not found for map '%s'. No whole-map fallback was used."),
            *FString::Join(MissingFilterNames, TEXT(", ")),
            *MapPackage.ToString());
        return false;
    }

    if (SelectedAnchorCount == 0)
    {
        OutFailureReason = FString::Printf(
            TEXT("No baked World Partition anchor groups exist for map '%s'. Rebuild the manifest with anchor and spline route collection enabled."),
            *MapPackage.ToString());
        return false;
    }

    OutMaterials = MoveTemp(SelectedMaterials);
    OutTextures = MoveTemp(SelectedTextures);
    OutNiagara = MoveTemp(SelectedNiagara);
    OutStaticMeshes = MoveTemp(SelectedStaticMeshes);
    OutSkeletalMeshes = MoveTemp(SelectedSkeletalMeshes);
    OutAnchorCount = SelectedAnchorCount;
    return true;
}

bool UStageZeroSubsystem::BuildAssetsFromWPSplines(
    const UStageZeroWarmupManifest& Manifest,
    const FName& MapPackage,
    const TArray<FName>& RouteNames,
    TArray<FSoftObjectPath>& OutMaterials,
    TArray<FSoftObjectPath>& OutTextures,
    TArray<FSoftObjectPath>& OutNiagara,
    TArray<FSoftObjectPath>& OutStaticMeshes,
    TArray<FSoftObjectPath>& OutSkeletalMeshes,
    int32& OutRouteCount,
    FString& OutFailureReason) const
{
    OutMaterials.Reset();
    OutTextures.Reset();
    OutNiagara.Reset();
    OutStaticMeshes.Reset();
    OutSkeletalMeshes.Reset();
    OutRouteCount = 0;
    OutFailureReason.Reset();

    TArray<FName> RequestedFilters;
    for (const FName& RouteName : RouteNames)
    {
        RequestedFilters.AddUnique(RouteName);
    }

    const bool bFilter = RequestedFilters.Num() > 0;
    TSet<FName> MatchedFilters;
    TArray<FSoftObjectPath> SelectedMaterials;
    TArray<FSoftObjectPath> SelectedTextures;
    TArray<FSoftObjectPath> SelectedNiagara;
    TArray<FSoftObjectPath> SelectedStaticMeshes;
    TArray<FSoftObjectPath> SelectedSkeletalMeshes;
    int32 SelectedRouteCount = 0;

    for (const FStageZeroWPSplineGroup& Route : Manifest.WPSplineGroups)
    {
        if (Route.MapPackageName != MapPackage)
        {
            continue;
        }

        const FName RouteIdName = Route.RouteId.IsEmpty() ? NAME_None : FName(*Route.RouteId);
        bool bSelected = !bFilter;
        if (bFilter)
        {
            for (const FName& RequestedFilter : RequestedFilters)
            {
                if (RequestedFilter == NAME_None)
                {
                    continue;
                }

                if (Route.GroupName == RequestedFilter || RouteIdName == RequestedFilter)
                {
                    bSelected = true;
                    MatchedFilters.Add(RequestedFilter);
                }
            }
        }

        if (!bSelected)
        {
            continue;
        }

        ++SelectedRouteCount;
        SelectedMaterials.Append(Route.Materials);
        SelectedTextures.Append(Route.Textures);
        SelectedNiagara.Append(Route.NiagaraSystems);
        SelectedStaticMeshes.Append(Route.StaticMeshes);
        SelectedSkeletalMeshes.Append(Route.SkeletalMeshes);
    }

    TArray<FString> MissingFilterNames;
    if (bFilter)
    {
        for (const FName& RequestedFilter : RequestedFilters)
        {
            if (RequestedFilter == NAME_None || !MatchedFilters.Contains(RequestedFilter))
            {
                MissingFilterNames.Add(RequestedFilter.IsNone() ? TEXT("<None>") : RequestedFilter.ToString());
            }
        }
    }

    if (MissingFilterNames.Num() > 0)
    {
        OutFailureReason = FString::Printf(
            TEXT("World Partition spline route selector(s) [%s] were not found for map '%s'. No whole-map fallback was used."),
            *FString::Join(MissingFilterNames, TEXT(", ")),
            *MapPackage.ToString());
        return false;
    }

    if (SelectedRouteCount == 0)
    {
        OutFailureReason = FString::Printf(
            TEXT("No baked World Partition spline routes exist for map '%s'. Rebuild the manifest with anchor and spline route collection enabled."),
            *MapPackage.ToString());
        return false;
    }

    OutMaterials = MoveTemp(SelectedMaterials);
    OutTextures = MoveTemp(SelectedTextures);
    OutNiagara = MoveTemp(SelectedNiagara);
    OutStaticMeshes = MoveTemp(SelectedStaticMeshes);
    OutSkeletalMeshes = MoveTemp(SelectedSkeletalMeshes);
    OutRouteCount = SelectedRouteCount;
    return true;
}

int32 UStageZeroSubsystem::WarmUpMatTexList(const TArray<FSoftObjectPath>& Materials, const TArray<FSoftObjectPath>& Textures, const TArray<FSoftObjectPath>& NiagaraSystems, const TArray<FSoftObjectPath>& StaticMeshes, const TArray<FSoftObjectPath>& SkeletalMeshes, const FName& Label)
{
    ManifestJobQueue.Reset();
    ManifestJobCursor = 0;
    ManifestFailedLoads = 0;
    ManifestTasksCompleted = 0;

    TryStartRecordedPipelineCacheWarmup();

    int32 ImmediateTerminalCount = 0;
    TSet<FSoftObjectPath> EnqueuedPaths;
    auto EnqueueJob = [&](EStageZeroManifestJobType Type, const FSoftObjectPath& Path)
    {
        if (EnqueuedPaths.Contains(Path))
        {
            UE_LOG(LogStageZeroSubsystem, Warning,
                TEXT("StageZero: Ignoring duplicate manifest path across asset categories: %s"),
                *Path.ToString());
            return;
        }
        EnqueuedPaths.Add(Path);

        FStageZeroManifestJob Job;
        Job.Type = Type;
        Job.Path = Path;

        const FString AssetTypeLabel = (Type == EStageZeroManifestJobType::Material) ? TEXT("Material")
            : (Type == EStageZeroManifestJobType::Texture) ? TEXT("Texture")
            : (Type == EStageZeroManifestJobType::StaticMesh) ? TEXT("StaticMesh")
            : (Type == EStageZeroManifestJobType::SkeletalMesh) ? TEXT("SkeletalMesh")
            : TEXT("Niagara");

        TrackWarmupJobRequested(Path, AssetTypeLabel, /*bInManifest*/ true);
        if (!IsLoadableManifestAssetPath(Path))
        {
            RecordManifestFailure(Job, TEXT("Manifest path is invalid, editor-only, or unavailable at runtime."));
            TrackWarmupJobFinished(Path, EStageZeroWarmupJobTerminalState::Failed,
                TEXT("Manifest path is invalid, editor-only, or unavailable at runtime."));
            ++ImmediateTerminalCount;
            return;
        }
        ManifestJobQueue.Add(Job);
    };

    for (const FSoftObjectPath& Path : Materials)
    {
        EnqueueJob(EStageZeroManifestJobType::Material, Path);
    }
    for (const FSoftObjectPath& Path : Textures)
    {
        EnqueueJob(EStageZeroManifestJobType::Texture, Path);
    }
    for (const FSoftObjectPath& Path : NiagaraSystems)
    {
        EnqueueJob(EStageZeroManifestJobType::Niagara, Path);
    }
    for (const FSoftObjectPath& Path : StaticMeshes)
    {
        EnqueueJob(EStageZeroManifestJobType::StaticMesh, Path);
    }
    for (const FSoftObjectPath& Path : SkeletalMeshes)
    {
        EnqueueJob(EStageZeroManifestJobType::SkeletalMesh, Path);
    }
    ManifestTaskTotal = ManifestJobQueue.Num() + ImmediateTerminalCount;
    ManifestTasksCompleted = ImmediateTerminalCount;
    TotalTasks = ManifestTaskTotal;
    CurrentMapTaskTotal = ManifestTaskTotal;
    CurrentMapName = Label;
    CompletedTasks = ManifestTasksCompleted;
    TaskBaseline = 0;

    UE_LOG(LogStageZeroSubsystem, Log, TEXT("[StageZero] Manifest asset load plan: Total=%d Mat=%d Tex=%d Niagara=%d StaticMesh=%d SkeletalMesh=%d ImmediateFailures=%d."),
        ManifestTaskTotal,
        Materials.Num(),
        Textures.Num(),
        NiagaraSystems.Num(),
        StaticMeshes.Num(),
        SkeletalMeshes.Num(),
        ImmediateTerminalCount);

    StartManifestProcessing();

    return CurrentMapTaskTotal;
}

void UStageZeroSubsystem::CancelWarmup(bool bShowDebug)
{
    if (!bWarmupRunning)
    {
        const bool bCancelledPendingPrimaryTransition = bLegacyTransitionWidgetAwaitingTravelCleanup;
        ClearRuntimeWarmupCallbacks();
        ClearLegacyTransitionPresentation();
        if (bCancelledPendingPrimaryTransition)
        {
            bLegacyTransitionMessageDispatched = false;
        }
        StageZeroDebugMessage(this, bShowDebug, TEXT("StageZero: No warmup was running; stale warmup callbacks were cleared."));
        return;
    }

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(FinishDelayTimer);
    }
    bWarmupFinishPending = false;
    bWasCancelled = true;
    MarkOutstandingManifestJobsCancelled(TEXT("Warmup was cancelled before this manifest item completed."));
    StageZeroDebugMessage(this, bShowDebug, TEXT("StageZero: Warmup cancelled"), FColor::Yellow);
    FinishWarmupInternal(true);
}

void UStageZeroSubsystem::StartManifestProcessing()
{
    if (!GetWorld())
    {
        FinishWarmupFailure(EStageZeroWarmupResult::Failed, TEXT("No valid game world was available for manifest processing."));
        return;
    }

    bManifestProcessingActive = ManifestJobCursor < ManifestJobQueue.Num();

    GetWorld()->GetTimerManager().ClearTimer(ManifestJobTimer);
    if (bManifestProcessingActive)
    {
        ManifestRunDeadlineTimeSeconds = FPlatformTime::Seconds() + GetManifestAssetLoadTimeoutSeconds();
        if (ManifestRunDeadlineTickerHandle.IsValid())
        {
            FTSTicker::GetCoreTicker().RemoveTicker(ManifestRunDeadlineTickerHandle);
        }
        ManifestRunDeadlineTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateUObject(this, &UStageZeroSubsystem::TickManifestRunDeadline),
            0.1f);
        ManifestJobTimer = GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UStageZeroSubsystem::ProcessManifestJobBatch);
    }
    else
    {
        BroadcastWarmupStatus();
    }
}

bool UStageZeroSubsystem::TickManifestRunDeadline(float DeltaSeconds)
{
    (void)DeltaSeconds;

    if (!bWarmupRunning || !bManifestProcessingActive)
    {
        ManifestRunDeadlineTickerHandle.Reset();
        ManifestRunDeadlineTimeSeconds = 0.0;
        return false;
    }

    if (ManifestRunDeadlineTimeSeconds <= 0.0 || FPlatformTime::Seconds() < ManifestRunDeadlineTimeSeconds)
    {
        return true;
    }

    const float TimeoutSeconds = GetManifestAssetLoadTimeoutSeconds();
    const FString Reason = FString::Printf(
        TEXT("Manifest asset preload exceeded its %.1f second wall-clock deadline."),
        TimeoutSeconds);

    ManifestRunDeadlineTickerHandle.Reset();
    ManifestRunDeadlineTimeSeconds = 0.0;
    ++ManifestLoadRequestSerial;
    ActiveManifestLoadRequestSerial = 0;
    ActiveManifestLoadStartTimeSeconds = 0.0;
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ManifestJobTimer);
        World->GetTimerManager().ClearTimer(ManifestLoadTimeoutTimer);
    }
    if (ActiveManifestLoadHandle.IsValid())
    {
        ActiveManifestLoadHandle->CancelHandle();
        ActiveManifestLoadHandle.Reset();
    }

    MarkOutstandingManifestJobsTimedOut(Reason);
    ActiveManifestBatch.Reset();
    ManifestJobQueue.Reset();
    ManifestJobCursor = 0;
    bManifestProcessingActive = false;
    UE_LOG(LogStageZeroSubsystem, Error, TEXT("StageZero: %s"), *Reason);
    FinishWarmupInternal(
        false,
        IsStrictCompletionEnabled() ? EStageZeroWarmupResult::TimedOut : EStageZeroWarmupResult::Partial,
        Reason);
    return false;
}

void UStageZeroSubsystem::AdvanceManifestProgress(int32 Delta)
{
    ManifestTasksCompleted = FMath::Clamp(ManifestTasksCompleted + Delta, 0, FMath::Max(ManifestTaskTotal, 0));
    const bool bShouldBroadcast = (ManifestTasksCompleted % 25 == 0) || (ManifestTasksCompleted >= ManifestTaskTotal);
    const bool bSmallManifest = ManifestTaskTotal > 0 && ManifestTaskTotal <= 100;
    const bool bFirstVisibleStep = ManifestTasksCompleted > 0 && LastBroadcastCompletedTasks == 0;
    const bool bShouldBroadcastSmallStep = bSmallManifest && Delta > 0;
    if (bShouldBroadcast || bFirstVisibleStep || bShouldBroadcastSmallStep)
    {
        UpdateProgressBroadcast(ManifestTasksCompleted, CurrentMapTaskTotal);
    }
}

void UStageZeroSubsystem::ProcessManifestJobBatch()
{
    if (!bWarmupRunning || !GetWorld())
    {
        if (GetWorld())
        {
            GetWorld()->GetTimerManager().ClearTimer(ManifestJobTimer);
        }
        bManifestProcessingActive = false;
        if (bWarmupRunning)
        {
            MarkOutstandingManifestJobsCancelled(TEXT("The game world became unavailable during manifest loading."));
            FinishWarmupInternal(false, EStageZeroWarmupResult::Failed,
                TEXT("The game world became unavailable during manifest loading."));
        }
        return;
    }

    if (ActiveManifestLoadRequestSerial != 0 || ActiveManifestLoadHandle.IsValid())
    {
        return;
    }

    const int32 BatchSize = FMath::Max(1, ManifestBatchSize);
    ActiveManifestBatch.Reset();
    TArray<FSoftObjectPath> BatchPaths;
    BatchPaths.Reserve(BatchSize);

    while (ManifestJobCursor < ManifestJobQueue.Num() && ActiveManifestBatch.Num() < BatchSize)
    {
        const FStageZeroManifestJob& Job = ManifestJobQueue[ManifestJobCursor++];
        ActiveManifestBatch.Add(Job);
        TrackWarmupJobLoading(Job.Path);

        if (Job.Path.IsValid())
        {
            BatchPaths.AddUnique(Job.Path);
        }
    }

    if (ActiveManifestBatch.Num() == 0)
    {
        CompleteManifestBatchAndContinue();
        return;
    }

    if (!ShouldUseAsyncWarmupLoading())
    {
        TArray<FStageZeroManifestJob> SynchronouslyLoadedJobs;
        SynchronouslyLoadedJobs.Reserve(ActiveManifestBatch.Num());

        for (const FStageZeroManifestJob& Job : ActiveManifestBatch)
        {
            UObject* LoadedObject = Job.Path.IsValid() ? Job.Path.ResolveObject() : nullptr;
            if (!LoadedObject && Job.Path.IsValid())
            {
                LoadedObject = Job.Path.TryLoad();
            }

            if (LoadedObject)
            {
                SynchronouslyLoadedJobs.Add(Job);
                continue;
            }

            const FString FailureReason = TEXT("Synchronous manifest asset load failed to resolve the object.");
            RecordManifestFailure(Job, FailureReason);
            TrackWarmupJobFinished(Job.Path, EStageZeroWarmupJobTerminalState::Failed, FailureReason);
            AdvanceManifestProgress(1);
        }

        ProcessLoadedManifestBatch(SynchronouslyLoadedJobs);
        ActiveManifestBatch.Reset();
        CompleteManifestBatchAndContinue();
        return;
    }

    if (BatchPaths.Num() > 0)
    {
        const uint64 RequestSerial = ++ManifestLoadRequestSerial;
        ActiveManifestLoadRequestSerial = RequestSerial;
        ActiveManifestLoadStartTimeSeconds = FPlatformTime::Seconds();
        ActiveManifestLoadHandle = ManifestStreamableManager.RequestAsyncLoad(
            BatchPaths,
            FStreamableDelegate::CreateUObject(this, &UStageZeroSubsystem::HandleManifestAsyncLoadComplete, RequestSerial),
            FStreamableManager::DefaultAsyncLoadPriority,
            false,
            true,
            TEXT("StageZeroManifestBatch"));

        if (ActiveManifestLoadHandle.IsValid())
        {
            const TSharedPtr<FStreamableHandle> HandleToStart = ActiveManifestLoadHandle;
            FTimerDelegate TimeoutDelegate = FTimerDelegate::CreateUObject(
                this,
                &UStageZeroSubsystem::HandleManifestAsyncLoadTimeout,
                RequestSerial);
            GetWorld()->GetTimerManager().SetTimer(
                ManifestLoadTimeoutTimer,
                TimeoutDelegate,
                StageZeroManifestBatchLoadTimeoutSeconds,
                false);
            // Starting only after the handle, serial, and timeout are owned makes
            // already-loaded assets safe even when delegates execute immediately.
            HandleToStart->StartStalledHandle();
            return;
        }

        ActiveManifestLoadRequestSerial = 0;
        ActiveManifestLoadStartTimeSeconds = 0.0;
    }

    const FString RequestFailure = TEXT("The asynchronous manifest load request could not be created. Synchronous fallback is intentionally disabled.");
    for (const FStageZeroManifestJob& Job : ActiveManifestBatch)
    {
        RecordManifestFailure(Job, RequestFailure);
        TrackWarmupJobFinished(Job.Path, EStageZeroWarmupJobTerminalState::Failed, RequestFailure);
        AdvanceManifestProgress(1);
    }
    ActiveManifestBatch.Reset();
    CompleteManifestBatchAndContinue();
}

void UStageZeroSubsystem::HandleManifestAsyncLoadComplete(uint64 RequestSerial)
{
    if (RequestSerial == 0 || RequestSerial != ActiveManifestLoadRequestSerial)
    {
        return;
    }

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(ManifestLoadTimeoutTimer);
    }
    ActiveManifestLoadRequestSerial = 0;
    ActiveManifestLoadStartTimeSeconds = 0.0;
    ActiveManifestLoadHandle.Reset();

    if (!bWarmupRunning)
    {
        MarkOutstandingManifestJobsCancelled(TEXT("Warmup ended before the async asset load callback completed."));
        ActiveManifestBatch.Reset();
        bManifestProcessingActive = false;
        return;
    }

    if (!GetWorld())
    {
        for (const FStageZeroManifestJob& Job : ActiveManifestBatch)
        {
            RecordManifestFailure(Job, TEXT("The game world became unavailable before the async asset load completed."));
            TrackWarmupJobFinished(Job.Path, EStageZeroWarmupJobTerminalState::Failed,
                TEXT("The game world became unavailable before the async asset load completed."));
            AdvanceManifestProgress(1);
        }
        ActiveManifestBatch.Reset();
        bManifestProcessingActive = false;
        FinishWarmupInternal(false, EStageZeroWarmupResult::Failed,
            TEXT("The game world became unavailable during manifest loading."));
        return;
    }

    ProcessLoadedManifestBatch(ActiveManifestBatch);
    ActiveManifestBatch.Reset();
    CompleteManifestBatchAndContinue();
}

void UStageZeroSubsystem::HandleManifestAsyncLoadTimeout(uint64 RequestSerial)
{
    if (RequestSerial == 0 || RequestSerial != ActiveManifestLoadRequestSerial)
    {
        return;
    }

    const double ElapsedSeconds = ActiveManifestLoadStartTimeSeconds > 0.0
        ? FPlatformTime::Seconds() - ActiveManifestLoadStartTimeSeconds
        : StageZeroManifestBatchLoadTimeoutSeconds;
    ActiveManifestLoadRequestSerial = 0;
    ActiveManifestLoadStartTimeSeconds = 0.0;

    TSharedPtr<FStreamableHandle> TimedOutHandle = MoveTemp(ActiveManifestLoadHandle);
    TArray<FStageZeroManifestJob> ResolvedJobs;
    for (const FStageZeroManifestJob& Job : ActiveManifestBatch)
    {
        if (Job.Path.IsValid() && Job.Path.ResolveObject())
        {
            ResolvedJobs.Add(Job);
        }
        else
        {
            RecordManifestFailure(Job,
                FString::Printf(TEXT("Asset load exceeded the %.0f second batch deadline."), StageZeroManifestBatchLoadTimeoutSeconds));
            TrackWarmupJobFinished(Job.Path, EStageZeroWarmupJobTerminalState::TimedOut,
                FString::Printf(TEXT("Asset load exceeded the %.0f second batch deadline."), StageZeroManifestBatchLoadTimeoutSeconds));
            AdvanceManifestProgress(1);
        }
    }

    ProcessLoadedManifestBatch(ResolvedJobs);
    ActiveManifestBatch.Reset();
    if (TimedOutHandle.IsValid())
    {
        TimedOutHandle->CancelHandle();
    }

    UE_LOG(LogStageZeroSubsystem, Error,
        TEXT("StageZero: Manifest asset batch timed out after %.1f seconds; every item was recorded as loaded or failed."),
        ElapsedSeconds);
    CompleteManifestBatchAndContinue();
}

void UStageZeroSubsystem::CompleteManifestBatchAndContinue()
{
    if (!bWarmupRunning || !GetWorld())
    {
        bManifestProcessingActive = false;
        if (bWarmupRunning)
        {
            MarkOutstandingManifestJobsCancelled(TEXT("The game world became unavailable before manifest processing completed."));
            FinishWarmupInternal(false, EStageZeroWarmupResult::Failed,
                TEXT("The game world became unavailable before manifest processing completed."));
        }
        return;
    }

    if (ManifestJobCursor >= ManifestJobQueue.Num())
    {
        ManifestJobQueue.Reset();
        ManifestJobCursor = 0;
        bManifestProcessingActive = false;
        GetWorld()->GetTimerManager().ClearTimer(ManifestJobTimer);
        if (ManifestFailedLoads > 0)
        {
            UE_LOG(LogStageZeroSubsystem, Warning,
                TEXT("StageZero: %d required manifest asset(s) reached a failed terminal state."),
                ManifestFailedLoads);
        }
        PollRealJobProgress();
        return;
    }

    ManifestJobTimer = GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UStageZeroSubsystem::ProcessManifestJobBatch);
}

void UStageZeroSubsystem::ProcessLoadedManifestBatch(const TArray<FStageZeroManifestJob>& Jobs)
{
    for (const FStageZeroManifestJob& Job : Jobs)
    {
        if (!Job.Path.IsValid())
        {
            RecordManifestFailure(Job, TEXT("Invalid soft object path."));
            TrackWarmupJobFinished(Job.Path, EStageZeroWarmupJobTerminalState::Failed, TEXT("Invalid soft object path."));
            AdvanceManifestProgress(1);
            continue;
        }

        UObject* LoadedObject = Job.Path.ResolveObject();
        if (!LoadedObject)
        {
            RecordManifestFailure(Job, TEXT("Async load completed without resolving the asset."));
            TrackWarmupJobFinished(Job.Path, EStageZeroWarmupJobTerminalState::Failed,
                TEXT("Asset load completed without resolving the object."));
            AdvanceManifestProgress(1);
            continue;
        }

        if (!FStageZeroWarmupUtils::IsRuntimeWarmupAssetAllowed(LoadedObject))
        {
            RecordManifestFailure(Job, TEXT("Loaded object is editor-only or otherwise unavailable to runtime warmup."));
            TrackWarmupJobFinished(Job.Path, EStageZeroWarmupJobTerminalState::Failed,
                TEXT("Loaded object is editor-only or otherwise unavailable to runtime warmup."));
            AdvanceManifestProgress(1);
            continue;
        }

        bool bTypeMatches = false;
        switch (Job.Type)
        {
            case EStageZeroManifestJobType::Material:
                bTypeMatches = LoadedObject->IsA<UMaterialInterface>();
                break;
            case EStageZeroManifestJobType::Texture:
                bTypeMatches = LoadedObject->IsA<UTexture>() && !LoadedObject->IsA<UTextureRenderTarget>();
                break;
            case EStageZeroManifestJobType::Niagara:
                bTypeMatches = LoadedObject->IsA<UNiagaraSystem>();
                break;
            case EStageZeroManifestJobType::StaticMesh:
                bTypeMatches = LoadedObject->IsA<UStaticMesh>();
                break;
            case EStageZeroManifestJobType::SkeletalMesh:
                bTypeMatches = LoadedObject->IsA<USkeletalMesh>();
                break;
            case EStageZeroManifestJobType::PSO:
            default:
                break;
        }

        if (bTypeMatches)
        {
            RetainedWarmupAssets.AddUnique(LoadedObject);
            TrackWarmupJobFinished(Job.Path, EStageZeroWarmupJobTerminalState::Succeeded);
        }
        else
        {
            const FString TypeFailure = FString::Printf(TEXT("Loaded object type '%s' does not match the manifest job type '%s'."),
                *LoadedObject->GetClass()->GetName(),
                StageZeroManifestJobTypeName(Job.Type));
            RecordManifestFailure(Job, TypeFailure);
            TrackWarmupJobFinished(Job.Path, EStageZeroWarmupJobTerminalState::Failed, TypeFailure);
        }
        AdvanceManifestProgress(1);
    }
}

void UStageZeroSubsystem::RecordManifestFailure(const FStageZeroManifestJob& Job, const FString& Reason)
{
    ++ManifestFailedLoads;

    const TCHAR* TypeName = StageZeroManifestJobTypeName(Job.Type);

    const FString PathText = Job.Path.IsValid() ? Job.Path.ToString() : TEXT("<invalid>");
    const FString Failure = FString::Printf(TEXT("%s [%s]: %s"), *PathText, TypeName, *Reason);
    if (Job.Path.IsValid())
    {
        if (!FailedWarmupAssetPaths.Contains(Job.Path))
        {
            FailedWarmupAssetPaths.Add(Job.Path);
            FailedWarmupAssets.Add(Failure);
        }
    }
    else
    {
        FailedWarmupAssets.Add(Failure);
    }

    UE_LOG(LogStageZeroSubsystem, Warning, TEXT("StageZero: Manifest asset warmup failed: %s"), *Failure);
}

bool UStageZeroSubsystem::InitializeWarmupMapCoverage(
    const TArray<FStageZeroManifestMapCoverage>& MapCoveragePlan,
    const FName& FallbackMapName,
    const TSet<FSoftObjectPath>& AggregatePaths,
    FString& OutFailureReason)
{
    ActiveWarmupMapCoverage.Reset();
    ManifestPathMapCoverageMembership.Reset();
    CompletedMaps = 0;
    TargetMapCount = 0;
    OutFailureReason.Reset();

    const FName EffectiveFallbackName = FallbackMapName != NAME_None
        ? FallbackMapName
        : FName(TEXT("AllMaps"));

    for (const FStageZeroManifestMapCoverage& PlannedCoverage : MapCoveragePlan)
    {
        for (const FSoftObjectPath& Path : PlannedCoverage.RequiredPaths)
        {
            if (!AggregatePaths.Contains(Path))
            {
                const FName MapName = PlannedCoverage.MapName != NAME_None
                    ? PlannedCoverage.MapName
                    : EffectiveFallbackName;
                OutFailureReason = FString::Printf(
                    TEXT("Manifest map coverage for '%s' referenced '%s', but that path was absent from the aggregate load plan. Warmup was not started with incomplete map coverage."),
                    *MapName.ToString(),
                    *Path.ToString());
                return false;
            }
        }
    }

    for (const FStageZeroManifestMapCoverage& PlannedCoverage : MapCoveragePlan)
    {
        const FName EffectiveMapName = PlannedCoverage.MapName != NAME_None
            ? PlannedCoverage.MapName
            : EffectiveFallbackName;
        FStageZeroManifestMapCoverage* Coverage = ActiveWarmupMapCoverage.FindByPredicate(
            [EffectiveMapName](const FStageZeroManifestMapCoverage& Candidate)
            {
                return Candidate.MapName == EffectiveMapName;
            });
        if (!Coverage)
        {
            Coverage = &ActiveWarmupMapCoverage.AddDefaulted_GetRef();
            Coverage->MapName = EffectiveMapName;
        }

        for (const FSoftObjectPath& Path : PlannedCoverage.RequiredPaths)
        {
            Coverage->RequiredPaths.Add(Path);
        }
    }

    if (ActiveWarmupMapCoverage.Num() == 0)
    {
        FStageZeroManifestMapCoverage& FallbackCoverage = ActiveWarmupMapCoverage.AddDefaulted_GetRef();
        FallbackCoverage.MapName = EffectiveFallbackName;
        FallbackCoverage.RequiredPaths = AggregatePaths;
    }

    for (int32 MapIndex = 0; MapIndex < ActiveWarmupMapCoverage.Num(); ++MapIndex)
    {
        FStageZeroManifestMapCoverage& Coverage = ActiveWarmupMapCoverage[MapIndex];
        Coverage.ProcessedTerminalPaths.Reset();
        Coverage.bStarted = false;
        Coverage.bFinished = false;
        for (const FSoftObjectPath& Path : Coverage.RequiredPaths)
        {
            ManifestPathMapCoverageMembership.FindOrAdd(Path).AddUnique(MapIndex);
        }
    }

    TargetMapCount = FMath::Max(1, ActiveWarmupMapCoverage.Num());
    return true;
}

void UStageZeroSubsystem::BroadcastWarmupMapCoverageStarts()
{
    const int32 MapCount = ActiveWarmupMapCoverage.Num();
    for (int32 MapIndex = 0; MapIndex < MapCount; ++MapIndex)
    {
        FStageZeroManifestMapCoverage& Coverage = ActiveWarmupMapCoverage[MapIndex];
        if (!Coverage.bStarted)
        {
            Coverage.bStarted = true;
            OnWarmupMapStarted.Broadcast(Coverage.MapName, MapIndex + 1, MapCount);
        }
    }

    // Empty authored map entries are legitimate logical groups. Complete them
    // only after every accepted map has received its start notification.
    for (int32 MapIndex = 0; MapIndex < MapCount; ++MapIndex)
    {
        FinishWarmupMapCoverageIfComplete(MapIndex);
    }
}

void UStageZeroSubsystem::MarkWarmupMapPathProcessed(
    const FSoftObjectPath& Path,
    EStageZeroWarmupJobTerminalState State)
{
    const bool bProcessedTerminalState = State == EStageZeroWarmupJobTerminalState::Succeeded
        || State == EStageZeroWarmupJobTerminalState::Failed
        || State == EStageZeroWarmupJobTerminalState::SkippedPolicy;
    if (!bProcessedTerminalState)
    {
        // Cancellation and timeout terminate the run, not the authored work.
        // Reporting those pending memberships as finished would be misleading.
        return;
    }

    const TArray<int32>* MapIndices = ManifestPathMapCoverageMembership.Find(Path);
    if (!MapIndices)
    {
        return;
    }

    for (const int32 MapIndex : *MapIndices)
    {
        if (!ActiveWarmupMapCoverage.IsValidIndex(MapIndex))
        {
            continue;
        }

        FStageZeroManifestMapCoverage& Coverage = ActiveWarmupMapCoverage[MapIndex];
        if (!Coverage.RequiredPaths.Contains(Path) || Coverage.ProcessedTerminalPaths.Contains(Path))
        {
            continue;
        }

        Coverage.ProcessedTerminalPaths.Add(Path);
        FinishWarmupMapCoverageIfComplete(MapIndex);
    }
}

void UStageZeroSubsystem::FinishWarmupMapCoverageIfComplete(int32 MapIndex)
{
    if (!ActiveWarmupMapCoverage.IsValidIndex(MapIndex))
    {
        return;
    }

    FStageZeroManifestMapCoverage& Coverage = ActiveWarmupMapCoverage[MapIndex];
    if (!Coverage.bStarted
        || Coverage.bFinished
        || Coverage.ProcessedTerminalPaths.Num() < Coverage.RequiredPaths.Num())
    {
        return;
    }

    Coverage.bFinished = true;
    CompletedMaps = FMath::Clamp(CompletedMaps + 1, 0, ActiveWarmupMapCoverage.Num());
    OnWarmupMapFinished.Broadcast(Coverage.MapName, MapIndex + 1, ActiveWarmupMapCoverage.Num());
}

void UStageZeroSubsystem::TrackWarmupJobRequested(const FSoftObjectPath& Path, const FString& AssetType, bool bInManifest)
{
    FStageZeroWarmupJobInfo& Info = WarmupDebugJobs.FindOrAdd(Path);
    Info.AssetName = FName(*Path.GetAssetName());
    Info.AssetPath = Path;
    if (!AssetType.IsEmpty())
    {
        Info.AssetType = AssetType;
    }
    Info.bInManifest = Info.bInManifest || bInManifest;
    Info.bWarmupRequested = true;
    Info.bWarmupFinished = false;
    Info.TerminalState = EStageZeroWarmupJobTerminalState::Pending;
    Info.TerminalReason.Reset();
}

void UStageZeroSubsystem::TrackWarmupJobLoading(const FSoftObjectPath& Path)
{
    FStageZeroWarmupJobInfo& Info = WarmupDebugJobs.FindOrAdd(Path);
    if (Info.AssetName.IsNone())
    {
        Info.AssetName = FName(*Path.GetAssetName());
    }
    Info.AssetPath = Path;
    Info.bWarmupRequested = true;
    Info.TerminalState = EStageZeroWarmupJobTerminalState::Loading;
}

void UStageZeroSubsystem::TrackWarmupJobFinished(const FSoftObjectPath& Path, EStageZeroWarmupJobTerminalState State, const FString& Reason)
{
    FStageZeroWarmupJobInfo& Info = WarmupDebugJobs.FindOrAdd(Path);
    const bool bAlreadyTerminal = Info.TerminalState != EStageZeroWarmupJobTerminalState::Pending
        && Info.TerminalState != EStageZeroWarmupJobTerminalState::Loading;
    if (bAlreadyTerminal)
    {
        return;
    }

    if (Info.AssetName.IsNone())
    {
        Info.AssetName = FName(*Path.GetAssetName());
    }
    Info.AssetPath = Path;
    Info.bWarmupRequested = true;
    Info.bWarmupFinished = true;
    Info.TerminalState = State;
    Info.TerminalReason = Reason;
    MarkWarmupMapPathProcessed(Path, State);
}

void UStageZeroSubsystem::MarkOutstandingManifestJobsCancelled(const FString& Reason)
{
    for (TPair<FSoftObjectPath, FStageZeroWarmupJobInfo>& Pair : WarmupDebugJobs)
    {
        FStageZeroWarmupJobInfo& Info = Pair.Value;
        if (Info.bInManifest
            && (Info.TerminalState == EStageZeroWarmupJobTerminalState::Pending
                || Info.TerminalState == EStageZeroWarmupJobTerminalState::Loading))
        {
            Info.bWarmupFinished = true;
            Info.TerminalState = EStageZeroWarmupJobTerminalState::Cancelled;
            Info.TerminalReason = Reason;
        }
    }
    ManifestTasksCompleted = ManifestTaskTotal;
}

void UStageZeroSubsystem::MarkOutstandingManifestJobsTimedOut(const FString& Reason)
{
    for (TPair<FSoftObjectPath, FStageZeroWarmupJobInfo>& Pair : WarmupDebugJobs)
    {
        FStageZeroWarmupJobInfo& Info = Pair.Value;
        if (!Info.bInManifest
            || (Info.TerminalState != EStageZeroWarmupJobTerminalState::Pending
                && Info.TerminalState != EStageZeroWarmupJobTerminalState::Loading))
        {
            continue;
        }

        Info.bWarmupFinished = true;
        Info.TerminalState = EStageZeroWarmupJobTerminalState::TimedOut;
        Info.TerminalReason = Reason;
        ++ManifestFailedLoads;

        const FString Failure = FString::Printf(
            TEXT("%s [%s]: %s"),
            *Pair.Key.ToString(),
            Info.AssetType.IsEmpty() ? TEXT("ManifestAsset") : *Info.AssetType,
            *Reason);
        if (!FailedWarmupAssetPaths.Contains(Pair.Key))
        {
            FailedWarmupAssetPaths.Add(Pair.Key);
            FailedWarmupAssets.Add(Failure);
        }
    }

    ManifestTasksCompleted = ManifestTaskTotal;
    UpdateProgressBroadcast(ManifestTasksCompleted, CurrentMapTaskTotal);
}

bool UStageZeroSubsystem::AreAllManifestJobsTerminal() const
{
    for (const TPair<FSoftObjectPath, FStageZeroWarmupJobInfo>& Pair : WarmupDebugJobs)
    {
        const FStageZeroWarmupJobInfo& Info = Pair.Value;
        if (Info.bInManifest
            && (Info.TerminalState == EStageZeroWarmupJobTerminalState::Pending
                || Info.TerminalState == EStageZeroWarmupJobTerminalState::Loading))
        {
            return false;
        }
    }
    return ManifestTasksCompleted >= ManifestTaskTotal;
}

bool UStageZeroSubsystem::DidAllManifestJobsSucceed() const
{
    for (const TPair<FSoftObjectPath, FStageZeroWarmupJobInfo>& Pair : WarmupDebugJobs)
    {
        const FStageZeroWarmupJobInfo& Info = Pair.Value;
        if (Info.bInManifest && Info.TerminalState != EStageZeroWarmupJobTerminalState::Succeeded)
        {
            return false;
        }
    }
    return AreAllManifestJobsTerminal();
}

void UStageZeroSubsystem::FinishWarmupInternal(bool bCancelled)
{
    FinishWarmupInternal(bCancelled, bCancelled ? EStageZeroWarmupResult::Cancelled : EStageZeroWarmupResult::Succeeded);
}

void UStageZeroSubsystem::FinishWarmupAfterMinimumDisplay(bool bCancelled, EStageZeroWarmupResult Result, FString Reason)
{
    bWarmupFinishPending = false;
    FinishWarmupInternal(bCancelled, Result, Reason);
}

void UStageZeroSubsystem::FinishWarmupInternal(bool bCancelled, EStageZeroWarmupResult Result, const FString& Reason)
{
    if (bWarmupFinishPending)
    {
        return;
    }
    if (!bWarmupRunning && WarmupResult != EStageZeroWarmupResult::Running && WarmupResult != EStageZeroWarmupResult::NotStarted)
    {
        return;
    }

    const int32 PipelineRemaining = GetRecordedPipelineCacheWorkRemaining();
    const int32 MapCompletedRaw = ManifestTasksCompleted;
    const int32 MapTotalForFinish = FMath::Max(CurrentMapTaskTotal, 1);
    const int32 MapCompleted = FMath::Clamp(MapCompletedRaw, 0, MapTotalForFinish);
    const FName MapNameForFinish = CurrentMapName;
    EStageZeroWarmupResult FinalResult = bCancelled ? EStageZeroWarmupResult::Cancelled : Result;
    FString FinalReason = Reason;

    if (FinalResult == EStageZeroWarmupResult::Succeeded && IsStrictCompletionEnabled())
    {
        if (!AreAllManifestJobsTerminal())
        {
            FinalResult = EStageZeroWarmupResult::Failed;
            FinalReason = TEXT("Strict completion blocked because one or more required manifest items never reached a terminal state.");
        }
        else if (!DidAllManifestJobsSucceed() && ShouldTreatAssetFailuresAsFatal())
        {
            FinalResult = EStageZeroWarmupResult::Failed;
            FinalReason = TEXT("Strict completion blocked because at least one required manifest item did not succeed.");
        }
        else if (PipelineRemaining > 0)
        {
            bPipelineTimedOut = true;
            const bool bContinueAsPartial = ShouldContinueOnPipelineCacheTimeout();
            FinalResult = bContinueAsPartial
                ? EStageZeroWarmupResult::Partial
                : EStageZeroWarmupResult::TimedOut;
            FinalReason = bContinueAsPartial
                ? FString::Printf(TEXT("Pipeline cache timed out with %d remaining work unit(s); continuing with explicitly partial PSO coverage."), PipelineRemaining)
                : FString::Printf(TEXT("Strict completion blocked by %d remaining pipeline cache work unit(s)."), PipelineRemaining);
        }
        else if (!DidAllManifestJobsSucceed() || ManifestFailedLoads > 0)
        {
            FinalResult = EStageZeroWarmupResult::Partial;
            FinalReason = FString::Printf(
                TEXT("Manifest preload completed with %d failed required item(s); continuing because asset failures are configured as non-fatal."),
                ManifestFailedLoads);
        }
    }

    if (FinalResult == EStageZeroWarmupResult::Succeeded
        && bPipelineCacheStageRequested
        && !bPipelineCacheStageAccepted)
    {
        FinalResult = IsStrictCompletionEnabled()
            ? EStageZeroWarmupResult::Failed
            : EStageZeroWarmupResult::Partial;
        FinalReason = PipelineCacheStageFailureReason.IsEmpty()
            ? TEXT("Asset preload completed, but no recorded PSO cache was available; PSO coverage is not established.")
            : PipelineCacheStageFailureReason;
    }

    if (FinalReason.IsEmpty())
    {
        switch (FinalResult)
        {
            case EStageZeroWarmupResult::Succeeded:
                FinalReason = TEXT("Warmup completed successfully.");
                break;
            case EStageZeroWarmupResult::Partial:
                FinalReason = TEXT("Warmup completed with skipped or failed optional work.");
                break;
            case EStageZeroWarmupResult::TimedOut:
                FinalReason = TEXT("Warmup timed out before strict completion.");
                break;
            case EStageZeroWarmupResult::Cancelled:
                FinalReason = TEXT("Warmup was cancelled.");
                break;
            case EStageZeroWarmupResult::Failed:
                FinalReason = TEXT("Warmup failed.");
                break;
            default:
                FinalReason = TEXT("Warmup finished.");
                break;
        }
    }

    const float Elapsed = GetWorld() ? (GetWorld()->GetTimeSeconds() - WarmupStartTimeSeconds) : 0.0f;
    const bool bDelayEligible = FinalResult == EStageZeroWarmupResult::Succeeded || FinalResult == EStageZeroWarmupResult::Partial;
    const bool bNeedsDelay = bDelayEligible && MinimumDisplaySeconds > 0.0f && Elapsed < MinimumDisplaySeconds;
    if (bNeedsDelay && GetWorld())
    {
        const float Remaining = MinimumDisplaySeconds - Elapsed;
        bWarmupFinishPending = true;
        FTimerDelegate FinishDelegate = FTimerDelegate::CreateUObject(
            this,
            &UStageZeroSubsystem::FinishWarmupAfterMinimumDisplay,
            bCancelled,
            FinalResult,
            FinalReason);
        GetWorld()->GetTimerManager().SetTimer(FinishDelayTimer, FinishDelegate, Remaining, false);
        return;
    }

    if (!AreAllManifestJobsTerminal())
    {
        MarkOutstandingManifestJobsCancelled(
            FinalReason.IsEmpty() ? TEXT("Warmup ended before this manifest item completed.") : FinalReason);
    }
    ClearRuntimeWarmupCallbacks();

    WarmupResult = FinalResult;
    LastWarmupStatusMessage = FinalReason;
    bWarmupCompleted = FinalResult == EStageZeroWarmupResult::Succeeded;
    bWarmupRunning = false;
    if (TotalTasks <= 0)
    {
        TotalTasks = 1;
    }
    if (FinalResult == EStageZeroWarmupResult::Succeeded || FinalResult == EStageZeroWarmupResult::Partial)
    {
        CompletedTasks = TotalTasks;
        RawCompletedTasks = TotalTasks;
        LastBroadcastCompletedTasks = TotalTasks;
        LastBroadcastTotalTasks = TotalTasks;
        LastBroadcastProgress = 1.0f;
        WarmupProgress = 1.0f;
    }
    else
    {
        RawCompletedTasks = FMath::Clamp(TaskBaseline + MapCompleted, 0, FMath::Max(TotalTasks, 1));
        CompletedTasks = FMath::Clamp(FMath::Max(CompletedTasks, RawCompletedTasks), 0, FMath::Max(TotalTasks, 1));
        WarmupProgress = FMath::Clamp(WarmupProgress, 0.0f, 1.0f);
    }

    const bool bAutoHideLoadingWidget = FinalResult == EStageZeroWarmupResult::Succeeded || FinalResult == EStageZeroWarmupResult::Partial;
    if (bAutoHideLoadingWidget && ActiveLoadingWidget.IsValid() && ActiveLoadingWidget->bHideAfterWarmup)
    {
        bHideRegisteredWidgetOnComplete = true;
    }

    const bool bTransitionEligible = FinalResult == EStageZeroWarmupResult::Succeeded || FinalResult == EStageZeroWarmupResult::Partial;
    const bool bShouldOpenPrimary = bTransitionEligible && bAutoTransitionToPrimary && !PrimaryLevelOverride.IsNull();
    const FString AutoTransitionPackage = PrimaryLevelOverride.IsNull() ? FString() : GetPackageNameFromMap(PrimaryLevelOverride);
    const FString AutoTransitionTarget = AutoTransitionPackage.IsEmpty() ? FString(TEXT("<none>")) : AutoTransitionPackage;
    const FName AutoTransitionLevelName = AutoTransitionPackage.IsEmpty()
        ? NAME_None
        : FName(*AutoTransitionPackage);
    const int32 DisplayMapCompleted = bTransitionEligible
        ? MapTotalForFinish
        : FMath::Clamp(CompletedTasks - TaskBaseline, 0, MapTotalForFinish);
    OnWarmupProgress.Broadcast(WarmupProgress, MapNameForFinish, CompletedTasks, TotalTasks, DisplayMapCompleted, MapTotalForFinish);
    BroadcastWarmupStatus();
    OnWarmupFinishedDetailed.Broadcast(FinalResult, BuildWarmupStatusSnapshot());
    OnWarmupFinished.Broadcast(bCancelled);

    StopBootMusic(false);
    const bool bCanSchedulePrimaryTravel = bShouldOpenPrimary && AutoTransitionLevelName != NAME_None && GetWorld() != nullptr;
    if (bCanSchedulePrimaryTravel)
    {
        // Legacy behavior presents this message during DelayAfterComplete, so
        // dispatch when the transition is scheduled rather than after the delay.
        DispatchLegacyTransitionMessage(AutoTransitionLevelName);
    }
    const bool bKeepWidgetForLegacyTransition = bLegacyTransitionWidgetAwaitingTravelCleanup;
    if (bAutoHideLoadingWidget && !bKeepWidgetForLegacyTransition)
    {
        RemoveLoadingWidget();
    }
    UE_LOG(LogStageZeroSubsystem, Display, TEXT("[StageZero] Warmup finished. Result=%s Reason=\"%s\" Manifest=%d/%d Failed=%d RecordedPSOAccepted=%s RemainingPSO=%d AutoTransition=%s Target=%s."),
        *StaticEnum<EStageZeroWarmupResult>()->GetNameStringByValue(static_cast<int64>(FinalResult)),
        *FinalReason,
        MapCompleted,
        MapTotalForFinish,
        ManifestFailedLoads,
        bPipelineCacheStageAccepted ? TEXT("true") : TEXT("false"),
        PipelineRemaining,
        bShouldOpenPrimary ? TEXT("true") : TEXT("false"),
        *AutoTransitionTarget);
    if (!bTransitionEligible)
    {
        ReleaseRetainedWarmupAssets();
    }
    if (bCanSchedulePrimaryTravel)
    {
        if (UWorld* World = GetWorld())
        {
            const FName LevelName = AutoTransitionLevelName;
            if (LevelName != NAME_None)
            {
                if (DelayAfterComplete > 0.0f)
                {
                    FTimerDelegate TravelDelegate = FTimerDelegate::CreateUObject(
                        this,
                        &UStageZeroSubsystem::OpenLevelWithoutTravelLoadingScreen,
                        LevelName,
                        true,
                        FString(),
                        false);
                    World->GetTimerManager().SetTimer(FinishDelayTimer, TravelDelegate, DelayAfterComplete, false);
                }
                else
                {
                    OpenLevelWithoutTravelLoadingScreen(LevelName, true, FString(), false);
                }
            }
        }
    }

}

bool UStageZeroSubsystem::TransitionToPrimaryLevel(bool bUseConfiguredDelay, bool bShowDebug)
{
    if (PrimaryLevelOverride.IsNull())
    {
        if (bShowDebug && GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("StageZero: No primary level set"));
        }
        return false;
    }

    if (!GetWorld())
    {
        if (bShowDebug && GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("StageZero: Cannot transition (no world)"));
        }
        return false;
    }

    if (bWarmupRunning)
    {
        if (bShowDebug && GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, TEXT("StageZero: Warmup still running, transition blocked"));
        }
        return false;
    }

    const bool bCompletedEnoughToTransition = bWarmupCompleted || WarmupResult == EStageZeroWarmupResult::Partial;
    if (!bCompletedEnoughToTransition)
    {
        if (bShowDebug && GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, TEXT("StageZero: Warmup not completed yet"));
        }
        return false;
    }

    const FString PackageName = GetPackageNameFromMap(PrimaryLevelOverride);
    const FName LevelName = PackageName.IsEmpty() ? NAME_None : FName(*PackageName);
    if (LevelName == NAME_None)
    {
        if (bShowDebug && GEngine)
        {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("StageZero: Invalid primary level"));
        }
        return false;
    }

    if (bShowDebug && GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, FString::Printf(TEXT("StageZero: Transitioning to %s"), *LevelName.ToString()));
    }

    DispatchLegacyTransitionMessage(LevelName);
    if (bUseConfiguredDelay && DelayAfterComplete > 0.0f)
    {
        GetWorld()->GetTimerManager().ClearTimer(FinishDelayTimer);
        FTimerDelegate TravelDelegate = FTimerDelegate::CreateUObject(
            this,
            &UStageZeroSubsystem::OpenLevelWithoutTravelLoadingScreen,
            LevelName,
            true,
            FString(),
            false);
        GetWorld()->GetTimerManager().SetTimer(FinishDelayTimer, TravelDelegate, DelayAfterComplete, false);
    }
    else
    {
        OpenLevelWithoutTravelLoadingScreen(LevelName, true, FString(), bShowDebug);
    }

    return true;
}

bool UStageZeroSubsystem::OpenLevelWithLoadingScreen(FName LevelName, bool bAbsolute, FString Options, bool bShowDebug)
{
    if (LevelName == NAME_None)
    {
        StageZeroDebugMessage(this, bShowDebug, TEXT("StageZero: OpenLevelWithLoadingScreen failed (invalid level name)."), FColor::Red);
        return false;
    }

    if (bPendingTravelOpenLevel)
    {
        UE_LOG(LogStageZeroSubsystem, Warning, TEXT("StageZero: OpenLevelWithLoadingScreen ignored duplicate request for %s while %s is already pending."),
            *LevelName.ToString(),
            *PendingTravelOpenLevelName.ToString());
        StageZeroDebugMessage(this, bShowDebug, TEXT("StageZero: Travel is already pending; duplicate request ignored."), FColor::Yellow);
        return true;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        StageZeroDebugMessage(this, bShowDebug, TEXT("StageZero: OpenLevelWithLoadingScreen failed (no world)."), FColor::Red);
        return false;
    }

    if (DoesWorldMatchRequestedMap(World, LevelName.ToString()))
    {
        UE_LOG(LogStageZeroSubsystem, Display, TEXT("StageZero: OpenLevelWithLoadingScreen skipped because %s is already the active world."),
            *LevelName.ToString());
        StageZeroDebugMessage(this, bShowDebug, FString::Printf(TEXT("StageZero: %s is already loaded."), *LevelName.ToString()));
        return true;
    }

    if (ShouldSuppressTravelLoadingScreenForDestination(LevelName.ToString(), true))
    {
        UE_LOG(LogStageZeroSubsystem, Log, TEXT("StageZero: Opening %s without travel loading screen because the destination is excluded from travel screens."),
            *LevelName.ToString());
        UGameplayStatics::OpenLevel(World, LevelName, bAbsolute, Options);
        return true;
    }

    const bool bPreparedLoadingScreen = SetupTravelLoadingScreen(LevelName.ToString(), true);
    if (!bPreparedLoadingScreen)
    {
        UE_LOG(LogStageZeroSubsystem, Warning, TEXT("StageZero: OpenLevelWithLoadingScreen could not start MoviePlayer for %s. Stage Zero will use the viewport travel cover fallback."),
            *LevelName.ToString());
        StageZeroDebugMessage(this, bShowDebug, TEXT("StageZero: MoviePlayer unavailable; using viewport travel cover fallback."), FColor::Yellow);
    }

    const bool bAddedPreTravelCover = AddTravelViewportCover(LevelName.ToString(), false, World);
    if (bAddedPreTravelCover)
    {
        UE_LOG(LogStageZeroSubsystem, Log, TEXT("StageZero: Added pre-travel viewport cover for %s before deferred OpenLevel."),
            *LevelName.ToString());
    }

    PendingTravelOpenLevelName = LevelName;
    PendingTravelOpenLevelOptions = Options;
    bPendingTravelOpenLevelAbsolute = bAbsolute;
    bPendingTravelOpenLevelDebug = bShowDebug;
    bPendingTravelOpenLevel = true;
    if (PendingTravelOpenLevelTickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(PendingTravelOpenLevelTickerHandle);
        PendingTravelOpenLevelTickerHandle.Reset();
    }
    PendingTravelOpenLevelTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateUObject(this, &UStageZeroSubsystem::TickPendingTravelOpenLevel),
        0.0f);
    StageZeroDebugMessage(this, bShowDebug, FString::Printf(TEXT("StageZero: Preparing travel to %s with Stage Zero loading screen."), *LevelName.ToString()));
    return true;
}

bool UStageZeroSubsystem::ShouldShowWarmupLoadingScreen() const
{
    // Boot/warmup UI is controlled by BuiltInLoadingWidgetClass and boot-screen settings.
    // Loading Screen settings are reserved for between-level map travel.
    return true;
}

bool UStageZeroSubsystem::ShouldSuppressTravelLoadingScreenForDestination(const FString& DestinationMapName, bool bExplicitRequest) const
{
    const UStageZeroProjectSettings* Settings = GetSettings();
    if (DestinationMapName.IsEmpty())
    {
        return false;
    }

    if (IsBuiltInStageZeroBootMap(DestinationMapName))
    {
        return true;
    }

    if (Settings)
    {
        for (const TSoftObjectPtr<UWorld>& ExcludedMap : Settings->TravelLoadingScreenExcludedMaps)
        {
            const FString ExcludedPackageName = GetPackageNameFromMap(ExcludedMap);
            if (!ExcludedPackageName.IsEmpty() && DoesMapPackageMatch(ExcludedPackageName, DestinationMapName))
            {
                return true;
            }
        }

        const bool bWarmupHasNotStarted = !bAutoStartupAttempted
            && !bWarmupRunning
            && !bWarmupCompleted
            && WarmupResult == EStageZeroWarmupResult::NotStarted;
        if (!bExplicitRequest
            && Settings->StartupMode != EStageZeroStartupMode::Manual
            && bWarmupHasNotStarted)
        {
            return true;
        }
    }

    return false;
}

bool UStageZeroSubsystem::ShouldShowTravelLoadingScreenForWorld(const UWorld* World, bool bExplicitRequest, const FString& DestinationMapName) const
{
    const UStageZeroProjectSettings* Settings = GetSettings();
    if (!Settings)
    {
        return false;
    }

    if (ShouldSuppressTravelLoadingScreenForDestination(DestinationMapName, bExplicitRequest))
    {
        return false;
    }

    if (!bExplicitRequest
        && Settings->LoadingScreenMode != EStageZeroLoadingScreenMode::MapTravelOnly)
    {
        return false;
    }

    if (IsRunningCommandlet() || IsRunningDedicatedServer())
    {
        return false;
    }

    if (!bExplicitRequest && (!World || !World->IsGameWorld()))
    {
        return false;
    }

    if (World)
    {
        if (World->GetNetMode() == NM_DedicatedServer)
        {
            return false;
        }

    }

    return true;
}

void UStageZeroSubsystem::HandlePreLoadMapWithContext(const FWorldContext& WorldContext, const FString& MapName)
{
    // The legacy message is allowed to remain visible during the configured
    // transition delay, but Stage Zero releases its widget ownership/tracking
    // before map load begins.
    ClearLegacyTransitionPresentation();

    if (bSuppressNextAutomaticTravelLoadingScreen)
    {
        const FString SuppressedPackage = NormalizeMapPackageForComparison(SuppressedTravelMapName);
        const FString IncomingPackage = NormalizeMapPackageForComparison(MapName);
        const bool bSuppressThisTravel = SuppressedPackage.IsEmpty()
            || IncomingPackage.IsEmpty()
            || SuppressedPackage.Equals(IncomingPackage, ESearchCase::IgnoreCase)
            || FPackageName::GetShortName(SuppressedPackage).Equals(FPackageName::GetShortName(IncomingPackage), ESearchCase::IgnoreCase);

        bSuppressNextAutomaticTravelLoadingScreen = false;
        SuppressedTravelMapName.Reset();

        if (bSuppressThisTravel)
        {
            UE_LOG(LogStageZeroSubsystem, Log, TEXT("StageZero: Suppressed automatic travel loading screen for primary transition to %s."),
                MapName.IsEmpty() ? TEXT("unknown map") : *MapName);
            return;
        }
    }

    if (!ShouldShowTravelLoadingScreenForWorld(WorldContext.World(), false, MapName))
    {
        return;
    }

    if (bTravelLoadingScreenActive && !PendingTravelMapName.IsEmpty())
    {
        const FString PendingPackage = NormalizeMapPackageForComparison(PendingTravelMapName);
        const FString IncomingPackage = NormalizeMapPackageForComparison(MapName);
        if (!PendingPackage.IsEmpty()
            && !IncomingPackage.IsEmpty()
            && (PendingPackage.Equals(IncomingPackage, ESearchCase::IgnoreCase)
                || FPackageName::GetShortName(PendingPackage).Equals(FPackageName::GetShortName(IncomingPackage), ESearchCase::IgnoreCase)))
        {
            UE_LOG(LogStageZeroSubsystem, Log, TEXT("StageZero: Automatic travel setup skipped for %s because an explicit Stage Zero travel screen is already active."),
                *MapName);
            return;
        }
    }

    SetupTravelLoadingScreen(MapName, false);
}

void UStageZeroSubsystem::HandlePostLoadMapWithWorld(UWorld* LoadedWorld)
{
    // Defensive fallback for travel implementations that do not emit the
    // expected pre-load callback in this process.
    ClearLegacyTransitionPresentation();

    if (bReleaseRetainedAssetsAfterNextMapLoad || bStageZeroInitiatedTravel)
    {
        ReleaseRetainedWarmupAssets();
    }

    if (!bTravelLoadingScreenActive)
    {
        return;
    }

    StartPostLoadTravelCover(LoadedWorld);
}

void UStageZeroSubsystem::HandleTravelMoviePlaybackStarted()
{
    bTravelMoviePlayerStarted = true;
}

int32 UStageZeroSubsystem::ResolveTravelLoadingScreenProfileIndex(const FString& MapName) const
{
    const UStageZeroProjectSettings* Settings = GetSettings();
    if (!Settings)
    {
        return INDEX_NONE;
    }

    int32 BestIndex = INDEX_NONE;
    int32 BestScore = 0;
    for (int32 Index = 0; Index < Settings->TravelLoadingScreenProfiles.Num(); ++Index)
    {
        const FStageZeroLoadingScreenProfile& Profile = Settings->TravelLoadingScreenProfiles[Index];
        if (!DoesLoadingScreenPlatformProfileMatch(Profile.Platform))
        {
            continue;
        }

        const bool bHasMap = Profile.DestinationMap.ToSoftObjectPath().IsValid();
        const bool bMapMatches = bHasMap && DoesLoadingScreenProfileMapMatch(Profile, MapName);
        if (bHasMap && !bMapMatches)
        {
            continue;
        }

        int32 Score = bMapMatches ? 100 : 10;
        if (Profile.Platform != EStageZeroLoadingScreenPlatformProfile::Any)
        {
            Score += 5;
        }

        if (Score > BestScore)
        {
            BestScore = Score;
            BestIndex = Index;
        }
    }

    return BestIndex;
}

const FStageZeroLoadingScreenProfile* UStageZeroSubsystem::GetActiveTravelLoadingScreenProfile() const
{
    const UStageZeroProjectSettings* Settings = GetSettings();
    if (!Settings || !Settings->TravelLoadingScreenProfiles.IsValidIndex(ActiveTravelLoadingScreenProfileIndex))
    {
        return nullptr;
    }

    return &Settings->TravelLoadingScreenProfiles[ActiveTravelLoadingScreenProfileIndex];
}

bool UStageZeroSubsystem::HasPostLoadTravelDismissInput(UWorld* World) const
{
    if (FSlateApplication::IsInitialized() && FSlateApplication::Get().GetPressedMouseButtons().Num() > 0)
    {
        return true;
    }

    APlayerController* PC = World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
    if (!PC)
    {
        return false;
    }

    if (PC->WasInputKeyJustPressed(EKeys::AnyKey))
    {
        return true;
    }

    static const FKey DismissKeys[] =
    {
        EKeys::SpaceBar,
        EKeys::Enter,
#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7)
        EKeys::Virtual_Gamepad_Accept.GetVirtualKey(),
#else
        EKeys::Virtual_Accept,
#endif
        EKeys::LeftMouseButton,
        EKeys::RightMouseButton,
        EKeys::Gamepad_FaceButton_Bottom,
        EKeys::Gamepad_Special_Right
    };

    for (const FKey& Key : DismissKeys)
    {
        if (PC->WasInputKeyJustPressed(Key))
        {
            return true;
        }
    }

    return false;
}

TArray<FString> UStageZeroSubsystem::BuildNormalizedTravelMoviePaths() const
{
    TArray<FString> Result;
    const UStageZeroProjectSettings* Settings = GetSettings();
    if (!Settings)
    {
        return Result;
    }

    const FStageZeroLoadingScreenProfile* ActiveProfile = GetActiveTravelLoadingScreenProfile();
    const TArray<FString>& SourceMoviePaths = (ActiveProfile && ActiveProfile->MoviePaths.Num() > 0)
        ? ActiveProfile->MoviePaths
        : Settings->TravelMoviePaths;

    for (FString MoviePath : SourceMoviePaths)
    {
        MoviePath.TrimStartAndEndInline();
        MoviePath.ReplaceInline(TEXT("\\"), TEXT("/"));
        if (MoviePath.IsEmpty())
        {
            continue;
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

        const FString Directory = FPaths::GetPath(MoviePath);
        const FString BaseName = FPaths::GetBaseFilename(MoviePath);
        MoviePath = Directory.IsEmpty() ? BaseName : (Directory / BaseName);

        if (!MoviePath.IsEmpty())
        {
            Result.AddUnique(MoviePath);
        }
    }

    return Result;
}

TArray<UTexture2D*> UStageZeroSubsystem::LoadTravelBackgroundTextures(bool bHasMovies)
{
    TArray<UTexture2D*> Result;
    TravelLoadingBackgroundTextures.Reset();

    const UStageZeroProjectSettings* Settings = GetSettings();
    if (!Settings)
    {
        return Result;
    }

    const bool bCanShowImage = !bHasMovies || Settings->bShowTravelImageWhenMoviesPlay;
    if (!bCanShowImage)
    {
        return Result;
    }

    const FStageZeroLoadingScreenProfile* ActiveProfile = GetActiveTravelLoadingScreenProfile();
    const bool bUseProfileImages = ActiveProfile && ActiveProfile->BackgroundImages.Num() > 0;
    const EStageZeroTravelImageSelection ImageSelection = bUseProfileImages
        ? ActiveProfile->ImageSelection
        : Settings->TravelImageSelection;
    const float ImageIntervalSeconds = Settings->bReduceLoadingScreenMotion
        ? 0.0f
        : (bUseProfileImages ? ActiveProfile->ImageIntervalSeconds : Settings->TravelImageIntervalSeconds);

    TArray<TSoftObjectPtr<UTexture2D>> CandidateImages;
    if (bUseProfileImages)
    {
        CandidateImages = ActiveProfile->BackgroundImages;
    }
    else if (Settings->TravelBackgroundImages.Num() > 0)
    {
        CandidateImages = Settings->TravelBackgroundImages;
    }
    else if (Settings->bUseBootSlideAsTravelImageFallback)
    {
        CandidateImages = Settings->BackgroundSlides;
    }

    CandidateImages.RemoveAll([](const TSoftObjectPtr<UTexture2D>& Image)
    {
        return Image.IsNull();
    });

    if (CandidateImages.Num() == 0)
    {
        return Result;
    }

    TArray<TSoftObjectPtr<UTexture2D>> SelectedImages;
    switch (ImageSelection)
    {
        case EStageZeroTravelImageSelection::Random:
        {
            SelectedImages.Add(CandidateImages[FMath::RandRange(0, CandidateImages.Num() - 1)]);
            break;
        }
        case EStageZeroTravelImageSelection::Sequential:
        {
            const int32 StartIndex = CandidateImages.Num() > 0
                ? FMath::Abs(TravelBackgroundSelectionCounter) % CandidateImages.Num()
                : 0;

            if (ImageIntervalSeconds > 0.0f && CandidateImages.Num() > 1)
            {
                for (int32 Offset = 0; Offset < CandidateImages.Num(); ++Offset)
                {
                    SelectedImages.Add(CandidateImages[(StartIndex + Offset) % CandidateImages.Num()]);
                }
            }
            else
            {
                SelectedImages.Add(CandidateImages[StartIndex]);
            }

            ++TravelBackgroundSelectionCounter;
            break;
        }
        case EStageZeroTravelImageSelection::First:
        default:
        {
            SelectedImages.Add(CandidateImages[0]);
            break;
        }
    }

    for (TSoftObjectPtr<UTexture2D>& ImageRef : SelectedImages)
    {
        if (UTexture2D* Texture = ImageRef.LoadSynchronous())
        {
            TravelLoadingBackgroundTextures.Add(Texture);
            Result.Add(Texture);
        }
    }

    return Result;
}

TSharedRef<SWidget> UStageZeroSubsystem::BuildTravelLoadingScreenWidget(const FString& MapName, bool bHasMovies, bool bIsPostLoadCover)
{
    const UStageZeroProjectSettings* Settings = GetSettings();
    const FText FallbackTitle = NSLOCTEXT("StageZeroSubsystem", "TravelScreenTitleFallback", "Stage Zero");
    const FText FallbackSubtitle = NSLOCTEXT("StageZeroSubsystem", "TravelScreenSubtitleFallback", "Preparing the next level");
    const FText FallbackStatus = NSLOCTEXT("StageZeroSubsystem", "TravelScreenStatusFallback", "Loading");
    const FStageZeroLoadingScreenProfile* ActiveProfile = GetActiveTravelLoadingScreenProfile();
    const EStageZeroTravelLoadingScreenTemplate LoadingScreenTemplate = ActiveProfile && ActiveProfile->bOverrideLoadingScreenTemplate
        ? ActiveProfile->LoadingScreenTemplate
        : (Settings ? Settings->TravelLoadingScreenTemplate : EStageZeroTravelLoadingScreenTemplate::Classic);

    if (Settings && !Settings->NativeLoadingScreenProviderClass.IsNull())
    {
        UClass* ProviderClass = Settings->NativeLoadingScreenProviderClass.LoadSynchronous();
        if (ProviderClass)
        {
            ActiveNativeLoadingScreenProvider = NewObject<UStageZeroNativeLoadingScreenProvider>(this, ProviderClass);
            if (ActiveNativeLoadingScreenProvider)
            {
                FStageZeroNativeLoadingScreenContext Context;
                Context.MapName = MapName;
                Context.bHasMovies = bHasMovies;
                Context.bIsPostLoadCover = bIsPostLoadCover;
                Context.Settings = Settings;

                TSharedPtr<SWidget> CustomWidget = ActiveNativeLoadingScreenProvider->BuildLoadingScreenWidget(Context);
                if (CustomWidget.IsValid())
                {
                    return CustomWidget.ToSharedRef();
                }
            }
        }

        UE_LOG(LogStageZeroSubsystem, Warning, TEXT("StageZero: Native loading screen provider did not return a valid Slate widget; using built-in travel loading screen."));
        ActiveNativeLoadingScreenProvider = nullptr;
    }

    const TArray<UTexture2D*> BackgroundTextures = LoadTravelBackgroundTextures(bHasMovies);
    const bool bAllowMovieBehindWidget = bHasMovies
        && BackgroundTextures.Num() == 0
        && Settings
        && !Settings->bShowTravelImageWhenMoviesPlay;

    const FText Title = ActiveProfile && !ActiveProfile->TitleText.IsEmpty()
        ? ActiveProfile->TitleText
        : (Settings && !Settings->TravelTitleText.IsEmpty() ? Settings->TravelTitleText : FallbackTitle);
    const FText Subtitle = ActiveProfile && !ActiveProfile->SubtitleText.IsEmpty()
        ? ActiveProfile->SubtitleText
        : (Settings && !Settings->TravelSubtitleText.IsEmpty() ? Settings->TravelSubtitleText : FallbackSubtitle);
    FText Status = ActiveProfile && !ActiveProfile->StatusText.IsEmpty()
        ? ActiveProfile->StatusText
        : (Settings && !Settings->TravelStatusText.IsEmpty() ? Settings->TravelStatusText : FallbackStatus);
    if (!MapName.IsEmpty() && (!Settings || Settings->bShowTravelDestinationLevelName))
    {
        FFormatNamedArguments Args;
        Args.Add(TEXT("Status"), Status);
        Args.Add(TEXT("Map"), FText::FromString(FPackageName::GetShortName(MapName)));
        Status = FText::Format(NSLOCTEXT("StageZeroSubsystem", "TravelScreenStatusWithMap", "{Status} - {Map}"), Args);
    }

    FText Footer = ActiveProfile && !ActiveProfile->FooterText.IsEmpty()
        ? ActiveProfile->FooterText
        : (Settings ? Settings->TravelFooterText : FText::GetEmpty());
    if (bIsPostLoadCover && Settings && Settings->TravelPostLoadDismissMode == EStageZeroPostLoadDismissMode::PressAnyKey)
    {
        const FText Prompt = !Settings->TravelPressAnyKeyText.IsEmpty()
            ? Settings->TravelPressAnyKeyText
            : NSLOCTEXT("StageZeroSubsystem", "TravelPressAnyKeyFallback", "Press any key to continue");
        if (Footer.IsEmpty())
        {
            Footer = Prompt;
        }
        else
        {
            FFormatNamedArguments Args;
            Args.Add(TEXT("Footer"), Footer);
            Args.Add(TEXT("Prompt"), Prompt);
            Footer = FText::Format(NSLOCTEXT("StageZeroSubsystem", "TravelFooterWithPressAnyKey", "{Footer}\n{Prompt}"), Args);
        }
    }

    TArray<FText> TipTexts = ActiveProfile && ActiveProfile->Tips.Num() > 0
        ? ActiveProfile->Tips
        : (Settings ? Settings->TravelTips : TArray<FText>());
    TipTexts.RemoveAll([](const FText& Tip)
    {
        return Tip.IsEmpty();
    });

    FLinearColor BackgroundColor = Settings ? Settings->TravelBackgroundColor : FLinearColor::Black;
    FLinearColor OverlayTint = Settings ? Settings->TravelOverlayTint : FLinearColor(0.0f, 0.0f, 0.0f, 0.58f);
    FLinearColor PrimaryTextColor = Settings ? Settings->TravelPrimaryTextColor : FLinearColor::White;
    FLinearColor SecondaryTextColor = Settings ? Settings->TravelSecondaryTextColor : FLinearColor(0.70f, 0.72f, 0.78f, 1.0f);
    FLinearColor AccentColor = Settings ? Settings->TravelAccentColor : FLinearColor(0.16f, 0.58f, 1.0f, 1.0f);
    int32 BaseTitleFontSize = Settings ? Settings->TravelTitleFontSize : 34;
    int32 BaseSubtitleFontSize = Settings ? Settings->TravelSubtitleFontSize : 18;
    int32 BaseSmallTextFontSize = Settings ? Settings->TravelSmallTextFontSize : 13;

    if (Settings && LoadingScreenTemplate != EStageZeroTravelLoadingScreenTemplate::Classic)
    {
        const UStageZeroProjectSettings* DefaultSettings = GetDefault<UStageZeroProjectSettings>();
        auto IsDefaultColor = [](const FLinearColor& Value, const FLinearColor& DefaultValue)
        {
            return Value.Equals(DefaultValue, 0.001f);
        };

        FLinearColor TemplateBackgroundColor = BackgroundColor;
        FLinearColor TemplateOverlayTint = OverlayTint;
        FLinearColor TemplatePrimaryTextColor = PrimaryTextColor;
        FLinearColor TemplateSecondaryTextColor = SecondaryTextColor;
        FLinearColor TemplateAccentColor = AccentColor;
        int32 TemplateTitleFontSize = BaseTitleFontSize;
        int32 TemplateSubtitleFontSize = BaseSubtitleFontSize;
        int32 TemplateSmallTextFontSize = BaseSmallTextFontSize;

        switch (LoadingScreenTemplate)
        {
            case EStageZeroTravelLoadingScreenTemplate::CinematicWide:
                TemplateBackgroundColor = FLinearColor(0.010f, 0.012f, 0.016f, 1.0f);
                TemplateOverlayTint = FLinearColor(0.0f, 0.0f, 0.0f, 0.54f);
                TemplatePrimaryTextColor = FLinearColor(0.98f, 0.98f, 0.96f, 1.0f);
                TemplateSecondaryTextColor = FLinearColor(0.78f, 0.82f, 0.86f, 1.0f);
                TemplateAccentColor = FLinearColor(0.35f, 0.72f, 1.0f, 1.0f);
                TemplateTitleFontSize = 44;
                TemplateSubtitleFontSize = 20;
                TemplateSmallTextFontSize = 14;
                break;
            case EStageZeroTravelLoadingScreenTemplate::MinimalStudio:
                TemplateBackgroundColor = FLinearColor(0.018f, 0.020f, 0.024f, 1.0f);
                TemplateOverlayTint = FLinearColor(0.0f, 0.0f, 0.0f, 0.68f);
                TemplatePrimaryTextColor = FLinearColor(0.96f, 0.97f, 0.98f, 1.0f);
                TemplateSecondaryTextColor = FLinearColor(0.66f, 0.70f, 0.75f, 1.0f);
                TemplateAccentColor = FLinearColor(0.86f, 0.90f, 0.94f, 1.0f);
                TemplateTitleFontSize = 32;
                TemplateSubtitleFontSize = 17;
                TemplateSmallTextFontSize = 13;
                break;
            case EStageZeroTravelLoadingScreenTemplate::GameTipCarousel:
                TemplateBackgroundColor = FLinearColor(0.014f, 0.018f, 0.020f, 1.0f);
                TemplateOverlayTint = FLinearColor(0.0f, 0.0f, 0.0f, 0.62f);
                TemplatePrimaryTextColor = FLinearColor(0.96f, 0.98f, 0.94f, 1.0f);
                TemplateSecondaryTextColor = FLinearColor(0.76f, 0.83f, 0.80f, 1.0f);
                TemplateAccentColor = FLinearColor(0.26f, 0.92f, 0.72f, 1.0f);
                TemplateTitleFontSize = 34;
                TemplateSubtitleFontSize = 20;
                TemplateSmallTextFontSize = 14;
                break;
            case EStageZeroTravelLoadingScreenTemplate::MapBriefing:
                TemplateBackgroundColor = FLinearColor(0.018f, 0.018f, 0.020f, 1.0f);
                TemplateOverlayTint = FLinearColor(0.0f, 0.0f, 0.0f, 0.66f);
                TemplatePrimaryTextColor = FLinearColor(0.98f, 0.96f, 0.90f, 1.0f);
                TemplateSecondaryTextColor = FLinearColor(0.78f, 0.76f, 0.70f, 1.0f);
                TemplateAccentColor = FLinearColor(1.0f, 0.68f, 0.32f, 1.0f);
                TemplateTitleFontSize = 42;
                TemplateSubtitleFontSize = 20;
                TemplateSmallTextFontSize = 14;
                break;
            case EStageZeroTravelLoadingScreenTemplate::VideoSplash:
                TemplateBackgroundColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
                TemplateOverlayTint = FLinearColor(0.0f, 0.0f, 0.0f, 0.38f);
                TemplatePrimaryTextColor = FLinearColor(0.98f, 0.99f, 1.0f, 1.0f);
                TemplateSecondaryTextColor = FLinearColor(0.78f, 0.82f, 0.88f, 1.0f);
                TemplateAccentColor = FLinearColor(0.40f, 0.76f, 1.0f, 1.0f);
                TemplateTitleFontSize = 28;
                TemplateSubtitleFontSize = 17;
                TemplateSmallTextFontSize = 13;
                break;
            case EStageZeroTravelLoadingScreenTemplate::ConsoleSafe:
                TemplateBackgroundColor = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
                TemplateOverlayTint = FLinearColor(0.0f, 0.0f, 0.0f, 0.86f);
                TemplatePrimaryTextColor = FLinearColor::White;
                TemplateSecondaryTextColor = FLinearColor(0.94f, 0.96f, 1.0f, 1.0f);
                TemplateAccentColor = FLinearColor(0.42f, 0.86f, 1.0f, 1.0f);
                TemplateTitleFontSize = 46;
                TemplateSubtitleFontSize = 24;
                TemplateSmallTextFontSize = 18;
                break;
            case EStageZeroTravelLoadingScreenTemplate::Classic:
            default:
                break;
        }

        if (IsDefaultColor(Settings->TravelBackgroundColor, DefaultSettings->TravelBackgroundColor))
        {
            BackgroundColor = TemplateBackgroundColor;
        }
        if (IsDefaultColor(Settings->TravelOverlayTint, DefaultSettings->TravelOverlayTint))
        {
            OverlayTint = TemplateOverlayTint;
        }
        if (IsDefaultColor(Settings->TravelPrimaryTextColor, DefaultSettings->TravelPrimaryTextColor))
        {
            PrimaryTextColor = TemplatePrimaryTextColor;
        }
        if (IsDefaultColor(Settings->TravelSecondaryTextColor, DefaultSettings->TravelSecondaryTextColor))
        {
            SecondaryTextColor = TemplateSecondaryTextColor;
        }
        if (IsDefaultColor(Settings->TravelAccentColor, DefaultSettings->TravelAccentColor))
        {
            AccentColor = TemplateAccentColor;
        }
        if (Settings->TravelTitleFontSize == DefaultSettings->TravelTitleFontSize)
        {
            BaseTitleFontSize = TemplateTitleFontSize;
        }
        if (Settings->TravelSubtitleFontSize == DefaultSettings->TravelSubtitleFontSize)
        {
            BaseSubtitleFontSize = TemplateSubtitleFontSize;
        }
        if (Settings->TravelSmallTextFontSize == DefaultSettings->TravelSmallTextFontSize)
        {
            BaseSmallTextFontSize = TemplateSmallTextFontSize;
        }
    }

    if (Settings && Settings->bUseAccessibleHighContrastText)
    {
        OverlayTint = FLinearColor(0.0f, 0.0f, 0.0f, FMath::Max(OverlayTint.A, 0.78f));
        PrimaryTextColor = FLinearColor::White;
        SecondaryTextColor = FLinearColor(0.92f, 0.94f, 0.97f, 1.0f);
        AccentColor = FLinearColor::White;
    }

    const bool bUseProfileImages = ActiveProfile && ActiveProfile->BackgroundImages.Num() > 0;
    const EStageZeroTravelImageSizeMode ImageSizeMode = bUseProfileImages
        ? ActiveProfile->ImageSizeMode
        : (Settings ? Settings->TravelImageSizeMode : EStageZeroTravelImageSizeMode::FillCrop);
    const float ImageIntervalSeconds = Settings && Settings->bReduceLoadingScreenMotion
        ? 0.0f
        : (bUseProfileImages ? ActiveProfile->ImageIntervalSeconds : (Settings ? Settings->TravelImageIntervalSeconds : 2.5f));
    const bool bLoopImages = bUseProfileImages
        ? ActiveProfile->bLoopImages
        : (Settings ? Settings->bLoopTravelImages : true);
    const float TextScale = Settings ? FMath::Clamp(Settings->TravelAccessibilityTextScale, 0.75f, 1.5f) : 1.0f;
    const int32 TitleFontSize = FMath::RoundToInt(BaseTitleFontSize * TextScale);
    const int32 SubtitleFontSize = FMath::RoundToInt(BaseSubtitleFontSize * TextScale);
    const int32 SmallTextFontSize = FMath::RoundToInt(BaseSmallTextFontSize * TextScale);
    const bool bReduceMotion = Settings ? Settings->bReduceLoadingScreenMotion : false;
    const float TipIntervalSeconds = Settings && Settings->bReduceLoadingScreenMotion
        ? 0.0f
        : (Settings ? Settings->TravelTipIntervalSeconds : 4.0f);

    return SNew(SStageZeroTravelLoadingScreen)
        .BackgroundTextures(BackgroundTextures)
        .TitleText(Title)
        .SubtitleText(Subtitle)
        .StatusText(Status)
        .FooterText(Footer)
        .BackgroundColor(BackgroundColor)
        .OverlayTint(OverlayTint)
        .PrimaryTextColor(PrimaryTextColor)
        .SecondaryTextColor(SecondaryTextColor)
        .AccentColor(AccentColor)
        .LoadingScreenTemplate(LoadingScreenTemplate)
        .ImageSizeMode(ImageSizeMode)
        .ImageIntervalSeconds(ImageIntervalSeconds)
        .TipTexts(TipTexts)
        .TipIntervalSeconds(TipIntervalSeconds)
        .bLoopImages(bLoopImages)
        .bAllowMovieBehindWidget(bAllowMovieBehindWidget)
        .TitleFontSize(TitleFontSize)
        .SubtitleFontSize(SubtitleFontSize)
        .SmallTextFontSize(SmallTextFontSize)
        .bShowSpinner(Settings ? Settings->bShowTravelSpinner : true)
        .FadeInSeconds(Settings ? Settings->TravelFadeInSeconds : 0.0f)
        .bReduceMotion(bReduceMotion);
}

bool UStageZeroSubsystem::AddTravelViewportCover(const FString& MapName, bool bIsPostLoadCover, UWorld* LoadedWorld)
{
    const UStageZeroProjectSettings* Settings = GetSettings();
    UGameViewportClient* ViewportClient = ResolveTravelViewportClient(LoadedWorld);
    if (!Settings || !ViewportClient)
    {
        UE_LOG(LogStageZeroSubsystem, Warning, TEXT("StageZero: Could not add %s travel cover for %s because no game viewport was available."),
            bIsPostLoadCover ? TEXT("post-load") : TEXT("pre-travel"),
            MapName.IsEmpty() ? TEXT("unknown map") : *MapName);
        return false;
    }

    bool bAddedCustomWidget = false;
    if (bIsPostLoadCover)
    {
        const FStageZeroLoadingScreenProfile* ActiveProfile = GetActiveTravelLoadingScreenProfile();
        const TSoftClassPtr<UUserWidget> PostLoadWidgetClassRef = (ActiveProfile && !ActiveProfile->PostLoadWidgetClass.IsNull())
            ? ActiveProfile->PostLoadWidgetClass
            : Settings->TravelPostLoadWidgetClass;

        if (!PostLoadWidgetClassRef.IsNull())
        {
            UClass* WidgetClass = PostLoadWidgetClassRef.LoadSynchronous();
            UWorld* WidgetWorld = LoadedWorld ? LoadedWorld : GetWorld();
            APlayerController* PC = WidgetWorld ? UGameplayStatics::GetPlayerController(WidgetWorld, 0) : nullptr;
            UUserWidget* Widget = nullptr;
            if (PC && WidgetClass)
            {
                Widget = CreateWidget<UUserWidget>(PC, WidgetClass);
            }
            else if (WidgetWorld && WidgetClass)
            {
                if (UGameInstance* GI = WidgetWorld->GetGameInstance())
                {
                    Widget = CreateWidget<UUserWidget>(GI, WidgetClass);
                }
            }

            if (Widget)
            {
                Widget->AddToViewport(10000);
                Widget->SetRenderOpacity(1.0f);
                ActivePostLoadTravelUserWidget = Widget;
                bAddedCustomWidget = true;
            }
            else
            {
                UE_LOG(LogStageZeroSubsystem, Warning, TEXT("StageZero: Could not create post-load travel widget for %s; using native cover."),
                    MapName.IsEmpty() ? TEXT("unknown map") : *MapName);
            }
        }
    }

    if (!bAddedCustomWidget)
    {
        const TArray<FString> MoviePaths = BuildNormalizedTravelMoviePaths();
        ActivePostLoadTravelCoverWidget = BuildTravelLoadingScreenWidget(MapName, MoviePaths.Num() > 0, bIsPostLoadCover);
        ViewportClient->AddViewportWidgetContent(ActivePostLoadTravelCoverWidget.ToSharedRef(), 10000);
        ActiveTravelViewportClient = ViewportClient;
        bPostLoadTravelCoverAdded = true;
    }

    UE_LOG(LogStageZeroSubsystem, Log, TEXT("StageZero: Added %s travel loading cover for %s%s."),
        bIsPostLoadCover ? TEXT("post-load") : TEXT("pre-travel"),
        MapName.IsEmpty() ? TEXT("unknown map") : *MapName,
        bAddedCustomWidget ? TEXT(" using custom UMG widget") : TEXT(" using native Slate"));
    return bAddedCustomWidget || bPostLoadTravelCoverAdded;
}

UGameViewportClient* UStageZeroSubsystem::ResolveTravelViewportClient(UWorld* PreferredWorld) const
{
    if (PreferredWorld)
    {
        if (UGameViewportClient* ViewportClient = PreferredWorld->GetGameViewport())
        {
            return ViewportClient;
        }
    }

    if (UWorld* CurrentWorld = GetWorld())
    {
        if (UGameViewportClient* ViewportClient = CurrentWorld->GetGameViewport())
        {
            return ViewportClient;
        }
    }

    if (GEngine)
    {
        for (const FWorldContext& Context : GEngine->GetWorldContexts())
        {
            UWorld* ContextWorld = Context.World();
            if (!ContextWorld)
            {
                continue;
            }

            const bool bUsableWorld =
                ContextWorld->WorldType == EWorldType::Game
                || ContextWorld->WorldType == EWorldType::PIE
                || ContextWorld->WorldType == EWorldType::GamePreview;
            if (!bUsableWorld)
            {
                continue;
            }

            if (UGameViewportClient* ViewportClient = ContextWorld->GetGameViewport())
            {
                return ViewportClient;
            }
        }

        return GEngine->GameViewport;
    }

    return nullptr;
}

float UStageZeroSubsystem::GetTravelMinimumDisplaySeconds() const
{
    if (bTravelMinimumDisplaySecondsOverrideSet)
    {
        return FMath::Max(0.0f, TravelMinimumDisplaySecondsOverride);
    }

    const UStageZeroProjectSettings* Settings = GetSettings();
    return Settings ? FMath::Max(0.0f, Settings->TravelMinimumDisplaySeconds) : 0.35f;
}

void UStageZeroSubsystem::OpenLevelWithoutTravelLoadingScreen(FName LevelName, bool bAbsolute, FString Options, bool bShowDebug)
{
    if (LevelName == NAME_None)
    {
        ClearLegacyTransitionPresentation();
        StageZeroDebugMessage(this, bShowDebug, TEXT("StageZero: OpenLevelWithoutTravelLoadingScreen failed (invalid level name)."), FColor::Red);
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        ClearLegacyTransitionPresentation();
        StageZeroDebugMessage(this, bShowDebug, TEXT("StageZero: OpenLevelWithoutTravelLoadingScreen failed (no world)."), FColor::Red);
        return;
    }

    if (DoesWorldMatchRequestedMap(World, LevelName.ToString()))
    {
        ClearLegacyTransitionPresentation();
        ReleaseRetainedWarmupAssets();
        UE_LOG(LogStageZeroSubsystem, Display, TEXT("StageZero: Primary transition skipped because %s is already the active world."),
            *LevelName.ToString());
        return;
    }

    bSuppressNextAutomaticTravelLoadingScreen = true;
    bReleaseRetainedAssetsAfterNextMapLoad = true;
    SuppressedTravelMapName = LevelName.ToString();
    UE_LOG(LogStageZeroSubsystem, Log, TEXT("StageZero: Opening primary level %s without the between-level travel loading screen."),
        *LevelName.ToString());
    DispatchLegacyTransitionMessage(LevelName);
    UGameplayStatics::OpenLevel(World, LevelName, bAbsolute, Options);
    StageZeroDebugMessage(this, bShowDebug, FString::Printf(TEXT("StageZero: Opening primary level %s without travel loading screen."), *LevelName.ToString()));
}

void UStageZeroSubsystem::ExecutePendingTravelOpenLevel()
{
    if (!bPendingTravelOpenLevel)
    {
        return;
    }

    const FName LevelName = PendingTravelOpenLevelName;
    const FString Options = PendingTravelOpenLevelOptions;
    const bool bAbsolute = bPendingTravelOpenLevelAbsolute;
    const bool bShowDebug = bPendingTravelOpenLevelDebug;

    bPendingTravelOpenLevel = false;
    PendingTravelOpenLevelName = NAME_None;
    PendingTravelOpenLevelOptions.Reset();
    bPendingTravelOpenLevelAbsolute = true;
    bPendingTravelOpenLevelDebug = false;

    UWorld* World = GetWorld();
    if (!World || LevelName == NAME_None)
    {
        UE_LOG(LogStageZeroSubsystem, Warning, TEXT("StageZero: Pending OpenLevel cancelled because the world or level was invalid."));
        RemovePostLoadTravelCover();
        return;
    }

    UE_LOG(LogStageZeroSubsystem, Log, TEXT("StageZero: Executing deferred OpenLevel for %s with travel loading screen active."),
        *LevelName.ToString());
    UGameplayStatics::OpenLevel(World, LevelName, bAbsolute, Options);
    StageZeroDebugMessage(this, bShowDebug, FString::Printf(TEXT("StageZero: Opening level %s with Stage Zero loading screen."), *LevelName.ToString()));
}

bool UStageZeroSubsystem::TickPendingTravelOpenLevel(float DeltaSeconds)
{
    (void)DeltaSeconds;

    PendingTravelOpenLevelTickerHandle.Reset();
    ExecutePendingTravelOpenLevel();
    return false;
}

bool UStageZeroSubsystem::SetupTravelLoadingScreen(const FString& MapName, bool bStageZeroInitiated)
{
    UWorld* World = GetWorld();
    if (!ShouldShowTravelLoadingScreenForWorld(World, bStageZeroInitiated, MapName))
    {
        UE_LOG(LogStageZeroSubsystem, Log, TEXT("StageZero: Travel loading screen skipped for %s because this runtime context or destination is not allowed."),
            MapName.IsEmpty() ? TEXT("unknown map") : *MapName);
        return false;
    }

    const UStageZeroProjectSettings* Settings = GetSettings();
    ActiveTravelLoadingScreenProfileIndex = ResolveTravelLoadingScreenProfileIndex(MapName);
    const FStageZeroLoadingScreenProfile* ActiveProfile = GetActiveTravelLoadingScreenProfile();
    PendingTravelMapName = MapName;
    bTravelLoadingScreenActive = true;
    bStageZeroInitiatedTravel = bStageZeroInitiatedTravel || bStageZeroInitiated;
    bTravelMoviePlayerAvailable = false;
    bTravelMoviePlayerStarted = false;

#if WITH_EDITOR
    const bool bIsPIETravelWorld = World && World->WorldType == EWorldType::PIE;
    if (bIsPIETravelWorld && Settings && !Settings->bEnableTravelLoadingScreenInPIE)
    {
        UE_LOG(LogStageZeroSubsystem, Log, TEXT("StageZero: MoviePlayer travel screen is disabled for PIE; using viewport travel cover fallback for %s."),
            MapName.IsEmpty() ? TEXT("unknown map") : *MapName);
        return false;
    }
#endif

    IGameMoviePlayer* MoviePlayer = (IsMoviePlayerEnabled() ? GetMoviePlayer() : nullptr);
    if (!MoviePlayer || !MoviePlayer->IsInitialized())
    {
        UE_LOG(LogStageZeroSubsystem, Warning, TEXT("StageZero: MoviePlayer is unavailable for %s; Stage Zero will use the viewport travel cover fallback."),
            MapName.IsEmpty() ? TEXT("unknown map") : *MapName);
        return false;
    }

    bTravelMoviePlayerAvailable = true;
    if (!TravelMoviePlaybackStartedHandle.IsValid())
    {
        TravelMoviePlaybackStartedHandle = MoviePlayer->OnMoviePlaybackStarted().AddUObject(this, &UStageZeroSubsystem::HandleTravelMoviePlaybackStarted);
    }

    const TArray<FString> MoviePaths = BuildNormalizedTravelMoviePaths();
    const bool bHasMovies = MoviePaths.Num() > 0;

    FLoadingScreenAttributes Attributes;
    Attributes.WidgetLoadingScreen = BuildTravelLoadingScreenWidget(MapName, bHasMovies);
    Attributes.MoviePaths = MoviePaths;
    Attributes.MinimumLoadingScreenDisplayTime = GetTravelMinimumDisplaySeconds();
    Attributes.bAutoCompleteWhenLoadingCompletes = true;
    Attributes.bMoviesAreSkippable = Settings ? Settings->bTravelMoviesAreSkippable : false;
    Attributes.bWaitForManualStop = false;
    Attributes.bAllowInEarlyStartup = false;
    // Do not allow MoviePlayer to re-enter GEngine::Tick during map travel.
    // UE 5.5 can assert in render-thread managers when engine ticking occurs inside the blocking loading-screen loop.
    Attributes.bAllowEngineTick = false;

    const EStageZeroTravelMoviePlayback MoviePlayback = (ActiveProfile && ActiveProfile->MoviePaths.Num() > 0)
        ? ActiveProfile->MoviePlayback
        : (Settings ? Settings->TravelMoviePlayback : EStageZeroTravelMoviePlayback::LoopLast);

    switch (MoviePlayback)
    {
        case EStageZeroTravelMoviePlayback::LoopAll:
            Attributes.PlaybackType = EMoviePlaybackType::MT_Looped;
            break;
        case EStageZeroTravelMoviePlayback::LoopLast:
            Attributes.PlaybackType = EMoviePlaybackType::MT_LoadingLoop;
            break;
        case EStageZeroTravelMoviePlayback::PlayOnce:
        default:
            Attributes.PlaybackType = EMoviePlaybackType::MT_Normal;
            break;
    }

    if (!Attributes.IsValid())
    {
        ActiveNativeLoadingScreenProvider = nullptr;
        TravelLoadingBackgroundTextures.Reset();
        UE_LOG(LogStageZeroSubsystem, Warning, TEXT("StageZero: Travel loading screen attributes were invalid for %s; Stage Zero will use the post-load travel cover only."),
            MapName.IsEmpty() ? TEXT("unknown map") : *MapName);
        return false;
    }

    MoviePlayer->SetupLoadingScreen(Attributes);
    const FString ProfileLogSuffix = ActiveProfile
        ? FString::Printf(TEXT(" using profile '%s'"), *ActiveProfile->ProfileName.ToString())
        : FString();
    UE_LOG(LogStageZeroSubsystem, Log, TEXT("StageZero: Prepared map-travel loading screen for %s%s%s. MoviePlayer will start on PreLoadMap."),
        MapName.IsEmpty() ? TEXT("unknown map") : *MapName,
        bStageZeroInitiated ? TEXT(" (Stage Zero initiated)") : TEXT(""),
        *ProfileLogSuffix);
    return true;
}

void UStageZeroSubsystem::StartPostLoadTravelCover(UWorld* LoadedWorld)
{
    const UStageZeroProjectSettings* Settings = GetSettings();
    if (!Settings || !ShouldShowTravelLoadingScreenForWorld(LoadedWorld, bStageZeroInitiatedTravel, PendingTravelMapName))
    {
        RemovePostLoadTravelCover();
        return;
    }

    const int32 HoldFrames = FMath::Max(0, Settings->TravelPostLoadHoldFrames);
    float HoldSeconds = FMath::Max(0.0f, Settings->TravelPostLoadHoldSeconds);
    if (!bTravelMoviePlayerStarted)
    {
        HoldSeconds = FMath::Max(HoldSeconds, GetTravelMinimumDisplaySeconds());
    }

    const bool bWaitForInput = Settings->TravelPostLoadDismissMode == EStageZeroPostLoadDismissMode::PressAnyKey;
    if (HoldFrames <= 0 && HoldSeconds <= 0.0f && !bWaitForInput)
    {
        RemovePostLoadTravelCover();
        return;
    }

    const FString MapNameForCover = PendingTravelMapName;
    const int32 ProfileIndexForCover = ActiveTravelLoadingScreenProfileIndex;
    const bool bWasStageZeroInitiatedTravel = bStageZeroInitiatedTravel;
    const bool bWasTravelMoviePlayerAvailable = bTravelMoviePlayerAvailable;
    const bool bWasTravelMoviePlayerStarted = bTravelMoviePlayerStarted;
    RemovePostLoadTravelCover();
    PendingTravelMapName = MapNameForCover;
    ActiveTravelLoadingScreenProfileIndex = ProfileIndexForCover;
    bTravelLoadingScreenActive = true;
    bStageZeroInitiatedTravel = bWasStageZeroInitiatedTravel;
    bTravelMoviePlayerAvailable = bWasTravelMoviePlayerAvailable;
    bTravelMoviePlayerStarted = bWasTravelMoviePlayerStarted;

    const bool bAddedCover = AddTravelViewportCover(MapNameForCover, true, LoadedWorld);
    if (!bAddedCover)
    {
        UE_LOG(LogStageZeroSubsystem, Warning, TEXT("StageZero: Could not add post-load travel cover for %s."),
            MapNameForCover.IsEmpty() ? TEXT("unknown map") : *MapNameForCover);
    }

    PostLoadTravelCoverFramesRemaining = HoldFrames;
    PostLoadTravelCoverEndTimeSeconds = FPlatformTime::Seconds() + HoldSeconds;
    PostLoadTravelCoverFadeOutStartTimeSeconds = 0.0;
    bPostLoadTravelFadeOutStarted = false;
    bPostLoadTravelWaitingForInput = bWaitForInput;
    PostLoadTravelCoverTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateUObject(this, &UStageZeroSubsystem::TickPostLoadTravelCover),
        0.0f);
    UE_LOG(LogStageZeroSubsystem, Log, TEXT("StageZero: Post-load travel cover for %s will hold for %.2fs and %d frame(s)%s."),
        MapNameForCover.IsEmpty() ? TEXT("unknown map") : *MapNameForCover,
        HoldSeconds,
        HoldFrames,
        bWaitForInput ? TEXT(" plus input dismissal") : TEXT(""));
}

bool UStageZeroSubsystem::TickPostLoadTravelCover(float DeltaSeconds)
{
    (void)DeltaSeconds;

    if (PostLoadTravelCoverFramesRemaining > 0)
    {
        --PostLoadTravelCoverFramesRemaining;
    }

    const bool bFramesDone = PostLoadTravelCoverFramesRemaining <= 0;
    const bool bTimeDone = FPlatformTime::Seconds() >= PostLoadTravelCoverEndTimeSeconds;
    if (!bFramesDone || !bTimeDone)
    {
        return true;
    }

    UWorld* World = GetWorld();
    if (bPostLoadTravelWaitingForInput && !HasPostLoadTravelDismissInput(World))
    {
        return true;
    }

    const UStageZeroProjectSettings* Settings = GetSettings();
    const float FadeOutSeconds = Settings && !Settings->bReduceLoadingScreenMotion
        ? FMath::Max(0.0f, Settings->TravelFadeOutSeconds)
        : 0.0f;
    if (FadeOutSeconds > 0.0f)
    {
        const double Now = FPlatformTime::Seconds();
        if (!bPostLoadTravelFadeOutStarted)
        {
            bPostLoadTravelFadeOutStarted = true;
            PostLoadTravelCoverFadeOutStartTimeSeconds = Now;
        }

        const float Alpha = FMath::Clamp(static_cast<float>((Now - PostLoadTravelCoverFadeOutStartTimeSeconds) / FadeOutSeconds), 0.0f, 1.0f);
        const float Opacity = 1.0f - Alpha;
        if (ActivePostLoadTravelCoverWidget.IsValid())
        {
            ActivePostLoadTravelCoverWidget->SetRenderOpacity(Opacity);
        }
        if (ActivePostLoadTravelUserWidget)
        {
            ActivePostLoadTravelUserWidget->SetRenderOpacity(Opacity);
        }

        if (Alpha < 1.0f)
        {
            return true;
        }
    }

    if (bFramesDone && bTimeDone)
    {
        RemovePostLoadTravelCover();
        return false;
    }

    return true;
}

void UStageZeroSubsystem::RemovePostLoadTravelCover()
{
    if (PostLoadTravelCoverTickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(PostLoadTravelCoverTickerHandle);
        PostLoadTravelCoverTickerHandle.Reset();
    }

    if (TravelMoviePlaybackStartedHandle.IsValid())
    {
        if (IsMoviePlayerEnabled())
        {
            if (IGameMoviePlayer* MoviePlayer = GetMoviePlayer())
            {
                MoviePlayer->OnMoviePlaybackStarted().Remove(TravelMoviePlaybackStartedHandle);
            }
        }
        TravelMoviePlaybackStartedHandle.Reset();
    }

    if (bPostLoadTravelCoverAdded && ActivePostLoadTravelCoverWidget.IsValid())
    {
        UGameViewportClient* ViewportClient = ActiveTravelViewportClient.Get();
        if (!ViewportClient)
        {
            ViewportClient = ResolveTravelViewportClient();
        }

        if (ViewportClient)
        {
            ViewportClient->RemoveViewportWidgetContent(ActivePostLoadTravelCoverWidget.ToSharedRef());
        }
    }

    if (ActivePostLoadTravelUserWidget)
    {
        ActivePostLoadTravelUserWidget->RemoveFromParent();
        ActivePostLoadTravelUserWidget = nullptr;
    }

    ActivePostLoadTravelCoverWidget.Reset();
    ActiveTravelViewportClient.Reset();
    ActiveNativeLoadingScreenProvider = nullptr;
    bPostLoadTravelCoverAdded = false;
    bTravelLoadingScreenActive = false;
    bStageZeroInitiatedTravel = false;
    bTravelMoviePlayerAvailable = false;
    bTravelMoviePlayerStarted = false;
    PostLoadTravelCoverFramesRemaining = 0;
    PostLoadTravelCoverEndTimeSeconds = 0.0;
    PostLoadTravelCoverFadeOutStartTimeSeconds = 0.0;
    bPostLoadTravelFadeOutStarted = false;
    bPostLoadTravelWaitingForInput = false;
    ActiveTravelLoadingScreenProfileIndex = INDEX_NONE;
    PendingTravelMapName.Reset();
    TravelLoadingBackgroundTextures.Reset();
}

void UStageZeroSubsystem::SpawnBuiltInWidgetIfConfigured()
{
    if (!GetWorld())
    {
        return;
    }

    if (!bWarmupRunning || WarmupResult != EStageZeroWarmupResult::Running)
    {
        return;
    }

    if (!ShouldShowWarmupLoadingScreen())
    {
        return;
    }

    if (ActiveLoadingWidget.IsValid())
    {
        return;
    }

    const FStageZeroBootConfig BootConfig = BuildBootConfig();
    if (BootConfig.WidgetClass.IsNull())
    {
        return;
    }

    UClass* WidgetClass = BootConfig.WidgetClass.LoadSynchronous();
    if (!WidgetClass)
    {
        WidgetClass = UStageZeroLoadingWidget::StaticClass();
    }

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC)
    {
        PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    }

    if (!PC && bAllowCreatePlayerIfMissing && GetWorld()->GetNetMode() == NM_Standalone)
    {
        if (UGameInstance* GI = GetWorld()->GetGameInstance())
        {
            if (GI->GetNumLocalPlayers() == 0)
            {
                PC = UGameplayStatics::CreatePlayer(GetWorld(), 0, true);
            }
        }

        if (!PC)
        {
            PC = GetWorld()->GetFirstPlayerController();
        }
    }

    if (!PC)
    {
        UE_LOG(LogStageZeroSubsystem, Warning, TEXT("Stage Zero Subsystem could not find a PlayerController to own the loading widget."));
        return;
    }

    if (UUserWidget* Widget = CreateWidget<UUserWidget>(PC, WidgetClass))
    {
        Widget->AddToViewport();
        ActiveGenericWidget = Widget;
        if (UStageZeroLoadingWidget* TypedWidget = Cast<UStageZeroLoadingWidget>(Widget))
        {
            ActiveLoadingWidget = TypedWidget;
            bWidgetSpawnedBySubsystem = true;
            ApplyBootScreenConfig(TypedWidget);
            TypedWidget->BindToStageZeroSubsystem(this);
        }
    }
}

void UStageZeroSubsystem::ApplyBootScreenConfig(UStageZeroLoadingWidget* Widget)
{
    if (!IsValid(Widget))
    {
        return;
    }

    const FStageZeroBootConfig BootConfig = BuildBootConfig();
    UTexture2D* SpinnerTexture = BootConfig.SpinnerIcon.IsNull() ? nullptr : BootConfig.SpinnerIcon.LoadSynchronous();
    Widget->InitializeBootScreen(
        BootConfig.BackgroundSlides,
        BootConfig.SlideInterval,
        BootConfig.SlideFadeDuration,
        BootConfig.bLoopSlides,
        BootConfig.Tips,
        BootConfig.TipInterval,
        SpinnerTexture,
        true);
    Widget->ConfigureBootMovieOptions(
        BootConfig.bBootMovieSkippable,
        BootConfig.BootMovieSkipText,
        BootConfig.BootMovieSkipDelaySeconds,
        BootConfig.bRestoreSlidesAfterBootMovie);
    Widget->InitializeBootMovie(
        BootConfig.BootMoviePath,
        BootConfig.bLoopBootMovie,
        BootConfig.bPlayBootMovieAudio,
        BootConfig.BootMovieVolume);

    Widget->SetProgressVisibility(false);

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(SpinnerPhaseTimer);
        GetWorld()->GetTimerManager().SetTimer(SpinnerPhaseTimer, this, &UStageZeroSubsystem::ShowProgressAfterSpinnerPhase, BootConfig.SpinnerOnlyDuration, false);
    }
}

void UStageZeroSubsystem::ShowProgressAfterSpinnerPhase()
{
    if (!ActiveLoadingWidget.IsValid())
    {
        return;
    }

    if (!bWarmupRunning || WarmupResult != EStageZeroWarmupResult::Running)
    {
        return;
    }

    const FStageZeroBootConfig BootConfig = BuildBootConfig();
    ActiveLoadingWidget->SetProgressVisibility(true);
    ActiveLoadingWidget->SetSpinnerVisibility(BootConfig.bShowSpinnerDuringProgress);
}

void UStageZeroSubsystem::ConfigureLegacyTransitionMessage(bool bShowMessage, const FString& Message)
{
    bLegacyTransitionMessageEnabled = bShowMessage && !Message.IsEmpty();
    LegacyTransitionMessage = bLegacyTransitionMessageEnabled ? Message : FString();
    if (!bLegacyTransitionMessageEnabled)
    {
        ClearLegacyTransitionPresentation();
    }
}

bool UStageZeroSubsystem::DispatchLegacyTransitionMessage(FName DestinationLevel)
{
    const FString PrimaryPackageName = GetPackageNameFromMap(PrimaryLevelOverride);
    if (DestinationLevel == NAME_None
        || PrimaryPackageName.IsEmpty()
        || DestinationLevel != FName(*PrimaryPackageName))
    {
        return false;
    }

    if (!bLegacyTransitionMessageEnabled || bLegacyTransitionMessageDispatched || LegacyTransitionMessage.IsEmpty())
    {
        return false;
    }

    UStageZeroLoadingWidget* Widget = ActiveLoadingWidget.Get();
    if (!IsValid(Widget))
    {
        return false;
    }

    // Set the guard before invoking Blueprint code so re-entrant travel calls
    // cannot dispatch the event more than once.
    bLegacyTransitionMessageDispatched = true;
    bLegacyTransitionWidgetAwaitingTravelCleanup = true;
    Widget->OnTransitionMessage(LegacyTransitionMessage);
    return true;
}

void UStageZeroSubsystem::ClearLegacyTransitionPresentation()
{
    if (!bLegacyTransitionWidgetAwaitingTravelCleanup)
    {
        return;
    }

    bLegacyTransitionWidgetAwaitingTravelCleanup = false;
    RemoveLoadingWidget();
}

void UStageZeroSubsystem::RemoveLoadingWidget()
{
    const bool bShouldRemoveRegisteredWidget = bWidgetSpawnedBySubsystem || bHideRegisteredWidgetOnComplete;

    if (ActiveLoadingWidget.IsValid())
    {
        if (bShouldRemoveRegisteredWidget || ActiveLoadingWidget->bHideAfterWarmup)
        {
            ActiveLoadingWidget->RemoveFromParent();
        }
        ActiveLoadingWidget.Reset();
    }

    if (ActiveGenericWidget.IsValid())
    {
        if (bShouldRemoveRegisteredWidget)
        {
            ActiveGenericWidget->RemoveFromParent();
        }
        ActiveGenericWidget.Reset();
    }

    bWidgetSpawnedBySubsystem = false;
    bHideRegisteredWidgetOnComplete = false;
}

bool UStageZeroSubsystem::ShouldUseSubsystemBootMusic(const FStageZeroBootConfig& BootConfig) const
{
    if (bHasLegacySettings && !LegacyBootConfig.BackgroundMusic.IsNull())
    {
        return false;
    }
    return !BootConfig.BackgroundMusic.IsNull();
}

void UStageZeroSubsystem::StartBootMusic(const FStageZeroBootConfig& BootConfig)
{
    if (!GetWorld())
    {
        return;
    }

    StopBootMusic(true);

    if (BootConfig.BackgroundMusic.IsNull())
    {
        return;
    }

    USoundBase* Music = BootConfig.BackgroundMusic.LoadSynchronous();
    if (!Music)
    {
        return;
    }

    BootMusicFadeOutTime = FMath::Max(0.0f, BootConfig.MusicFadeOutTime);
    BootMusicComponent = UGameplayStatics::SpawnSound2D(GetWorld(), Music);
    if (BootMusicComponent)
    {
        const float FadeIn = FMath::Max(0.0f, BootConfig.MusicFadeInTime);
        if (FadeIn > 0.0f)
        {
            BootMusicComponent->FadeIn(FadeIn);
        }
    }
}

void UStageZeroSubsystem::StopBootMusic(bool bImmediate)
{
    if (!BootMusicComponent)
    {
        return;
    }

    const float FadeOut = bImmediate ? 0.0f : BootMusicFadeOutTime;
    if (FadeOut > 0.0f)
    {
        BootMusicComponent->FadeOut(FadeOut, 0.0f);
    }
    else
    {
        BootMusicComponent->Stop();
        BootMusicComponent->DestroyComponent();
    }

    BootMusicComponent = nullptr;
    BootMusicFadeOutTime = 0.0f;
}

void UStageZeroSubsystem::UpdateProgressBroadcast(int32 MapCompleted, int32 MapTotal)
{
    const int32 StableMapTotal = FMath::Max(MapTotal, 0);
    const int32 StableDisplayTotal = FMath::Max(1, TotalTasks);
    RawCompletedTasks = FMath::Clamp(TaskBaseline + MapCompleted, 0, StableDisplayTotal);

    const int32 PipelineTotal = bPipelineCacheStageRequested ? FMath::Max(1, PipelinePrecompileBaseline) : 0;
    const int32 PipelineRemaining = bPipelineCacheStageAccepted
        ? FMath::Clamp(GetRecordedPipelineCacheWorkRemaining(), 0, PipelineTotal)
        : PipelineTotal;
    const int32 PipelineCompleted = bPipelineCacheStageAccepted
        ? FMath::Clamp(PipelineTotal - PipelineRemaining, 0, PipelineTotal)
        : 0;
    const int32 OwnedWorkTotal = FMath::Max(1, ManifestTaskTotal + PipelineTotal);
    const int32 OwnedWorkCompleted = FMath::Clamp(ManifestTasksCompleted + PipelineCompleted, 0, OwnedWorkTotal);
    float RawProgress = static_cast<float>(OwnedWorkCompleted) / static_cast<float>(OwnedWorkTotal);
    if (bWarmupRunning)
    {
        RawProgress = FMath::Min(RawProgress, 0.99f);
    }
    LastBroadcastProgress = FMath::Clamp(FMath::Max(LastBroadcastProgress, RawProgress), 0.0f, 1.0f);
    WarmupProgress = LastBroadcastProgress;

    CompletedTasks = FMath::Clamp(FMath::Max(LastBroadcastCompletedTasks, RawCompletedTasks), 0, StableDisplayTotal);
    LastBroadcastCompletedTasks = CompletedTasks;
    LastBroadcastTotalTasks = StableDisplayTotal;

    if (bWarmupRunning && StableDisplayTotal > 0 && CompletedTasks != LastProgressLogCompleted)
    {
        if ((CompletedTasks % 50) == 0 || CompletedTasks == StableDisplayTotal)
        {
            UE_LOG(LogStageZeroSubsystem, Log, TEXT("[StageZero] Warmup progress: ManifestAssets=%d / %d."), CompletedTasks, StableDisplayTotal);
            LastProgressLogCompleted = CompletedTasks;
        }
    }

    if (ActiveLoadingWidget.IsValid() && ActiveLoadingWidget->bHideAfterWarmup && CompletedTasks >= StableDisplayTotal)
    {
        bHideRegisteredWidgetOnComplete = true;
    }

    const int32 DisplayMapCompleted = FMath::Clamp(CompletedTasks - TaskBaseline, 0, StableMapTotal);
    OnWarmupProgress.Broadcast(
        WarmupProgress,
        CurrentMapName,
        CompletedTasks,
        StableDisplayTotal,
        DisplayMapCompleted,
        StableMapTotal);
    BroadcastWarmupStatus();

    // Hide-on-complete is handled by FinishWarmupInternal after the detailed result is known.
}

void UStageZeroSubsystem::SnapshotJobBaselines()
{
    // Stage Zero owns only its manifest terminals and the recorded pipeline-cache
    // stage. Global editor shader and texture-streaming counters belong to the
    // entire process and must not gate this run.
    PipelinePrecompileBaseline = FMath::Max(PipelinePrecompileBaseline, GetRecordedPipelineCacheWorkRemaining());

    CurrentMapTaskTotal = FMath::Max(CurrentMapTaskTotal, ManifestTaskTotal);
    TotalTasks = FMath::Max(TotalTasks, CurrentMapTaskTotal);
}

void UStageZeroSubsystem::StartRealJobPolling(int32 MapCount)
{
    TargetMapCount = MapCount;
    LastObservedOutstandingRuntimeWorkTime = FPlatformTime::Seconds();
    SnapshotJobBaselines();
    PipelineWaitStartTimeSeconds = FPlatformTime::Seconds();
    LastPipelineDecreaseTimeSeconds = PipelineWaitStartTimeSeconds;
    LastPipelineRemaining = GetRecordedPipelineCacheWorkRemaining();
    const TCHAR* PipelineStageState = !bPipelineCacheStageRequested
        ? TEXT("disabled")
        : (bPipelineCacheStageAccepted ? TEXT("accepted") : TEXT("unavailable"));
    UE_LOG(LogStageZeroSubsystem, Log, TEXT("[StageZero] Warmup started. ManifestAssets=%d RecordedPipelineCache=%s InitialWorkUnits=%d."),
        FMath::Max(TotalTasks, 0),
        PipelineStageState,
        PipelinePrecompileBaseline);
    UpdateProgressBroadcast(ManifestTasksCompleted, CurrentMapTaskTotal);

    if (!GetWorld())
    {
        FinishWarmupFailure(EStageZeroWarmupResult::Failed, TEXT("No valid game world was available for runtime warmup polling."));
        return;
    }

    GetWorld()->GetTimerManager().ClearTimer(JobPollTimer);
    GetWorld()->GetTimerManager().SetTimer(JobPollTimer, this, &UStageZeroSubsystem::PollRealJobProgress, 0.1f, true);
    PollRealJobProgress(); // kick off immediately
}

void UStageZeroSubsystem::PollRealJobProgress()
{
    if (!bWarmupRunning)
    {
        if (GetWorld())
        {
            GetWorld()->GetTimerManager().ClearTimer(JobPollTimer);
        }
        return;
    }

    if (bPipelineCacheStageRequested && !bPipelineCacheStageAccepted)
    {
#if STAGEZERO_HAS_SHADER_PIPELINE_CACHE
        const FStageZeroPipelineCacheObservation Observation = StageZeroPipelineCacheObserver::GetSnapshot();
        const FStageZeroPipelineCacheKeyAssessment Assessment = StageZeroPipelineCacheObserver::AssessSnapshotForCacheKeys(
            Observation,
            PipelineExpectedProjectCacheKey,
            PipelineExpectedUserCacheKey,
            PipelineRequiredCacheKeys,
            PipelineObservationBaselineGeneration);
        const bool bObservedUsableCache = !Observation.bProtocolError
            && !Assessment.bHasIncompleteRelevantRecord
            && (Assessment.bHasActiveRelevantRecord
                || (Assessment.bHasCompletedRelevantRecord
                    && Assessment.bAllRelevantRecordsCompleted
                    && Assessment.bAllRequiredRecordsCompleted));
        if (bObservedUsableCache)
        {
            bPipelineCacheStageAccepted = true;
            PipelineCacheStageFailureReason.Reset();
            PipelinePrecompileBaseline = 1;
            PipelineWaitStartTimeSeconds = FPlatformTime::Seconds();
            UE_LOG(LogStageZeroSubsystem, Display, TEXT("StageZero: Adopted a recorded pipeline-cache task observed after manifest loading began."));
        }
#endif
    }

#if STAGEZERO_HAS_SHADER_PIPELINE_CACHE
    if (bPipelineCacheStageAccepted)
    {
        const FStageZeroPipelineCacheObservation Observation = StageZeroPipelineCacheObserver::GetSnapshot();
        const FStageZeroPipelineCacheKeyAssessment Assessment = StageZeroPipelineCacheObserver::AssessSnapshotForCacheKeys(
            Observation,
            PipelineExpectedProjectCacheKey,
            PipelineExpectedUserCacheKey,
            PipelineRequiredCacheKeys,
            PipelineObservationBaselineGeneration);
        if (Observation.bProtocolError
            || (Assessment.bHasIncompleteRelevantRecord && !Assessment.bHasActiveRelevantRecord))
        {
            bPipelineCacheStageAccepted = false;
            PipelineCacheStageFailureReason = Observation.bProtocolError && !Observation.ProtocolErrorReason.IsEmpty()
                ? Observation.ProtocolErrorReason
                : TEXT("A recorded PSO cache closed before a matched completion; PSO coverage is not established.");
        }
    }
#endif

    const int32 PipelineRemaining = GetRecordedPipelineCacheWorkRemaining();
    PipelinePrecompileBaseline = FMath::Max(PipelinePrecompileBaseline, PipelineRemaining);
    if (PipelineRemaining < LastPipelineRemaining)
    {
        LastPipelineDecreaseTimeSeconds = FPlatformTime::Seconds();
    }
    LastPipelineRemaining = PipelineRemaining;

    CurrentMapTaskTotal = FMath::Max(CurrentMapTaskTotal, ManifestTaskTotal);
    TotalTasks = FMath::Max(TotalTasks, CurrentMapTaskTotal);

    const int32 MapCompleted = FMath::Clamp(ManifestTasksCompleted, 0, FMath::Max(CurrentMapTaskTotal, 1));
    UpdateProgressBroadcast(MapCompleted, CurrentMapTaskTotal);

    const bool bManifestDone = !bManifestProcessingActive && ManifestJobQueue.Num() == 0;
    const bool bCountsComplete = (ManifestTaskTotal <= 0 || ManifestTasksCompleted >= ManifestTaskTotal)
        && AreAllManifestJobsTerminal();
    const float PipelineTimeoutSeconds = GetPipelineCacheTimeoutSeconds();
    if (bManifestDone
        && bCountsComplete
        && bPipelineCacheStageAccepted
        && PipelineRemaining > 0
        && (PipelineTimeoutSeconds <= 0.0f || (FPlatformTime::Seconds() - PipelineWaitStartTimeSeconds) >= PipelineTimeoutSeconds))
    {
        bPipelineTimedOut = true;
        const bool bFatalAssetFailures = ManifestFailedLoads > 0 && ShouldTreatAssetFailuresAsFatal();
        const bool bCanContinueAsPartial = ShouldContinueOnPipelineCacheTimeout()
            && bCountsComplete
            && !bFatalAssetFailures;
        if (bCanContinueAsPartial)
        {
            UE_LOG(LogStageZeroSubsystem, Warning, TEXT("[StageZero] Pipeline cache warmup timed out after %.1fs with RemainingPrecompiles=%d. Continuing as Partial; manifest warmup completed and no fatal asset failures remain."),
                PipelineTimeoutSeconds,
                PipelineRemaining);
        }
        else
        {
            UE_LOG(LogStageZeroSubsystem, Error, TEXT("[StageZero] Pipeline cache warmup timed out after %.1fs with RemainingPrecompiles=%d. Warmup will not be reported as success."),
                PipelineTimeoutSeconds,
                PipelineRemaining);
        }
        if (GetWorld())
        {
            GetWorld()->GetTimerManager().ClearTimer(JobPollTimer);
        }
        const EStageZeroWarmupResult TimeoutResult = bCanContinueAsPartial || !IsStrictCompletionEnabled()
            ? EStageZeroWarmupResult::Partial
            : EStageZeroWarmupResult::TimedOut;
        const FString TimeoutReason = bCanContinueAsPartial
            ? FString::Printf(TEXT("Pipeline cache timed out with %d remaining precompiles; continuing with partial PSO coverage."), PipelineRemaining)
            : FString::Printf(TEXT("Pipeline cache timed out with %d remaining precompiles."), PipelineRemaining);
        FinishWarmupInternal(false, TimeoutResult, TimeoutReason);
        return;
    }

    if (!bManifestDone || !bCountsComplete)
    {
        return;
    }

    if (ManifestFailedLoads > 0 && ShouldTreatAssetFailuresAsFatal())
    {
        if (GetWorld())
        {
            GetWorld()->GetTimerManager().ClearTimer(JobPollTimer);
        }
        FinishWarmupInternal(false, EStageZeroWarmupResult::Failed,
            FString::Printf(TEXT("%d required manifest assets failed during warmup."), ManifestFailedLoads));
        return;
    }

    if (bPipelineCacheStageRequested && !bPipelineCacheStageAccepted)
    {
        const bool bShouldAwaitObserver = PipelineRequiredCacheKeys.Num() > 0;
        const float ObservationTimeoutSeconds = GetPipelineCacheTimeoutSeconds();
        const double ObservationWaitElapsed = FPlatformTime::Seconds() - PipelineWaitStartTimeSeconds;
        if (bShouldAwaitObserver
            && ObservationTimeoutSeconds > 0.0f
            && ObservationWaitElapsed < static_cast<double>(ObservationTimeoutSeconds))
        {
            return;
        }

        if (bShouldAwaitObserver)
        {
            bPipelineTimedOut = true;
            PipelineCacheStageFailureReason = FString::Printf(
                TEXT("Recorded PSO cache lifecycle evidence for the required project/user cache key(s) did not complete within %.1f seconds."),
                ObservationTimeoutSeconds);
        }
        if (GetWorld())
        {
            GetWorld()->GetTimerManager().ClearTimer(JobPollTimer);
        }
        FinishWarmupInternal(false,
            IsStrictCompletionEnabled()
                ? (bPipelineTimedOut ? EStageZeroWarmupResult::TimedOut : EStageZeroWarmupResult::Failed)
                : EStageZeroWarmupResult::Partial,
            PipelineCacheStageFailureReason.IsEmpty()
                ? TEXT("Asset preload completed, but no recorded PSO cache was available; PSO coverage is not established.")
                : PipelineCacheStageFailureReason);
        return;
    }

    if (PipelineRemaining > 0)
    {
        return;
    }

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(JobPollTimer);
    }
    if (!DidAllManifestJobsSucceed() || ManifestFailedLoads > 0)
    {
        FinishWarmupInternal(false, EStageZeroWarmupResult::Partial,
            bPipelineCacheStageRequested
                ? FString::Printf(TEXT("Asset preload completed with %d failed required item(s); recorded PSO cache work completed."), ManifestFailedLoads)
                : FString::Printf(TEXT("Asset preload completed with %d failed required item(s); recorded PSO-cache warmup was disabled."), ManifestFailedLoads));
    }
    else
    {
        FinishWarmupInternal(false, EStageZeroWarmupResult::Succeeded,
            bPipelineCacheStageRequested
                ? TEXT("All required manifest assets reached Succeeded and the recorded PSO cache completed.")
                : TEXT("All required manifest assets reached Succeeded; recorded PSO-cache warmup was disabled."));
    }
}

int32 UStageZeroSubsystem::GetRecordedPipelineCacheWorkRemaining() const
{
#if STAGEZERO_HAS_SHADER_PIPELINE_CACHE
    if (!bPipelineCacheStageAccepted)
    {
        return 0;
    }

    const FStageZeroPipelineCacheObservation Observation = StageZeroPipelineCacheObserver::GetSnapshot();
    const FStageZeroPipelineCacheKeyAssessment Assessment = StageZeroPipelineCacheObserver::AssessSnapshotForCacheKeys(
        Observation,
        PipelineExpectedProjectCacheKey,
        PipelineExpectedUserCacheKey,
        PipelineRequiredCacheKeys,
        PipelineObservationBaselineGeneration);
    if (Observation.bProtocolError
        || (Assessment.bHasIncompleteRelevantRecord && !Assessment.bHasActiveRelevantRecord))
    {
        return 1;
    }
    if (!Assessment.bAllRequiredRecordsObserved
        || !Assessment.bAllRequiredRecordsCompleted
        || Assessment.bHasActiveRelevantRecord)
    {
        return 1;
    }
    if (!Assessment.bHasRelevantRecord || !Assessment.bAllRelevantRecordsCompleted)
    {
        return 1;
    }
    return 0;
#else
    return 0;
#endif
}

bool UStageZeroSubsystem::IsWarmupRunning(bool bShowDebug) const
{
    StageZeroDebugMessage(this, bShowDebug, FString::Printf(TEXT("StageZero: WarmupRunning=%s"), bWarmupRunning ? TEXT("true") : TEXT("false")));
    return bWarmupRunning;
}

bool UStageZeroSubsystem::IsWarmupCompleted(bool bShowDebug) const
{
    StageZeroDebugMessage(this, bShowDebug, FString::Printf(TEXT("StageZero: WarmupCompleted=%s"), bWarmupCompleted ? TEXT("true") : TEXT("false")));
    return bWarmupCompleted;
}

float UStageZeroSubsystem::GetWarmupProgress(bool bShowDebug) const
{
    StageZeroDebugMessage(this, bShowDebug, FString::Printf(TEXT("StageZero: WarmupProgress=%.2f%%"), WarmupProgress * 100.0f));
    return WarmupProgress;
}

FName UStageZeroSubsystem::GetCurrentMapName(bool bShowDebug) const
{
    StageZeroDebugMessage(this, bShowDebug, FString::Printf(TEXT("StageZero: CurrentMap=%s"), *CurrentMapName.ToString()));
    return CurrentMapName;
}

int32 UStageZeroSubsystem::GetTotalMaps(bool bShowDebug) const
{
    const int32 TotalMaps = TargetMapCount > 0 ? TargetMapCount : GetWarmupMaps(false).Num();
    StageZeroDebugMessage(this, bShowDebug, FString::Printf(TEXT("StageZero: TotalMaps=%d"), TotalMaps));
    return TotalMaps;
}

int32 UStageZeroSubsystem::GetCompletedMaps(bool bShowDebug) const
{
    StageZeroDebugMessage(this, bShowDebug, FString::Printf(TEXT("StageZero: CompletedMaps=%d"), CompletedMaps));
    return CompletedMaps;
}

TSoftObjectPtr<UWorld> UStageZeroSubsystem::GetPrimaryLevel(bool bShowDebug) const
{
    StageZeroDebugMessage(this, bShowDebug, FString::Printf(TEXT("StageZero: PrimaryLevel=%s"), *PrimaryLevelOverride.ToString()));
    return PrimaryLevelOverride;
}

bool UStageZeroSubsystem::IncludesLevelInstances(bool bShowDebug) const
{
    StageZeroDebugMessage(this, bShowDebug, TEXT("StageZero: IncludesLevelInstances=false (runtime traversal retired)"), FColor::Yellow);
    return false;
}

bool UStageZeroSubsystem::IncludesPackedLevelActors(bool bShowDebug) const
{
    StageZeroDebugMessage(this, bShowDebug, TEXT("StageZero: IncludesPackedLevelActors=false (runtime traversal retired)"), FColor::Yellow);
    return false;
}

bool UStageZeroSubsystem::IsWidgetAutoSpawnEnabled(bool bShowDebug) const
{
    StageZeroDebugMessage(this, bShowDebug, FString::Printf(TEXT("StageZero: AutoSpawnWidget=%s"), bShouldSpawnWidget ? TEXT("true") : TEXT("false")));
    return bShouldSpawnWidget;
}
