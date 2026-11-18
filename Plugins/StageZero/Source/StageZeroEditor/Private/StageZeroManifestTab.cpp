#include "StageZeroManifestTab.h"

#include "StageZeroWarmupManifest.h"
#include "StageZeroAssetRegistryCompat.h"
#include "StageZeroNumberFormatting.h"
#include "StageZeroWarmupUtils.h"
#include "StageZeroManifestBuilder.h"
#include "StageZeroManifestLogParser.h"
#include "StageZeroManifestOverrideSerialization.h"
#include "StageZeroManifestOverridesEditorObject.h"

#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ContentBrowserModule.h"
#include "Editor.h"
#include "EditorStyleSet.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Notifications/NotificationManager.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "IContentBrowserSingleton.h"
#include "Engine/World.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "FileHelpers.h"
#include "PackageTools.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "Styling/AppStyle.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "UnrealEdMisc.h"
#include "UObject/GarbageCollection.h"
#include "UObject/Package.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UObjectIterator.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "Runtime/Launch/Resources/Version.h"

DEFINE_LOG_CATEGORY_STATIC(LogStageZeroManifestTab, Log, All);

namespace
{
static const FSlateBrush* StageZeroEditorBrush(const ANSICHAR* BrushName)
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 0
    return FEditorStyle::GetBrush(FName(BrushName));
#else
    return FAppStyle::GetBrush(BrushName);
#endif
}

static FString StageZeroFormatBuildStatus(const FStageZeroManifestBuildResult& Result)
{
    FString Message = FString::Printf(
        TEXT("Requested=%d | Resolved=%d | Missing=%d | Processed=%d | Zero-asset maps=%d | World-load failures=%d"),
        Result.NumRequestedMaps,
        Result.NumResolvedMaps,
        Result.NumMissingRequestedMaps,
        Result.NumProcessedMaps,
        Result.NumZeroAssetMaps,
        Result.NumWorldLoadFailures);

    if (Result.bSucceeded)
    {
        if (Result.bForcedAllZeroSave)
        {
            return FString::Printf(TEXT("Manifest saved for diagnostics. %s"), *Message);
        }
        return FString::Printf(TEXT("Manifest build succeeded. %s"), *Message);
    }

    if (Result.bExistingManifestPreserved)
    {
        return FString::Printf(TEXT("Manifest build failed; existing manifest preserved. %s"), *Message);
    }

    return FString::Printf(TEXT("Manifest build failed. %s"), *Message);
}

static FSlateColor StageZeroBuildStatusColor(const FStageZeroManifestBuildResult& Result)
{
    if (Result.bSucceeded)
    {
        return Result.bForcedAllZeroSave ? FSlateColor(FLinearColor::Yellow) : FSlateColor(FLinearColor::White);
    }

    return Result.bExistingManifestPreserved ? FSlateColor(FLinearColor::Yellow) : FSlateColor(FLinearColor::Red);
}

static const TCHAR* StageZeroFabListingUrl = TEXT("https://www.fab.com/listings/65d47dde-ec15-48ea-aa7c-8cc58ccf1725");
static const TCHAR* StageZeroDiscordUrl = TEXT("https://discord.gg/f9kDJE6KUD");
static const TCHAR* StageZeroRoadmapUrl = TEXT("https://71studios.net/roadmap");
static const TCHAR* StageZeroWalkthroughUrl = TEXT("https://www.youtube.com/watch?v=gaqqKtk85Cg");
static const TCHAR* StageZeroSupportEmailUrl = TEXT("mailto:support@71studios.net");

static FReply StageZeroOpenUrl(const TCHAR* Url)
{
    FPlatformProcess::LaunchURL(Url, nullptr, nullptr);
    return FReply::Handled();
}

static FString StageZeroGetVersionLabel()
{
    if (const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("StageZero")))
    {
        const FString& VersionName = Plugin->GetDescriptor().VersionName;
        if (!VersionName.IsEmpty())
        {
            return FString::Printf(TEXT("v%s"), *VersionName);
        }
    }

    return TEXT("Version unknown");
}

static FString StageZeroFormatCount(int64 Value)
{
    return FStageZeroNumberFormatting::FormatConfiguredCount(Value);
}

static bool StageZeroSaveDirtySourcePackages(bool& bOutUserCancelled)
{
#if WITH_EDITOR
    bOutUserCancelled = false;
    TArray<UPackage*> DirtySourcePackages;
    const FString ManifestPackageName = FStageZeroWarmupUtils::GetManifestPackagePath();
    for (TObjectIterator<UPackage> It; It; ++It)
    {
        UPackage* Package = *It;
        if (!Package
            || !Package->IsDirty()
            || Package->HasAnyFlags(RF_Transient)
            || Package->HasAnyPackageFlags(PKG_CompiledIn | PKG_PlayInEditor))
        {
            continue;
        }
        const FString PackageName = Package->GetName();
        if (PackageName == ManifestPackageName
            || !FPackageName::IsValidLongPackageName(PackageName)
            || PackageName.StartsWith(TEXT("/Engine/"))
            || PackageName.StartsWith(TEXT("/Script/"))
            || FPackageName::IsMemoryPackage(PackageName)
            || FPackageName::IsTempPackage(PackageName))
        {
            continue;
        }
        DirtySourcePackages.Add(Package);
    }

    if (DirtySourcePackages.IsEmpty())
    {
        return true;
    }

    const EAppReturnType::Type Decision = FMessageDialog::Open(
        EAppMsgType::OkCancel,
        FText::FromString(FString::Printf(
            TEXT("Stage Zero found %d unsaved content package(s). Manifest generation must use saved source data so the result matches cooking and external commandlets.\n\nChoose OK to review/save them, or Cancel to stop without changing the manifest."),
            DirtySourcePackages.Num())),
        FText::FromString(TEXT("Save Manifest Source Packages")));
    if (Decision != EAppReturnType::Ok)
    {
        bOutUserCancelled = true;
        return false;
    }

    if (FEditorFileUtils::PromptForCheckoutAndSave(DirtySourcePackages, true, false) != FEditorFileUtils::PR_Success)
    {
        UE_LOG(LogStageZeroManifestTab, Error, TEXT("StageZero: Dirty manifest source packages were not saved; generation was not started."));
        return false;
    }
    return true;
#else
    bOutUserCancelled = false;
    return false;
#endif
}

static bool StageZeroPrepareManifestAsset(bool bUnloadForExternalBuild, bool& bOutUserCancelled)
{
#if WITH_EDITOR
    bOutUserCancelled = false;
    bool bDiscardedDirtyManifest = false;
    UPackage* ManifestPackage = FindPackage(nullptr, FStageZeroWarmupUtils::GetManifestPackagePath());
    if (ManifestPackage && ManifestPackage->IsDirty())
    {
        const EAppReturnType::Type Decision = FMessageDialog::Open(
            EAppMsgType::YesNoCancel,
            FText::FromString(TEXT("The Stage Zero manifest has unsaved changes.\n\nYes: save the current manifest before rebuilding.\nNo: explicitly discard those changes.\nCancel: leave the manifest untouched and stop.")),
            FText::FromString(TEXT("Stage Zero Manifest Has Unsaved Changes")));
        if (Decision == EAppReturnType::Cancel)
        {
            bOutUserCancelled = true;
            return false;
        }
        if (Decision == EAppReturnType::Yes)
        {
            TArray<UPackage*> PackagesToSave;
            PackagesToSave.Add(ManifestPackage);
            if (FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, true, false) != FEditorFileUtils::PR_Success)
            {
                UE_LOG(LogStageZeroManifestTab, Error, TEXT("StageZero: The dirty manifest could not be saved; external build was not started."));
                return false;
            }
        }
        else
        {
            bDiscardedDirtyManifest = true;
        }
    }

    if (!bUnloadForExternalBuild && !bDiscardedDirtyManifest)
    {
        return true;
    }

    UObject* LoadedManifest = FSoftObjectPath(FStageZeroWarmupUtils::GetManifestObjectPath()).ResolveObject();
    if (LoadedManifest && GEditor)
    {
        if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
        {
            AssetEditorSubsystem->CloseAllEditorsForAsset(LoadedManifest);
        }
    }

    if (ManifestPackage)
    {
        TArray<UPackage*> PackagesToUnload;
        PackagesToUnload.Add(ManifestPackage);
        UPackageTools::FUnloadPackageParams UnloadParams(PackagesToUnload);
        UnloadParams.bUnloadDirtyPackages = bDiscardedDirtyManifest;
        UnloadParams.bResetTransBuffer = false;
        if (!UPackageTools::UnloadPackages(UnloadParams))
        {
            UE_LOG(LogStageZeroManifestTab, Error,
                TEXT("StageZero: Could not unload the manifest without clearing editor Undo history; external generation was not started. %s"),
                *UnloadParams.OutErrorMessage.ToString());
            return false;
        }
        if (FindPackage(nullptr, FStageZeroWarmupUtils::GetManifestPackagePath()))
        {
            UE_LOG(LogStageZeroManifestTab, Error, TEXT("StageZero: Manifest package remained loaded after unload; external generation was not started."));
            return false;
        }
    }
    return true;
#else
    bOutUserCancelled = false;
    return false;
#endif
}

static FString StageZeroFindLatestManifestLog()
{
    const FString LogDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectLogDir());
    TArray<FString> LogFiles;
    IFileManager::Get().FindFiles(LogFiles, *(LogDirectory / TEXT("StageZeroBuildManifest-*.log")), true, false);
    LogFiles.Sort([](const FString& Left, const FString& Right)
    {
        return Left > Right;
    });
    if (!LogFiles.IsEmpty())
    {
        return LogDirectory / LogFiles[0];
    }
    return LogDirectory / TEXT("StageZeroBuildManifest.log");
}

static void StageZeroRefreshManifestRegistrationFromDisk()
{
    const FString ManifestFilePath = FPackageName::LongPackageNameToFilename(
        FStageZeroWarmupUtils::GetManifestPackagePath(),
        FPackageName::GetAssetPackageExtension());
    if (FPaths::FileExists(ManifestFilePath))
    {
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"))
            .Get()
            .ScanModifiedAssetFiles({ ManifestFilePath });
    }
}
}

SStageZeroManifestTab::~SStageZeroManifestTab()
{
    SyncSelectionToOptions();
    if (ManualOverridesEditorObject.IsValid())
    {
        BuildOptions.GlobalAdditionalAssets = ManualOverridesEditorObject->GlobalAdditionalAssets;
        BuildOptions.GlobalExcludedAssets = ManualOverridesEditorObject->GlobalExcludedAssets;
        BuildOptions.PerMapAssetOverrides = ManualOverridesEditorObject->PerMapAssetOverrides;
    }
    SaveOptionsToConfig();
    StopManifestPoll();
    TerminateManifestProcess();
}

void SStageZeroManifestTab::Construct(const FArguments& InArgs)
{
    const FSlateFontInfo TitleFont = FAppStyle::Get().GetFontStyle("NormalFont");
    const FSlateFontInfo SubTitleFont = FAppStyle::Get().GetFontStyle("NormalFont");
    const FSlateFontInfo StatFont = FAppStyle::Get().GetFontStyle("NormalFont");

    LoadOptionsFromConfig();
    ManualOverridesEditorObject = TStrongObjectPtr<UStageZeroManifestOverridesEditorObject>(
        NewObject<UStageZeroManifestOverridesEditorObject>(GetTransientPackage()));
    SyncManualOverridesEditorFromOptions();

    FDetailsViewArgs ManualOverrideDetailsArgs;
    ManualOverrideDetailsArgs.bAllowSearch = false;
    ManualOverrideDetailsArgs.bHideSelectionTip = true;
    ManualOverrideDetailsArgs.bLockable = false;
    ManualOverrideDetailsArgs.bUpdatesFromSelection = false;
    ManualOverrideDetailsArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
    ManualOverridesDetailsView = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"))
        .CreateDetailView(ManualOverrideDetailsArgs);
    ManualOverridesDetailsView->SetObject(ManualOverridesEditorObject.Get());
    ManualOverridesDetailsView->OnFinishedChangingProperties().AddSP(
        this,
        &SStageZeroManifestTab::HandleManualOverridesChanged);
    GatherWorldAssets();

    ChildSlot
    [
        SNew(SBorder)
        .Padding(12.0f)
        .BorderImage(StageZeroEditorBrush("ToolPanel.GroupBorder"))
        [
            SNew(SVerticalBox)

            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SBorder)
                .Padding(FMargin(10.f, 8.f))
                .BorderImage(StageZeroEditorBrush("ToolPanel.GroupBorder"))
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    .FillWidth(1.f)
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(STextBlock)
                            .Text(FText::FromString(TEXT("Stage Zero Manifest Studio")))
                            .Font(TitleFont)
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(FMargin(0, 2, 0, 0))
                        [
                            SNew(STextBlock)
                            .Text(FText::FromString(TEXT("Generate, validate, and explain warmup coverage before packaged builds.")))
                            .Font(SubTitleFont)
                            .WrapTextAt(560.f)
                        ]
                    ]
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    .Padding(FMargin(14, 0, 0, 0))
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .HAlign(HAlign_Right)
                        [
                            SNew(STextBlock)
                            .Text(FText::FromString(StageZeroGetVersionLabel()))
                            .Justification(ETextJustify::Right)
                            .ToolTipText(FText::FromString(TEXT("Stage Zero plugin version from the .uplugin descriptor.")))
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(FMargin(0, 5, 0, 0))
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .Padding(FMargin(0, 0, 4, 0))
                            [
                                SNew(SButton)
                                .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                                .ContentPadding(FMargin(6.f, 2.f))
                                .Text(FText::FromString(TEXT("Rate")))
                                .ToolTipText(FText::FromString(TEXT("Open the Stage Zero Fab listing.")))
                                .OnClicked(this, &SStageZeroManifestTab::HandleOpenRatePlugin)
                            ]
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .Padding(FMargin(0, 0, 4, 0))
                            [
                                SNew(SButton)
                                .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                                .ContentPadding(FMargin(6.f, 2.f))
                                .Text(FText::FromString(TEXT("Discord")))
                                .ToolTipText(FText::FromString(TEXT("Open the 71 Studios Discord support server.")))
                                .OnClicked(this, &SStageZeroManifestTab::HandleOpenDiscord)
                            ]
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .Padding(FMargin(0, 0, 4, 0))
                            [
                                SNew(SButton)
                                .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                                .ContentPadding(FMargin(6.f, 2.f))
                                .Text(FText::FromString(TEXT("Roadmap")))
                                .ToolTipText(FText::FromString(TEXT("Open the Stage Zero roadmap.")))
                                .OnClicked(this, &SStageZeroManifestTab::HandleOpenRoadmap)
                            ]
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .Padding(FMargin(0, 0, 4, 0))
                            [
                                SNew(SButton)
                                .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                                .ContentPadding(FMargin(6.f, 2.f))
                                .Text(FText::FromString(TEXT("Video")))
                                .ToolTipText(FText::FromString(TEXT("Open the Stage Zero walkthrough video.")))
                                .OnClicked(this, &SStageZeroManifestTab::HandleOpenWalkthrough)
                            ]
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            [
                                SNew(SButton)
                                .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                                .ContentPadding(FMargin(6.f, 2.f))
                                .Text(FText::FromString(TEXT("Email")))
                                .ToolTipText(FText::FromString(TEXT("Email support@71studios.net.")))
                                .OnClicked(this, &SStageZeroManifestTab::HandleOpenSupportEmail)
                            ]
                        ]
                    ]
                ]
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(FMargin(0, 2, 0, 10))
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("Generate and inspect the Stage Zero shader/material/texture manifest. Run this before cooking or sharing builds so warmup counts stay accurate.")))
                .Font(SubTitleFont)
                .WrapTextAt(620.f)
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SSeparator)
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(FMargin(0, 6))
            [
                SNew(SBorder)
                .Padding(6.0f)
                .BorderImage(StageZeroEditorBrush("ToolPanel.GroupBorder"))
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("Maps to include")))
                        .Font(SubTitleFont)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(FMargin(0, 4, 0, 2))
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(FMargin(0, 0, 12, 0))
                        [
                            SNew(SCheckBox)
                            .Style(FAppStyle::Get(), "RadioButton")
                            .IsChecked(this, &SStageZeroManifestTab::GetUseAllMapsState)
                            .OnCheckStateChanged(this, &SStageZeroManifestTab::OnUseAllMapsChanged)
                            .ToolTipText(FText::FromString(TEXT("Scan all UWorld maps in the project when building the Stage Zero manifest.")))
                            [
                                SNew(STextBlock).Text(FText::FromString(TEXT("All maps in project")))
                            ]
                        ]
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        [
                            SNew(SCheckBox)
                            .Style(FAppStyle::Get(), "RadioButton")
                            .IsChecked(this, &SStageZeroManifestTab::GetUseSelectedMapsState)
                            .OnCheckStateChanged(this, &SStageZeroManifestTab::OnUseSelectedMapsChanged)
                            .ToolTipText(FText::FromString(TEXT("Only include the maps you explicitly select below when building the Stage Zero manifest.")))
                            [
                                SNew(STextBlock).Text(FText::FromString(TEXT("Selected maps only")))
                            ]
                        ]
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(FMargin(0, 2, 0, 4))
                    [
                        SNew(STextBlock)
                        .Text_Lambda([this]() { return FText::FromString(GetSelectedMapSummary()); })
                        .ColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.82f, 0.92f)))
                        .ToolTipText(FText::FromString(TEXT("Stage Zero restores selected map paths from saved manifest generation options when the tab opens.")))
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(FMargin(0, 4, 0, 2))
                    [
                        SNew(SCheckBox)
                        .IsChecked(this, &SStageZeroManifestTab::GetIncludeEngineContentState)
                        .OnCheckStateChanged(this, &SStageZeroManifestTab::OnIncludeEngineContentChanged)
                        .ToolTipText(FText::FromString(TEXT("If enabled, also scan maps under /Engine when building the manifest. This can be slower and is usually unnecessary unless you rely on engine content maps.")))
                        [
                            SNew(STextBlock).Text(FText::FromString(TEXT("Include Engine content (/Engine)")))
                        ]
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(FMargin(0, 2))
                    [
                        SNew(SVerticalBox)
                        .Visibility(this, &SStageZeroManifestTab::GetMapSelectionVisibility)
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(FMargin(0, 0, 0, 4))
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .Padding(FMargin(0, 0, 6, 0))
                            [
                                SNew(SButton)
                                .ContentPadding(FMargin(6.f, 2.f))
                                .Text(FText::FromString(TEXT("Select All")))
                                .OnClicked_Lambda([this]()
                                {
                                    SetAllMapSelections(true);
                                    return FReply::Handled();
                                })
                            ]
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .Padding(FMargin(0, 0, 6, 0))
                            [
                                SNew(SButton)
                                .ContentPadding(FMargin(6.f, 2.f))
                                .Text(FText::FromString(TEXT("Select None")))
                                .OnClicked_Lambda([this]()
                                {
                                    SetAllMapSelections(false);
                                    return FReply::Handled();
                                })
                            ]
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(SBox)
                            .HeightOverride(260.f)
                            [
                                SAssignNew(MapListView, SListView<TSharedPtr<FStageZeroMapListEntry>>)
                                .ListItemsSource(&MapEntries)
                                .SelectionMode(ESelectionMode::None)
                                .ToolTipText(FText::FromString(TEXT("Choose which maps Stage Zero will scan when generating the warmup manifest.")))
                                .OnGenerateRow(this, &SStageZeroManifestTab::MakeMapRowWidget)
                            ]
                        ]
                    ]
                ]
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(FMargin(0, 6))
            [
                SNew(SBorder)
                .Padding(6.0f)
                .BorderImage(StageZeroEditorBrush("ToolPanel.GroupBorder"))
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("World Partition coverage")))
                        .Font(SubTitleFont)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(FMargin(0, 4, 0, 2))
                    [
                        SNew(SCheckBox)
                        .IsChecked_Lambda([this]() { return BuildOptions.bIncludeWorldPartition ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
                        .OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
                        {
                            BuildOptions.bIncludeWorldPartition = (NewState == ECheckBoxState::Checked);
                            if (!BuildOptions.bIncludeWorldPartition)
                            {
                                BuildOptions.bIncludeAnchorAndSplineRoutes = false;
                            }
                            SaveOptionsToConfig();
                        })
                        .ToolTipText(FText::FromString(TEXT("Scan World Partition actor descriptors and external-actor packages while building the manifest. This is editor-only authoring and does not stream or render world content at runtime.")))
                        [
                            SNew(STextBlock).Text(FText::FromString(TEXT("Include World Partition data")))
                        ]
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(SCheckBox)
                        .IsEnabled_Lambda([this]() { return BuildOptions.bIncludeWorldPartition; })
                        .IsChecked_Lambda([this]() { return BuildOptions.bIncludeAnchorAndSplineRoutes ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
                        .OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
                        {
                            BuildOptions.bIncludeAnchorAndSplineRoutes = (NewState == ECheckBoxState::Checked);
                            SaveOptionsToConfig();
                        })
                        .ToolTipText(FText::FromString(TEXT("Bake asset groups for placed Stage Zero World Partition anchors and spline routes during manifest generation. Runtime warmup only loads these precomputed asset references; it does not traverse, stream, capture, or render the world.")))
                        [
                            SNew(STextBlock).Text(FText::FromString(TEXT("Include anchor and spline routes")))
                        ]
                    ]
                ]
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(FMargin(0, 6))
            [
                SNew(SBorder)
                .Padding(6.0f)
                .BorderImage(StageZeroEditorBrush("ToolPanel.GroupBorder"))
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("Asset collection options")))
                        .Font(SubTitleFont)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(FMargin(0, 4, 0, 2))
                    [
                        SNew(SCheckBox)
                        .IsChecked_Lambda([this]() { return BuildOptions.bIncludeTextures ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
                        .OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
                        {
                            BuildOptions.bIncludeTextures = (NewState == ECheckBoxState::Checked);
                            if (!BuildOptions.bIncludeTextures)
                            {
                                BuildOptions.bIncludeAllProjectTextures = false;
                            }
                            SaveOptionsToConfig();
                        })
                        .ToolTipText(FText::FromString(TEXT("If enabled, Stage Zero automatically discovers textures alongside materials and Niagara systems. Disabling this reduces manifest size and build time; explicitly added manual texture overrides are still honored.")))
                        [
                            SNew(STextBlock).Text(FText::FromString(TEXT("Include textures in manifest")))
                        ]
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(SCheckBox)
                        .IsChecked_Lambda([this]() { return BuildOptions.bIncludeAllProjectMaterials ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
                        .OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
                        {
                            BuildOptions.bIncludeAllProjectMaterials = (NewState == ECheckBoxState::Checked);
                            SaveOptionsToConfig();
                        })
                        .ToolTipText(FText::FromString(TEXT("If enabled, Stage Zero will add every UMaterialInterface under the selected roots to the manifest, not just materials discovered from selected maps. Use this for broad coverage when map dependency discovery is incomplete.")))
                        [
                            SNew(STextBlock).Text(FText::FromString(TEXT("Include all project materials (slow)")))
                        ]
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(SCheckBox)
                        .IsEnabled_Lambda([this]() { return BuildOptions.bIncludeTextures; })
                        .IsChecked_Lambda([this]() { return BuildOptions.bIncludeAllProjectTextures ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
                        .OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
                        {
                            BuildOptions.bIncludeAllProjectTextures = (NewState == ECheckBoxState::Checked);
                            SaveOptionsToConfig();
                        })
                        .ToolTipText(FText::FromString(TEXT("If enabled, Stage Zero will add all UTexture assets in the project to the manifest, not just those referenced by the selected maps. This may significantly increase manifest build time on large projects.")))
                        [
                            SNew(STextBlock).Text(FText::FromString(TEXT("Include all project textures (slow)")))
                        ]
                    ]
                ]
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(FMargin(0, 6))
            [
                SNew(SExpandableArea)
                .InitiallyCollapsed(true)
                .BorderImage(StageZeroEditorBrush("ToolPanel.GroupBorder"))
                .HeaderContent()
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("Manual asset overrides (Advanced)")))
                    .Font(SubTitleFont)
                    .ToolTipText(FText::FromString(TEXT("Use only for cookable runtime assets that automatic map discovery intentionally cannot see. Generated manifest arrays remain read-only.")))
                ]
                .BodyContent()
                [
                    SNew(SBorder)
                    .Padding(FMargin(8.f, 6.f))
                    .BorderImage(StageZeroEditorBrush("ToolPanel.GroupBorder"))
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(FMargin(0, 0, 0, 6))
                        [
                            SNew(STextBlock)
                            .Text(FText::FromString(TEXT("Includes supplement generated coverage. Exclusions are applied last and always win. Invalid includes or map keys stop the build before the existing manifest is replaced.")))
                            .WrapTextAt(720.f)
                            .ColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.82f, 0.92f)))
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            ManualOverridesDetailsView.ToSharedRef()
                        ]
                    ]
                ]
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(FMargin(0, 6))
            [
                SNew(SBorder)
                .Padding(FMargin(8.0f))
                .BorderImage(StageZeroEditorBrush("ToolPanel.GroupBorder"))
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("PSO Guidance")))
                        .Font(SubTitleFont)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(FMargin(0, 4, 0, 0))
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("Stage Zero can wait for Unreal's visible pipeline-cache precompile work and report remaining PSOs, but it cannot guarantee every runtime PSO before gameplay. First-use PSOs can still be discovered by actual rendering, especially on D3D12/Vulkan projects without a collected bundled PSO cache.")))
                        .WrapTextAt(720.f)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(FMargin(0, 4, 0, 0))
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("For production builds, use Stage Zero together with Unreal's PSO precaching or bundled PSO workflow, then verify packaged logs for remaining precompiles and first-run hitches.")))
                        .WrapTextAt(720.f)
                        .ColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.82f, 0.92f)))
                    ]
                ]
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(FMargin(0, 6))
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(FMargin(0, 0, 8, 0))
                [
                    SNew(SButton)
                    .ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton"))
                    .IsEnabled(this, &SStageZeroManifestTab::IsGenerateEnabled)
                    .ContentPadding(FMargin(10.f, 4.f))
                    .Text(FText::FromString(TEXT("Build Manifest")))
                    .ToolTipText(FText::FromString(TEXT("Recommended. Builds the Stage Zero manifest in a separate process using the options above for reliable, repeatable results.")))
                    .OnClicked(this, &SStageZeroManifestTab::HandleGenerateClicked)
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(FMargin(0, 0, 8, 0))
                [
                    SNew(SButton)
                    .ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("Button"))
                    .IsEnabled(this, &SStageZeroManifestTab::IsGenerateEnabled)
                    .ContentPadding(FMargin(10.f, 4.f))
                    .Text(FText::FromString(TEXT("Build Manifest In Editor")))
                    .ToolTipText(FText::FromString(TEXT("Advanced. Builds the manifest inside the current editor session using the same options shown above. Use this for debugging or comparison against the recommended build.")))
                    .OnClicked(this, &SStageZeroManifestTab::HandleGenerateWithWPClicked)
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(FMargin(0, 0, 8, 0))
                [
                    SNew(SButton)
                    .ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("Button"))
                    .ContentPadding(FMargin(8.f, 4.f))
                    .Text(FText::FromString(TEXT("Open Asset")))
                    .ToolTipText(FText::FromString(TEXT("Select the manifest in the Content Browser")))
                    .OnClicked(this, &SStageZeroManifestTab::HandleOpenManifestAsset)
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(FMargin(0, 0, 8, 0))
                [
                    SNew(SButton)
                    .ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("Button"))
                    .ContentPadding(FMargin(8.f, 4.f))
                    .Text(FText::FromString(TEXT("View Log")))
                    .ToolTipText(FText::FromString(TEXT("Open the latest StageZeroBuildManifest.log in your default text editor.")))
                    .OnClicked(this, &SStageZeroManifestTab::HandleOpenManifestLog)
                ]
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(FMargin(0, 6))
            [
                SNew(SSeparator)
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("Status")))
                .Font(SubTitleFont)
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(FMargin(0, 4))
            [
                SAssignNew(StatusText, STextBlock)
                .Text(FText::FromString(TEXT("Ready.")))
                .WrapTextAt(600.f)
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(FMargin(0, 4))
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                [
                    SAssignNew(BuildProgressBar, SProgressBar)
                    .Percent_Lambda([this]() -> TOptional<float>
                    {
                        return bBuildInProgress && !bProgressIndeterminate ? TOptional<float>(BuildProgressValue) : TOptional<float>();
                    })
                    .FillColorAndOpacity(FLinearColor(0.1f, 0.6f, 1.0f))
                    .Visibility_Lambda([this]() { return bBuildInProgress ? EVisibility::Visible : EVisibility::Collapsed; })
                    .ToolTipText(FText::FromString(TEXT("Progress of the external commandlet or in-editor manifest build.")))
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(FMargin(8, 0, 0, 0))
                .VAlign(VAlign_Center)
                [
                    SAssignNew(BuildProgressText, STextBlock)
                    .Text(FText::FromString(TEXT("")))
                    .Visibility_Lambda([this]() { return bBuildInProgress ? EVisibility::Visible : EVisibility::Collapsed; })
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(FMargin(8, 0, 0, 0))
                .VAlign(VAlign_Center)
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("Cancel Build")))
                    .ToolTipText(FText::FromString(TEXT("Terminate the active external manifest commandlet. The last committed manifest remains intact.")))
                    .Visibility_Lambda([this]() { return ManifestProcHandle.IsValid() ? EVisibility::Visible : EVisibility::Collapsed; })
                    .OnClicked(this, &SStageZeroManifestTab::HandleCancelManifestBuild)
                ]
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(FMargin(0, 12, 0, 4))
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("Manifest Totals")))
                .Font(SubTitleFont)
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SAssignNew(StatsText, STextBlock)
                .Text(FText::FromString(TEXT("Maps: -, Materials: -, Textures: -, Static Meshes: -, Skeletal Meshes: -, Niagara: -")))
                .WrapTextAt(600.f)
                .Font(StatFont)
            ]
        ]
    ];

    RefreshStats();
}

FReply SStageZeroManifestTab::HandleGenerateClicked()
{
    SyncSelectionToOptions();
    if (!BuildOptions.bUseAllMaps && !HasAnySelectedMap())
    {
        SetStatus(TEXT("Select at least one map before generating."), FSlateColor(FLinearColor::Red));
        return FReply::Handled();
    }

    SaveOptionsToConfig();

    if (!ConfirmCommandletManifestBuild())
    {
        return FReply::Handled();
    }

    if (StartManifestBuildProcess())
    {
        SetStatus(TEXT("Manifest build started in external process..."));
    }
    else
    {
        SetStatus(
            bManifestBuildLaunchCancelled ? TEXT("Manifest build canceled; the existing manifest was left unchanged.") : TEXT("Failed to launch manifest commandlet. Check log for details."),
            bManifestBuildLaunchCancelled ? FSlateColor(FLinearColor::Yellow) : FSlateColor(FLinearColor::Red));
    }
    return FReply::Handled();
}

FReply SStageZeroManifestTab::HandleGenerateWithWPClicked()
{
    SyncSelectionToOptions();
    if (!BuildOptions.bUseAllMaps && !HasAnySelectedMap())
    {
        SetStatus(TEXT("Select at least one map before generating."), FSlateColor(FLinearColor::Red));
        return FReply::Handled();
    }

    SaveOptionsToConfig();

    if (!ConfirmInEditorManifestBuild())
    {
        return FReply::Handled();
    }

    bool bUserCancelled = false;
    if (!StageZeroSaveDirtySourcePackages(bUserCancelled)
        || !StageZeroPrepareManifestAsset(false, bUserCancelled))
    {
        SetStatus(
            bUserCancelled ? TEXT("Manifest build canceled; unsaved work was left unchanged.") : TEXT("Manifest build could not prepare saved source data. See the Output Log."),
            bUserCancelled ? FSlateColor(FLinearColor::Yellow) : FSlateColor(FLinearColor::Red));
        return FReply::Handled();
    }

#if WITH_EDITOR
    BeginProgress(TEXT("Building manifest in current editor session..."), true);

    FStageZeroManifestBuildOptions LocalOptions = BuildOptions;

    FStageZeroManifestBuilder::SetProgressCallback([this](const FStageZeroManifestProgress& Progress)
    {
        const float Fraction = Progress.MapTotal > 0 ? static_cast<float>(Progress.MapIndex) / static_cast<float>(Progress.MapTotal) : 0.f;
        const FString Label = FString::Printf(
            TEXT("Map %s/%s: %s | M=%s T=%s N=%s SM=%s SK=%s"),
            *StageZeroFormatCount(Progress.MapIndex),
            *StageZeroFormatCount(Progress.MapTotal),
            *Progress.MapPackage.ToString(),
            *StageZeroFormatCount(Progress.Materials),
            *StageZeroFormatCount(Progress.Textures),
            *StageZeroFormatCount(Progress.Niagara),
            *StageZeroFormatCount(Progress.StaticMeshes),
            *StageZeroFormatCount(Progress.SkeletalMeshes));
        UpdateProgress(Fraction, Label);
    });
    ON_SCOPE_EXIT
    {
        FStageZeroManifestBuilder::ClearProgressCallback();
        EndProgress();
    };

    FStageZeroManifestBuildResult BuildResult;
    if (UStageZeroWarmupManifest* Manifest = FStageZeroManifestBuilder::BuildManifestWithOptions(LocalOptions, &BuildResult))
    {
        RefreshStats();
        SetStatus(StageZeroFormatBuildStatus(BuildResult), StageZeroBuildStatusColor(BuildResult));
    }
    else
    {
        SetStatus(StageZeroFormatBuildStatus(BuildResult), StageZeroBuildStatusColor(BuildResult));
    }

#endif

    return FReply::Handled();
}

FReply SStageZeroManifestTab::HandleOpenManifestAsset()
{
#if WITH_EDITOR
    if (GEditor)
    {
        if (UObject* Manifest = LoadObject<UObject>(nullptr, FStageZeroWarmupUtils::GetManifestObjectPath()))
        {
            TArray<UObject*> Assets;
            Assets.Add(Manifest);
            FContentBrowserModule& CBModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
            CBModule.Get().SyncBrowserToAssets(Assets);
            SetStatus(TEXT("Selected manifest asset in Content Browser."));
        }
        else
        {
            SetStatus(TEXT("Manifest asset not found. Generate it first."), FSlateColor(FLinearColor::Red));
        }
    }
#endif
    return FReply::Handled();
}

FReply SStageZeroManifestTab::HandleOpenManifestLog()
{
#if WITH_EDITOR
    const FString ManifestLogFile = !ManifestLogPath.IsEmpty() ? ManifestLogPath : StageZeroFindLatestManifestLog();
    if (FPaths::FileExists(ManifestLogFile))
    {
        FPlatformProcess::LaunchFileInDefaultExternalApplication(*ManifestLogFile);
        SetStatus(FString::Printf(TEXT("Opened manifest log: %s"), *ManifestLogFile));
    }
    else
    {
        SetStatus(FString::Printf(TEXT("Manifest log not found at %s"), *ManifestLogFile), FSlateColor(FLinearColor::Red));
    }
#endif
    return FReply::Handled();
}

FReply SStageZeroManifestTab::HandleCancelManifestBuild()
{
    if (!ManifestProcHandle.IsValid())
    {
        return FReply::Handled();
    }

    if (FMessageDialog::Open(
        EAppMsgType::YesNo,
        FText::FromString(TEXT("Cancel the active Stage Zero manifest build?\n\nThe external commandlet will be terminated. The last committed manifest will remain available.")),
        FText::FromString(TEXT("Cancel Stage Zero Manifest Build"))) == EAppReturnType::Yes)
    {
        TerminateManifestProcess();
        SetStatus(TEXT("Manifest build canceled. The last committed manifest was preserved."), FSlateColor(FLinearColor::Yellow));
        ShowNotification(TEXT("Manifest build canceled."), SNotificationItem::CS_None);
    }
    return FReply::Handled();
}

FReply SStageZeroManifestTab::HandleOpenRatePlugin()
{
    return StageZeroOpenUrl(StageZeroFabListingUrl);
}

FReply SStageZeroManifestTab::HandleOpenDiscord()
{
    return StageZeroOpenUrl(StageZeroDiscordUrl);
}

FReply SStageZeroManifestTab::HandleOpenRoadmap()
{
    return StageZeroOpenUrl(StageZeroRoadmapUrl);
}

FReply SStageZeroManifestTab::HandleOpenWalkthrough()
{
    return StageZeroOpenUrl(StageZeroWalkthroughUrl);
}

FReply SStageZeroManifestTab::HandleOpenSupportEmail()
{
    return StageZeroOpenUrl(StageZeroSupportEmailUrl);
}

bool SStageZeroManifestTab::StartManifestBuildProcess()
{
#if WITH_EDITOR
    bManifestBuildLaunchCancelled = false;
    StopManifestPoll();
    TerminateManifestProcess();

    const FString EditorCmdExe = FUnrealEdMisc::Get().GetExecutableForCommandlets();
    if (EditorCmdExe.IsEmpty() || !FPaths::FileExists(EditorCmdExe))
    {
        UE_LOG(LogStageZeroManifestTab, Error, TEXT("StageZero: Could not locate UnrealEditor-Cmd executable at %s"), *EditorCmdExe);
        return false;
    }

    const FString ProjectPath = FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath());
    const FString ManifestLogFile = FPaths::ConvertRelativePathToFull(
        FPaths::ProjectLogDir() / FString::Printf(
            TEXT("StageZeroBuildManifest-%s-P%u-%s.log"),
            *FDateTime::UtcNow().ToString(TEXT("%Y%m%d-%H%M%S")),
            FPlatformProcess::GetCurrentProcessId(),
            *FGuid::NewGuid().ToString(EGuidFormats::Digits)));
    bool bUserCancelled = false;
    if (!StageZeroSaveDirtySourcePackages(bUserCancelled)
        || !StageZeroPrepareManifestAsset(true, bUserCancelled))
    {
        bManifestBuildLaunchCancelled = bUserCancelled;
        return false;
    }

    IFileManager::Get().Delete(*ManifestLogFile, false, true, true);
    ManifestLogReadOffset = 0;
    ManifestLogTail.Reset();

    auto BoolFlag = [](const TCHAR* Name, bool bValue)
    {
        return FString::Printf(TEXT(" -%s=%d"), Name, bValue ? 1 : 0);
    };

    FString SelectedMapsArg;
    if (!BuildOptions.bUseAllMaps && BuildOptions.SelectedMaps.Num() > 0)
    {
        TArray<FString> MapTokens;
        MapTokens.Reserve(BuildOptions.SelectedMaps.Num());
        for (const FSoftObjectPath& Path : BuildOptions.SelectedMaps)
        {
            MapTokens.Add(Path.ToString());
        }
        SelectedMapsArg = FString::Printf(TEXT(" -StageZeroSelectedMaps=\"%s\""), *FString::Join(MapTokens, TEXT(";")));
    }

    FString OverrideArgs;
    FString SerializedValue;
    FString SerializationError;
    if (BuildOptions.GlobalAdditionalAssets.Num() > 0)
    {
        if (!StageZeroManifestOverrideSerialization::EncodePathList(
            BuildOptions.GlobalAdditionalAssets, SerializedValue, SerializationError))
        {
            UE_LOG(LogStageZeroManifestTab, Error,
                TEXT("StageZero: Could not forward global additional assets to the manifest commandlet: %s"),
                *SerializationError);
            return false;
        }
        OverrideArgs += FString::Printf(TEXT(" -StageZeroGlobalAdditionalAssets=\"%s\""), *SerializedValue);
    }
    if (BuildOptions.GlobalExcludedAssets.Num() > 0)
    {
        if (!StageZeroManifestOverrideSerialization::EncodePathList(
            BuildOptions.GlobalExcludedAssets, SerializedValue, SerializationError))
        {
            UE_LOG(LogStageZeroManifestTab, Error,
                TEXT("StageZero: Could not forward global excluded assets to the manifest commandlet: %s"),
                *SerializationError);
            return false;
        }
        OverrideArgs += FString::Printf(TEXT(" -StageZeroGlobalExcludedAssets=\"%s\""), *SerializedValue);
    }
    if (BuildOptions.PerMapAssetOverrides.Num() > 0)
    {
        if (!StageZeroManifestOverrideSerialization::EncodeMapOverrides(
            BuildOptions.PerMapAssetOverrides, SerializedValue, SerializationError))
        {
            UE_LOG(LogStageZeroManifestTab, Error,
                TEXT("StageZero: Could not forward per-map asset overrides to the manifest commandlet: %s"),
                *SerializationError);
            return false;
        }
        OverrideArgs += FString::Printf(TEXT(" -StageZeroPerMapAssetOverrides=\"%s\""), *SerializedValue);
    }

    ManifestLogPath = ManifestLogFile;
    const FString CommandLine = FString::Printf(
        TEXT("\"%s\" -run=StageZeroBuildManifest -unattended -nop4 -nosplash -NoSound -log -abslog=\"%s\"%s%s%s%s%s%s%s%s%s"),
        *ProjectPath,
        *ManifestLogFile,
        *BoolFlag(TEXT("StageZeroUseAllMaps"), BuildOptions.bUseAllMaps),
        *BoolFlag(TEXT("StageZeroIncludeWorldPartition"), BuildOptions.bIncludeWorldPartition),
        *BoolFlag(TEXT("StageZeroIncludeAnchorAndSplineRoutes"), BuildOptions.bIncludeAnchorAndSplineRoutes),
        *BoolFlag(TEXT("StageZeroIncludeTextures"), BuildOptions.bIncludeTextures),
        *BoolFlag(TEXT("StageZeroIncludeAllProjectMaterials"), BuildOptions.bIncludeAllProjectMaterials),
        *BoolFlag(TEXT("StageZeroIncludeAllProjectTextures"), BuildOptions.bIncludeAllProjectTextures),
        *BoolFlag(TEXT("StageZeroIncludeEngineContent"), BuildOptions.bIncludeEngineContent),
        *SelectedMapsArg,
        *OverrideArgs);

    constexpr int32 SafeWindowsCommandLineLength = 30000;
    if (CommandLine.Len() > SafeWindowsCommandLineLength)
    {
        UE_LOG(LogStageZeroManifestTab, Error,
            TEXT("StageZero: Manifest command line is too long (%d characters). Reduce manual overrides or split coverage across maps."),
            CommandLine.Len());
        return false;
    }

    uint32 ProcId = 0;
    ManifestProcHandle = FPlatformProcess::CreateProc(*EditorCmdExe, *CommandLine, true, true, false, &ProcId, 0, nullptr, nullptr);
    if (!ManifestProcHandle.IsValid())
    {
        UE_LOG(LogStageZeroManifestTab, Error, TEXT("StageZero: Failed to launch StageZeroBuildManifest commandlet (%s %s)"), *EditorCmdExe, *CommandLine);
        EndProgress();
        return false;
    }

    BeginProgress(TEXT("Building manifest (commandlet)..."), true);
    ManifestTickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateSP(this, &SStageZeroManifestTab::PollManifestProcess), 1.0f);
    ShowNotification(TEXT("Manifest build started in external commandlet..."), SNotificationItem::CS_Pending);
    return true;
#else
    return false;
#endif
}

bool SStageZeroManifestTab::PollManifestProcess(float /*DeltaSeconds*/)
{
    if (!ManifestProcHandle.IsValid())
    {
        return false;
    }

    UpdateProgressFromLog();

    int32 ReturnCode = 0;
    if (FPlatformProcess::GetProcReturnCode(ManifestProcHandle, &ReturnCode))
    {
        const FString CompletedLogPath = ManifestLogPath;
        FPlatformProcess::CloseProc(ManifestProcHandle);
        ManifestProcHandle.Reset();
        StopManifestPoll();
        EndProgress();

        FStageZeroManifestBuildResult BuildResult;
        bool bParsedSummary = false;
        if (!CompletedLogPath.IsEmpty())
        {
            FString LogContents;
            if (FFileHelper::LoadFileToString(LogContents, *CompletedLogPath))
            {
                bParsedSummary = StageZeroManifestLog::ParseSummary(LogContents, BuildResult);
            }
        }

        const bool bSummarySucceeded = bParsedSummary && BuildResult.bSucceeded && BuildResult.bManifestSaved;
        StageZeroRefreshManifestRegistrationFromDisk();
        if (ReturnCode == 0 && bSummarySucceeded)
        {
            RefreshStats();
            SetStatus(StageZeroFormatBuildStatus(BuildResult), StageZeroBuildStatusColor(BuildResult));
            ShowNotification(TEXT("Manifest build completed successfully."), SNotificationItem::CS_Success);
        }
        else if (!bParsedSummary)
        {
            SetStatus(FString::Printf(TEXT("Manifest build failed contract validation (exit %d): no complete structured summary was found."), ReturnCode), FSlateColor(FLinearColor::Red));
            ShowNotification(TEXT("Manifest build failed validation. See log for details."), SNotificationItem::CS_Fail);
        }
        else
        {
            const FString ExitSuffix = ReturnCode == 0 ? FString() : FString::Printf(TEXT(" Commandlet exit code=%d."), ReturnCode);
            SetStatus(StageZeroFormatBuildStatus(BuildResult) + ExitSuffix, StageZeroBuildStatusColor(BuildResult));
            ShowNotification(TEXT("Manifest build failed. See log for details."), SNotificationItem::CS_Fail);
        }
        return false;
    }

    return true;
}

void SStageZeroManifestTab::StopManifestPoll()
{
    if (ManifestTickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(ManifestTickerHandle);
        ManifestTickerHandle.Reset();
    }
}

void SStageZeroManifestTab::TerminateManifestProcess()
{
    if (!ManifestProcHandle.IsValid())
    {
        return;
    }

    int32 ReturnCode = 0;
    if (!FPlatformProcess::GetProcReturnCode(ManifestProcHandle, &ReturnCode))
    {
        UE_LOG(LogStageZeroManifestTab, Warning, TEXT("StageZero: Terminating active manifest commandlet before closing process handle."));
        FPlatformProcess::TerminateProc(ManifestProcHandle, true);
    }

    FPlatformProcess::CloseProc(ManifestProcHandle);
    ManifestProcHandle.Reset();
    StageZeroRefreshManifestRegistrationFromDisk();
    bBuildInProgress = false;
    EndProgress();
}

void SStageZeroManifestTab::ShowNotification(const FString& Message, SNotificationItem::ECompletionState State)
{
    FNotificationInfo Info(FText::FromString(Message));
    Info.bFireAndForget = true;
    Info.bUseSuccessFailIcons = true;
    Info.ExpireDuration = 4.0f;
    Info.FadeOutDuration = 0.5f;

    if (TSharedPtr<SNotificationItem> Item = FSlateNotificationManager::Get().AddNotification(Info))
    {
        Item->SetCompletionState(State);
    }
}

void SStageZeroManifestTab::RefreshStats()
{
#if WITH_EDITOR
    if (UStageZeroWarmupManifest* Manifest = LoadObject<UStageZeroWarmupManifest>(nullptr, FStageZeroWarmupUtils::GetManifestObjectPath()))
    {
        const FString StatsString = FString::Printf(
            TEXT("Maps: %s | Materials: %s | Textures: %s | Static Meshes: %s | Skeletal Meshes: %s | Niagara: %s | WP Cells: %s | WP Anchors: %s | WP Splines: %s"),
            *StageZeroFormatCount(Manifest->MapEntries.Num()),
            *StageZeroFormatCount(Manifest->AllMaterials.Num()),
            *StageZeroFormatCount(Manifest->AllTextures.Num()),
            *StageZeroFormatCount(Manifest->AllStaticMeshes.Num()),
            *StageZeroFormatCount(Manifest->AllSkeletalMeshes.Num()),
            *StageZeroFormatCount(Manifest->AllNiagaraSystems.Num()),
            *StageZeroFormatCount(Manifest->WPCells.Num()),
            *StageZeroFormatCount(Manifest->WPAnchorGroups.Num()),
            *StageZeroFormatCount(Manifest->WPSplineGroups.Num()));

        SetStats(StatsString);
        if (Manifest->MapEntries.Num() > 0)
        {
            SetStatus(TEXT("Manifest loaded."));
            ShowMissingSelectionWarnings(Manifest);
        }
        else
        {
            SetStatus(TEXT("Manifest exists but is empty/stale. Rebuild required."), FSlateColor(FLinearColor::Yellow));
        }
    }
    else
    {
        SetStats(TEXT("Maps: -, Materials: -, Textures: -, Static Meshes: -, Skeletal Meshes: -, Niagara: -, WP Cells: -, WP Anchors: -, WP Splines: -"));
        SetStatus(TEXT("Manifest not found. Generate it to view stats."), FSlateColor(FLinearColor::Red));
    }
#else
    SetStatus(TEXT("Manifest generation is editor-only."), FSlateColor(FLinearColor::Red));
#endif
}

void SStageZeroManifestTab::SetStatus(const FString& Message, const FSlateColor& Color)
{
    if (StatusText.IsValid())
    {
        StatusText->SetText(FText::FromString(Message));
        StatusText->SetColorAndOpacity(Color);
    }
}

void SStageZeroManifestTab::ShowMissingSelectionWarnings(const UStageZeroWarmupManifest* Manifest)
{
#if WITH_EDITOR
    if (!Manifest || BuildOptions.bUseAllMaps)
    {
        return;
    }

    TSet<FName> ManifestPackages;
    for (const FStageZeroMapMatTexEntry& Entry : Manifest->MapEntries)
    {
        ManifestPackages.Add(Entry.MapPackageName);
    }

    TArray<FString> MissingPackages;
    for (const FSoftObjectPath& Path : BuildOptions.SelectedMaps)
    {
        const FString LongName = Path.GetLongPackageName();
        if (LongName.IsEmpty())
        {
            continue;
        }

        if (!ManifestPackages.Contains(FName(*LongName)))
        {
            MissingPackages.Add(LongName);
        }
    }

    if (MissingPackages.Num() == 0)
    {
        return;
    }

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    TArray<FString> EngineFiltered;
    TArray<FString> NotFound;
    for (const FString& Package : MissingPackages)
    {
        if (Package.StartsWith(TEXT("/Engine/")) && !BuildOptions.bIncludeEngineContent)
        {
            EngineFiltered.Add(Package);
            continue;
        }

        const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *Package, *FPackageName::GetShortName(Package));
        if (!StageZeroAssetRegistryCompat::GetAssetByObjectPath(AssetRegistry, FSoftObjectPath(ObjectPath)).IsValid())
        {
            NotFound.Add(Package);
        }
    }

    FString Message = FString::Printf(TEXT("Manifest built but missing %d selected map(s)."), MissingPackages.Num());
    TArray<FString> Reasons;
    if (NotFound.Num() > 0)
    {
        Reasons.Add(FString::Printf(TEXT("Not found in Asset Registry: %s"), *FString::Join(NotFound, TEXT(", "))));
    }
    if (EngineFiltered.Num() > 0)
    {
        Reasons.Add(FString::Printf(TEXT("Engine maps skipped (enable 'Include Engine Content'): %s"), *FString::Join(EngineFiltered, TEXT(", "))));
    }
    if (Reasons.Num() > 0)
    {
        Message.Append(TEXT(" "));
        Message.Append(FString::Join(Reasons, TEXT(" | ")));
    }

    SetStatus(Message, FSlateColor(FLinearColor::Yellow));
#endif
}

void SStageZeroManifestTab::BeginProgress(const FString& Label, bool bIndeterminate /*= true*/)
{
    bBuildInProgress = true;
    bProgressIndeterminate = bIndeterminate;
    BuildProgressValue = 0.f;
    BuildProgressLabel = Label;
    if (BuildProgressText.IsValid())
    {
        BuildProgressText->SetText(FText::FromString(Label));
    }
}

void SStageZeroManifestTab::UpdateProgress(float Value, const FString& Label)
{
    bBuildInProgress = true;
    bProgressIndeterminate = false;
    BuildProgressValue = FMath::Clamp(Value, 0.f, 1.f);
    BuildProgressLabel = Label;
    if (BuildProgressText.IsValid())
    {
        BuildProgressText->SetText(FText::FromString(Label));
    }
}

void SStageZeroManifestTab::EndProgress()
{
    bBuildInProgress = false;
    bProgressIndeterminate = false;
    BuildProgressValue = 0.f;
    BuildProgressLabel.Reset();
    ManifestLogPath.Reset();
    ManifestLogTail.Reset();
    ManifestLogReadOffset = 0;
    if (BuildProgressText.IsValid())
    {
        BuildProgressText->SetText(FText::GetEmpty());
    }
}

void SStageZeroManifestTab::UpdateProgressFromLog()
{
    if (ManifestLogPath.IsEmpty())
    {
        return;
    }

    TUniquePtr<FArchive> LogReader(IFileManager::Get().CreateFileReader(*ManifestLogPath, FILEREAD_AllowWrite));
    if (!LogReader)
    {
        return;
    }

    const int64 LogSize = LogReader->TotalSize();
    if (LogSize < ManifestLogReadOffset)
    {
        ManifestLogReadOffset = 0;
        ManifestLogTail.Reset();
    }

    const int64 AvailableBytes = LogSize - ManifestLogReadOffset;
    if (AvailableBytes <= 0)
    {
        return;
    }

    constexpr int64 MaxBytesPerPoll = 4ll * 1024ll * 1024ll;
    const int32 BytesToRead = static_cast<int32>(FMath::Min(AvailableBytes, MaxBytesPerPoll));
    TArray<uint8> NewBytes;
    NewBytes.SetNumUninitialized(BytesToRead);
    LogReader->Seek(ManifestLogReadOffset);
    LogReader->Serialize(NewBytes.GetData(), BytesToRead);
    if (LogReader->IsError())
    {
        return;
    }
    ManifestLogReadOffset += BytesToRead;

    const FUTF8ToTCHAR Converted(reinterpret_cast<const ANSICHAR*>(NewBytes.GetData()), NewBytes.Num());
    ManifestLogTail.AppendChars(Converted.Get(), Converted.Length());
    constexpr int32 MaxTailCharacters = 128 * 1024;
    if (ManifestLogTail.Len() > MaxTailCharacters)
    {
        ManifestLogTail = ManifestLogTail.Right(MaxTailCharacters);
    }

    const FString Prefix(TEXT("StageZeroBuildManifestProgress:"));
    int32 FoundIndex = ManifestLogTail.Find(Prefix, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
    if (FoundIndex == INDEX_NONE)
    {
        return;
    }

    FString Line = ManifestLogTail.Mid(FoundIndex);
    int32 NewlineIndex;
    if (Line.FindChar(TEXT('\n'), NewlineIndex))
    {
        Line = Line.Left(NewlineIndex);
    }

    const FString AfterPrefix = Line.Mid(Prefix.Len()).TrimStartAndEnd();
    TArray<FString> Tokens;
    AfterPrefix.ParseIntoArrayWS(Tokens);
    if (Tokens.Num() < 1)
    {
        return;
    }

    int32 MapIndex = 0;
    int32 MapTotal = 0;
    FString MapName;
    int32 Materials = 0;
    int32 Textures = 0;
    int32 Niagara = 0;
    int32 StaticMeshes = 0;
    int32 SkeletalMeshes = 0;

    // Token 0 should be "X/Y"
    {
        TArray<FString> Pair;
        Tokens[0].ParseIntoArray(Pair, TEXT("/"));
        if (Pair.Num() == 2)
        {
            MapIndex = FCString::Atoi(*Pair[0]);
            MapTotal = FCString::Atoi(*Pair[1]);
        }
    }

    for (int32 i = 1; i < Tokens.Num(); ++i)
    {
        const FString& Token = Tokens[i];
        if (Token.StartsWith(TEXT("Map=")))
        {
            MapName = Token.Mid(4);
        }
        else if (Token.StartsWith(TEXT("Materials=")))
        {
            Materials = FCString::Atoi(*Token.Mid(10));
        }
        else if (Token.StartsWith(TEXT("Textures=")))
        {
            Textures = FCString::Atoi(*Token.Mid(9));
        }
        else if (Token.StartsWith(TEXT("Niagara=")))
        {
            Niagara = FCString::Atoi(*Token.Mid(8));
        }
        else if (Token.StartsWith(TEXT("StaticMeshes=")))
        {
            StaticMeshes = FCString::Atoi(*Token.Mid(13));
        }
        else if (Token.StartsWith(TEXT("SkeletalMeshes=")))
        {
            SkeletalMeshes = FCString::Atoi(*Token.Mid(15));
        }
    }

    if (MapTotal <= 0)
    {
        return;
    }

    const float Fraction = FMath::Clamp(static_cast<float>(MapIndex) / static_cast<float>(MapTotal), 0.f, 1.f);
    const FString Label = FString::Printf(
        TEXT("Map %s/%s: %s | Mat=%s Tex=%s Niag=%s SM=%s SK=%s"),
        *StageZeroFormatCount(MapIndex),
        *StageZeroFormatCount(MapTotal),
        *MapName,
        *StageZeroFormatCount(Materials),
        *StageZeroFormatCount(Textures),
        *StageZeroFormatCount(Niagara),
        *StageZeroFormatCount(StaticMeshes),
        *StageZeroFormatCount(SkeletalMeshes));
    UpdateProgress(Fraction, Label);
}

void SStageZeroManifestTab::SetStats(const FString& Message)
{
    if (StatsText.IsValid())
    {
        StatsText->SetText(FText::FromString(Message));
    }
}

void SStageZeroManifestTab::LoadOptionsFromConfig()
{
#if WITH_EDITOR
    BuildOptions = FStageZeroManifestBuilder::LoadManifestBuildOptions();
#endif
}

void SStageZeroManifestTab::SaveOptionsToConfig()
{
#if WITH_EDITOR
    FStageZeroManifestBuilder::SaveManifestBuildOptions(BuildOptions);
#endif
}

void SStageZeroManifestTab::SyncManualOverridesEditorFromOptions()
{
    if (!ManualOverridesEditorObject.IsValid())
    {
        return;
    }

    ManualOverridesEditorObject->GlobalAdditionalAssets = BuildOptions.GlobalAdditionalAssets;
    ManualOverridesEditorObject->GlobalExcludedAssets = BuildOptions.GlobalExcludedAssets;
    ManualOverridesEditorObject->PerMapAssetOverrides = BuildOptions.PerMapAssetOverrides;
}

void SStageZeroManifestTab::HandleManualOverridesChanged(const FPropertyChangedEvent& PropertyChangedEvent)
{
    (void)PropertyChangedEvent;
    if (!ManualOverridesEditorObject.IsValid())
    {
        return;
    }

    BuildOptions.GlobalAdditionalAssets = ManualOverridesEditorObject->GlobalAdditionalAssets;
    BuildOptions.GlobalExcludedAssets = ManualOverridesEditorObject->GlobalExcludedAssets;
    BuildOptions.PerMapAssetOverrides = ManualOverridesEditorObject->PerMapAssetOverrides;
    SaveOptionsToConfig();
}

bool SStageZeroManifestTab::IsWorldPartitionAsset(const FAssetData& AssetData) const
{
    static const FName WorldPartitionSetupTag(TEXT("WorldPartitionSetup"));
    static const FName EnableWorldPartitionTag(TEXT("bEnableWorldPartition"));

    FString WPSetup;
    if (AssetData.GetTagValue(WorldPartitionSetupTag, WPSetup) && !WPSetup.IsEmpty())
    {
        return true;
    }

    bool bIsWP = false;
    AssetData.GetTagValue(EnableWorldPartitionTag, bIsWP);
    return bIsWP;
}

void SStageZeroManifestTab::GatherWorldAssets()
{
#if WITH_EDITOR
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    AssetRegistryModule.Get().SearchAllAssets(true);
    AssetRegistryModule.Get().WaitForCompletion();

    TArray<FName> WorldRoots;
    WorldRoots.Add(FName(TEXT("/Game")));
    for (const TSharedRef<IPlugin>& Plugin : IPluginManager::Get().GetEnabledPluginsWithContent())
    {
        if (Plugin->GetName().Equals(TEXT("StageZero"), ESearchCase::IgnoreCase))
        {
            continue;
        }

        const EPluginType PluginType = Plugin->GetType();
        const bool bProjectContentPlugin = PluginType == EPluginType::Project
            || PluginType == EPluginType::External
            || PluginType == EPluginType::Mod;
        if (!bProjectContentPlugin && !BuildOptions.bIncludeEngineContent)
        {
            continue;
        }

        FString MountedAssetPath = Plugin->GetMountedAssetPath();
        MountedAssetPath.RemoveFromEnd(TEXT("/"));
        if (MountedAssetPath.StartsWith(TEXT("/")) && MountedAssetPath.Len() > 1)
        {
            WorldRoots.AddUnique(FName(*MountedAssetPath));
        }
    }
    if (BuildOptions.bIncludeEngineContent)
    {
        WorldRoots.AddUnique(FName(TEXT("/Engine")));
    }
    WorldRoots.Sort(FNameLexicalLess());

    FARFilter WorldFilter;
    WorldFilter.PackagePaths = WorldRoots;
    WorldFilter.bRecursivePaths = true;
    StageZeroAssetRegistryCompat::AddClassToFilter(WorldFilter, UWorld::StaticClass());
    WorldFilter.bRecursiveClasses = true;

    TArray<FAssetData> WorldAssets;
    StageZeroAssetRegistryCompat::GetAssets(AssetRegistryModule.Get(), WorldFilter, WorldAssets, true);
    WorldAssets.RemoveAll([](const FAssetData& Asset)
    {
        return Asset.IsRedirector() || (Asset.PackageFlags & PKG_EditorOnly) != 0;
    });
    WorldAssets.Sort([](const FAssetData& Left, const FAssetData& Right)
    {
        return Left.PackageName.LexicalLess(Right.PackageName);
    });

    TSet<FName> SelectedNames;
    for (const FSoftObjectPath& Path : BuildOptions.SelectedMaps)
    {
        const FString LongName = Path.GetLongPackageName();
        if (!LongName.IsEmpty())
        {
            SelectedNames.Add(*LongName);
        }
    }

    MapEntries.Reset();
    for (const FAssetData& Asset : WorldAssets)
    {
        TSharedPtr<FStageZeroMapListEntry> Entry = MakeShared<FStageZeroMapListEntry>();
        Entry->AssetData = Asset;
        Entry->bSelected = SelectedNames.Contains(Asset.PackageName);
        Entry->bIsWorldPartition = IsWorldPartitionAsset(Asset);
        MapEntries.Add(Entry);
    }

    if (MapListView.IsValid())
    {
        MapListView->RequestListRefresh();
    }
#endif
}

void SStageZeroManifestTab::SyncSelectionToOptions(bool bPreserveHiddenSelections)
{
    if (MapEntries.Num() == 0)
    {
        return;
    }

    const TArray<FSoftObjectPath> PreviousSelection = BuildOptions.SelectedMaps;
    BuildOptions.SelectedMaps.Reset();
    for (const TSharedPtr<FStageZeroMapListEntry>& Entry : MapEntries)
    {
        if (Entry.IsValid() && Entry->bSelected)
        {
            BuildOptions.SelectedMaps.Add(Entry->AssetData.ToSoftObjectPath());
        }
    }

    if (!bPreserveHiddenSelections)
    {
        return;
    }

    TSet<FString> VisibleMapPackages;
    for (const TSharedPtr<FStageZeroMapListEntry>& Entry : MapEntries)
    {
        if (Entry.IsValid())
        {
            VisibleMapPackages.Add(Entry->AssetData.PackageName.ToString());
        }
    }

    for (const FSoftObjectPath& Path : PreviousSelection)
    {
        const FString LongName = Path.GetLongPackageName();
        if (!LongName.IsEmpty() && !VisibleMapPackages.Contains(LongName) && !BuildOptions.SelectedMaps.Contains(Path))
        {
            BuildOptions.SelectedMaps.Add(Path);
        }
    }
}

bool SStageZeroManifestTab::HasAnySelectedMap() const
{
    if (BuildOptions.SelectedMaps.Num() > 0)
    {
        return true;
    }

    for (const TSharedPtr<FStageZeroMapListEntry>& Entry : MapEntries)
    {
        if (Entry.IsValid() && Entry->bSelected)
        {
            return true;
        }
    }
    return false;
}

void SStageZeroManifestTab::SetAllMapSelections(bool bSelected)
{
    for (TSharedPtr<FStageZeroMapListEntry>& Entry : MapEntries)
    {
        if (Entry.IsValid())
        {
            Entry->bSelected = bSelected;
        }
    }

    if (MapListView.IsValid())
    {
        MapListView->RequestListRefresh();
    }

    SyncSelectionToOptions(bSelected);
    SaveOptionsToConfig();
}

void SStageZeroManifestTab::SelectWorldPartitionMapsOnly()
{
    for (TSharedPtr<FStageZeroMapListEntry>& Entry : MapEntries)
    {
        if (Entry.IsValid())
        {
            Entry->bSelected = Entry->bIsWorldPartition;
        }
    }

    if (MapListView.IsValid())
    {
        MapListView->RequestListRefresh();
    }

    SyncSelectionToOptions(false);
    SaveOptionsToConfig();
}

EVisibility SStageZeroManifestTab::GetMapSelectionVisibility() const
{
    return BuildOptions.bUseAllMaps ? EVisibility::Collapsed : EVisibility::Visible;
}

ECheckBoxState SStageZeroManifestTab::GetUseAllMapsState() const
{
    return BuildOptions.bUseAllMaps ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

ECheckBoxState SStageZeroManifestTab::GetUseSelectedMapsState() const
{
    return BuildOptions.bUseAllMaps ? ECheckBoxState::Unchecked : ECheckBoxState::Checked;
}

void SStageZeroManifestTab::OnUseAllMapsChanged(ECheckBoxState NewState)
{
    if (NewState == ECheckBoxState::Checked)
    {
        BuildOptions.bUseAllMaps = true;
        if (MapListView.IsValid())
        {
            MapListView->RequestListRefresh();
        }
        SyncSelectionToOptions();
        SaveOptionsToConfig();
    }
}

void SStageZeroManifestTab::OnUseSelectedMapsChanged(ECheckBoxState NewState)
{
    if (NewState == ECheckBoxState::Checked)
    {
        BuildOptions.bUseAllMaps = false;
        if (MapListView.IsValid())
        {
            MapListView->RequestListRefresh();
        }
        SyncSelectionToOptions();
        SaveOptionsToConfig();
    }
}

ECheckBoxState SStageZeroManifestTab::GetIncludeEngineContentState() const
{
    return BuildOptions.bIncludeEngineContent ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SStageZeroManifestTab::OnIncludeEngineContentChanged(ECheckBoxState NewState)
{
    SyncSelectionToOptions();
    BuildOptions.bIncludeEngineContent = (NewState == ECheckBoxState::Checked);
    SaveOptionsToConfig();
    GatherWorldAssets();
}

ECheckBoxState SStageZeroManifestTab::GetMapCheckedState(TSharedPtr<FStageZeroMapListEntry> Entry) const
{
    if (Entry.IsValid() && Entry->bSelected)
    {
        return ECheckBoxState::Checked;
    }
    return ECheckBoxState::Unchecked;
}

void SStageZeroManifestTab::OnMapCheckStateChanged(ECheckBoxState NewState, TSharedPtr<FStageZeroMapListEntry> Entry)
{
    if (Entry.IsValid())
    {
        Entry->bSelected = (NewState == ECheckBoxState::Checked);
        SyncSelectionToOptions();
        SaveOptionsToConfig();
    }
}

TSharedRef<ITableRow> SStageZeroManifestTab::MakeMapRowWidget(TSharedPtr<FStageZeroMapListEntry> Entry, const TSharedRef<STableViewBase>& OwnerTable)
{
    const FString DisplayName = Entry.IsValid()
        ? FString::Printf(TEXT("%s %s"), *Entry->AssetData.AssetName.ToString(), Entry->bIsWorldPartition ? TEXT("(World Partition)") : TEXT(""))
        : TEXT("");

    return SNew(STableRow<TSharedPtr<FStageZeroMapListEntry>>, OwnerTable)
    [
        SNew(SCheckBox)
        .IsEnabled_Lambda([this]() { return !BuildOptions.bUseAllMaps; })
        .IsChecked(this, &SStageZeroManifestTab::GetMapCheckedState, Entry)
        .OnCheckStateChanged(this, &SStageZeroManifestTab::OnMapCheckStateChanged, Entry)
        [
            SNew(STextBlock)
            .Text(FText::FromString(DisplayName))
            .ToolTipText(FText::FromString(Entry.IsValid() ? Entry->AssetData.PackageName.ToString() : TEXT("")))
        ]
    ];
}

bool SStageZeroManifestTab::IsGenerateEnabled() const
{
    return !ManifestProcHandle.IsValid();
}

bool SStageZeroManifestTab::ConfirmCommandletManifestBuild() const
{
    const FText Message = FText::FromString(TEXT("Stage Zero will build the manifest in an external Unreal commandlet.\n\nDue to Unreal Engine editor/commandlet loading differences, commandlet manifest generation can sometimes report lower totals than in-editor generation for the same maps. This does not mean your selection was lost; it means some editor-only discovery paths are not available to the commandlet.\n\nContinue with commandlet generation?"));
    return FMessageDialog::Open(EAppMsgType::OkCancel, Message) == EAppReturnType::Ok;
}

bool SStageZeroManifestTab::ConfirmInEditorManifestBuild() const
{
    const FText Message = FText::FromString(TEXT("Stage Zero will generate the manifest inside the current editor session.\n\nThis path can temporarily freeze or appear to hang Unreal while maps, Blueprint defaults, components, textures, materials, Niagara systems, and World Partition data are inspected. Save your work before continuing, especially on large projects.\n\nContinue with in-editor generation?"));
    return FMessageDialog::Open(EAppMsgType::OkCancel, Message) == EAppReturnType::Ok;
}

FString SStageZeroManifestTab::GetSelectedMapSummary() const
{
    if (BuildOptions.bUseAllMaps)
    {
        return FString::Printf(TEXT("Mode: All maps | Available maps: %s"), *StageZeroFormatCount(MapEntries.Num()));
    }

    int32 VisibleSelected = 0;
    for (const TSharedPtr<FStageZeroMapListEntry>& Entry : MapEntries)
    {
        if (Entry.IsValid() && Entry->bSelected)
        {
            ++VisibleSelected;
        }
    }

    const int32 PreservedHidden = FMath::Max(0, BuildOptions.SelectedMaps.Num() - VisibleSelected);
    if (PreservedHidden > 0)
    {
        return FString::Printf(
            TEXT("Mode: Selected maps | Selected: %s visible, %s preserved from saved paths"),
            *StageZeroFormatCount(VisibleSelected),
            *StageZeroFormatCount(PreservedHidden));
    }

    return FString::Printf(TEXT("Mode: Selected maps | Selected: %s"), *StageZeroFormatCount(VisibleSelected));
}
