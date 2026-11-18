// Copyright (c) 2025 71 Studios. All rights reserved.

#include "StageZeroProjectSettings.h"
#include "StageZeroLoadingWidget.h"

#include "HAL/FileManager.h"
#include "Misc/MessageDialog.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY(LogStageZeroSettings);

namespace
{
FString StageZeroNormalizeMoviePathForValidation(FString MoviePath)
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

    const FString Directory = FPaths::GetPath(MoviePath);
    const FString BaseName = FPaths::GetBaseFilename(MoviePath);
    return Directory.IsEmpty() ? BaseName : (Directory / BaseName);
}

void StageZeroValidateTextureRef(
    const TSoftObjectPtr<UTexture2D>& TextureRef,
    const FString& Label,
    TArray<FString>& OutIssues)
{
    if (TextureRef.IsNull())
    {
        return;
    }

    UObject* LoadedObject = TextureRef.ToSoftObjectPath().TryLoad();
    if (!LoadedObject || !LoadedObject->IsA<UTexture2D>())
    {
        OutIssues.Add(FString::Printf(TEXT("%s texture is missing or is not a Texture2D: %s"),
            *Label,
            *TextureRef.ToSoftObjectPath().ToString()));
    }
}

void StageZeroValidateWidgetClassRef(
    TSoftClassPtr<UUserWidget> WidgetClassRef,
    const FString& Label,
    TArray<FString>& OutIssues)
{
    if (WidgetClassRef.IsNull())
    {
        return;
    }

    UClass* WidgetClass = WidgetClassRef.LoadSynchronous();
    if (!WidgetClass || !WidgetClass->IsChildOf(UUserWidget::StaticClass()))
    {
        OutIssues.Add(FString::Printf(TEXT("%s widget class is missing or is not a UserWidget: %s"),
            *Label,
            *WidgetClassRef.ToSoftObjectPath().ToString()));
    }
}

void StageZeroValidateProviderClassRef(
    TSoftClassPtr<UStageZeroNativeLoadingScreenProvider> ProviderClassRef,
    const FString& Label,
    TArray<FString>& OutIssues)
{
    if (ProviderClassRef.IsNull())
    {
        return;
    }

    UClass* ProviderClass = ProviderClassRef.LoadSynchronous();
    if (!ProviderClass || !ProviderClass->IsChildOf(UStageZeroNativeLoadingScreenProvider::StaticClass()))
    {
        OutIssues.Add(FString::Printf(TEXT("%s provider class is missing or is not a StageZeroNativeLoadingScreenProvider: %s"),
            *Label,
            *ProviderClassRef.ToSoftObjectPath().ToString()));
    }
}

void StageZeroValidateMoviePath(
    const FString& MoviePath,
    const FString& Label,
    TArray<FString>& OutIssues)
{
    FString TrimmedPath = MoviePath;
    TrimmedPath.TrimStartAndEndInline();
    if (TrimmedPath.IsEmpty())
    {
        return;
    }

    if (FPaths::FileExists(TrimmedPath))
    {
        return;
    }

    const FString NormalizedMoviePath = StageZeroNormalizeMoviePathForValidation(TrimmedPath);
    if (NormalizedMoviePath.IsEmpty())
    {
        return;
    }

    static const TCHAR* SupportedExtensions[] =
    {
        TEXT("mp4"),
        TEXT("bk2"),
        TEXT("webm"),
        TEXT("mov"),
        TEXT("avi")
    };

    for (const TCHAR* Extension : SupportedExtensions)
    {
        const FString CandidatePath = FPaths::ProjectContentDir() / TEXT("Movies") / (NormalizedMoviePath + TEXT(".") + Extension);
        if (FPaths::FileExists(CandidatePath))
        {
            return;
        }
    }

    OutIssues.Add(FString::Printf(TEXT("%s movie was not found under Content/Movies: %s"),
        *Label,
        *MoviePath));
}

void StageZeroValidateLoadingScreenProfile(
    const FStageZeroLoadingScreenProfile& Profile,
    int32 ProfileIndex,
    TArray<FString>& OutIssues)
{
    const FString ProfileNameSuffix = Profile.ProfileName.IsNone()
        ? FString()
        : FString::Printf(TEXT(" (%s)"), *Profile.ProfileName.ToString());
    const FString Label = FString::Printf(TEXT("Profile %d%s"),
        ProfileIndex,
        *ProfileNameSuffix);

    if (!Profile.DestinationMap.IsNull())
    {
        UObject* LoadedMap = Profile.DestinationMap.ToSoftObjectPath().TryLoad();
        if (!LoadedMap || !LoadedMap->IsA<UWorld>())
        {
            OutIssues.Add(FString::Printf(TEXT("%s destination map is missing or invalid: %s"),
                *Label,
                *Profile.DestinationMap.ToSoftObjectPath().ToString()));
        }
    }

    for (int32 Index = 0; Index < Profile.BackgroundImages.Num(); ++Index)
    {
        StageZeroValidateTextureRef(Profile.BackgroundImages[Index], FString::Printf(TEXT("%s background image %d"), *Label, Index), OutIssues);
    }

    for (int32 Index = 0; Index < Profile.MoviePaths.Num(); ++Index)
    {
        StageZeroValidateMoviePath(Profile.MoviePaths[Index], FString::Printf(TEXT("%s movie %d"), *Label, Index), OutIssues);
    }

    StageZeroValidateWidgetClassRef(Profile.PostLoadWidgetClass, Label + TEXT(" post-load widget"), OutIssues);
}
}

UStageZeroProjectSettings::UStageZeroProjectSettings()
{
    CategoryName = TEXT("Plugins");
    SectionName = TEXT("StageZero");

    TravelTitleText = NSLOCTEXT("StageZeroProjectSettings", "DefaultTravelTitle", "Stage Zero");
    TravelSubtitleText = NSLOCTEXT("StageZeroProjectSettings", "DefaultTravelSubtitle", "Preparing the next level");
    TravelStatusText = NSLOCTEXT("StageZeroProjectSettings", "DefaultTravelStatus", "Loading");
    TravelPressAnyKeyText = NSLOCTEXT("StageZeroProjectSettings", "DefaultTravelPressAnyKey", "Press any key to continue");
    TravelFooterText = FText::GetEmpty();
    BootMovieSkipText = NSLOCTEXT("StageZeroProjectSettings", "DefaultBootMovieSkipText", "Press any key to skip");

    // Default to the packaged sample widget blueprint so backgrounds/spinner/progress have a UI.
    const FSoftObjectPath DefaultWidgetPath(TEXT("/StageZero/StageZero/WB_Loading.WB_Loading_C"));
    BuiltInLoadingWidgetClass = TSoftClassPtr<UUserWidget>(DefaultWidgetPath);

    // The Stage Zero boot/warmup map owns the main prewarming screen and must not be covered by the travel loading screen.
    TravelLoadingScreenExcludedMaps.Add(TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/StageZero/StageZero/StageZero-BootLevel.StageZero-BootLevel"))));

    // Fallback to the C++ base if the asset is missing.
    if (BuiltInLoadingWidgetClass.IsNull())
    {
        BuiltInLoadingWidgetClass = UStageZeroLoadingWidget::StaticClass();
    }
}

void UStageZeroProjectSettings::ValidateLoadingScreenAssets() const
{
#if WITH_EDITOR
    TArray<FString> Issues;

    for (int32 Index = 0; Index < TravelBackgroundImages.Num(); ++Index)
    {
        StageZeroValidateTextureRef(TravelBackgroundImages[Index], FString::Printf(TEXT("Travel background image %d"), Index), Issues);
    }

    for (int32 Index = 0; Index < TravelMoviePaths.Num(); ++Index)
    {
        StageZeroValidateMoviePath(TravelMoviePaths[Index], FString::Printf(TEXT("Travel movie %d"), Index), Issues);
    }

    StageZeroValidateWidgetClassRef(BuiltInLoadingWidgetClass, TEXT("Built-in warmup loading widget"), Issues);
    StageZeroValidateMoviePath(BootMoviePath, TEXT("Boot movie"), Issues);
    StageZeroValidateWidgetClassRef(TravelPostLoadWidgetClass, TEXT("Travel post-load widget"), Issues);
    StageZeroValidateProviderClassRef(NativeLoadingScreenProviderClass, TEXT("Native Slate loading screen provider"), Issues);

    for (int32 Index = 0; Index < TravelLoadingScreenProfiles.Num(); ++Index)
    {
        StageZeroValidateLoadingScreenProfile(TravelLoadingScreenProfiles[Index], Index, Issues);
    }

    if (Issues.Num() == 0)
    {
        UE_LOG(LogStageZeroSettings, Display, TEXT("StageZero: Loading screen asset validation passed."));
        FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("StageZeroProjectSettings", "LoadingScreenValidationPassed", "Stage Zero loading screen asset validation passed."));
        return;
    }

    UE_LOG(LogStageZeroSettings, Warning, TEXT("StageZero: Loading screen asset validation found %d issue(s)."), Issues.Num());
    for (const FString& Issue : Issues)
    {
        UE_LOG(LogStageZeroSettings, Warning, TEXT("StageZero: %s"), *Issue);
    }

    FFormatNamedArguments Args;
    Args.Add(TEXT("Count"), FText::AsNumber(Issues.Num()));
    FMessageDialog::Open(
        EAppMsgType::Ok,
        FText::Format(
            NSLOCTEXT("StageZeroProjectSettings", "LoadingScreenValidationIssues", "Stage Zero loading screen validation found {Count} issue(s). Check the Output Log for details."),
            Args));
#else
    UE_LOG(LogStageZeroSettings, Warning, TEXT("StageZero: Loading screen asset validation is only available in editor builds."));
#endif
}
