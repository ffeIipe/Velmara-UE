#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"
#include "Containers/Ticker.h"
#include "HAL/PlatformProcess.h"
#include "StageZeroManifestBuildOptions.h"
#include "StageZeroManifestOverridesEditorObject.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Notifications/SNotificationList.h"

class STextBlock;
template <typename ItemType> class SListView;
class UStageZeroWarmupManifest;
class IDetailsView;
struct FPropertyChangedEvent;

struct FStageZeroMapListEntry
{
    FAssetData AssetData;
    bool bSelected = false;
    bool bIsWorldPartition = false;
};

class SStageZeroManifestTab : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SStageZeroManifestTab) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    virtual ~SStageZeroManifestTab();

private:
    FReply HandleGenerateClicked();
    FReply HandleGenerateWithWPClicked();
    FReply HandleOpenManifestAsset();
    FReply HandleOpenManifestLog();
    FReply HandleCancelManifestBuild();
    FReply HandleOpenRatePlugin();
    FReply HandleOpenDiscord();
    FReply HandleOpenRoadmap();
    FReply HandleOpenWalkthrough();
    FReply HandleOpenSupportEmail();
    bool StartManifestBuildProcess();
    bool PollManifestProcess(float DeltaSeconds);
    void StopManifestPoll();
    void TerminateManifestProcess();
    void ShowNotification(const FString& Message, SNotificationItem::ECompletionState State);
    void RefreshStats();
    void SetStatus(const FString& Message, const FSlateColor& Color = FSlateColor(FLinearColor::White));
    void SetStats(const FString& Message);
    void LoadOptionsFromConfig();
    void SaveOptionsToConfig();
    void SyncManualOverridesEditorFromOptions();
    void HandleManualOverridesChanged(const FPropertyChangedEvent& PropertyChangedEvent);
    void GatherWorldAssets();
    void SyncSelectionToOptions(bool bPreserveHiddenSelections = true);
    bool HasAnySelectedMap() const;
    bool IsWorldPartitionAsset(const FAssetData& AssetData) const;
    void SetAllMapSelections(bool bSelected);
    void SelectWorldPartitionMapsOnly();
    EVisibility GetMapSelectionVisibility() const;
    ECheckBoxState GetUseAllMapsState() const;
    ECheckBoxState GetUseSelectedMapsState() const;
    void OnUseAllMapsChanged(ECheckBoxState NewState);
    void OnUseSelectedMapsChanged(ECheckBoxState NewState);
    ECheckBoxState GetIncludeEngineContentState() const;
    void OnIncludeEngineContentChanged(ECheckBoxState NewState);
    ECheckBoxState GetMapCheckedState(TSharedPtr<struct FStageZeroMapListEntry> Entry) const;
    void OnMapCheckStateChanged(ECheckBoxState NewState, TSharedPtr<struct FStageZeroMapListEntry> Entry);
    TSharedRef<ITableRow> MakeMapRowWidget(TSharedPtr<struct FStageZeroMapListEntry> Entry, const TSharedRef<STableViewBase>& OwnerTable);
    bool IsGenerateEnabled() const;
    void ShowMissingSelectionWarnings(const class UStageZeroWarmupManifest* Manifest);
    void BeginProgress(const FString& Label, bool bIndeterminate = true);
    void UpdateProgress(float Value, const FString& Label);
    void EndProgress();
    void UpdateProgressFromLog();
    bool ConfirmCommandletManifestBuild() const;
    bool ConfirmInEditorManifestBuild() const;
    FString GetSelectedMapSummary() const;

private:
    FStageZeroManifestBuildOptions BuildOptions;
    TArray<TSharedPtr<struct FStageZeroMapListEntry>> MapEntries;

    TSharedPtr<STextBlock> StatusText;
    TSharedPtr<STextBlock> StatsText;
    TSharedPtr<SListView<TSharedPtr<struct FStageZeroMapListEntry>>> MapListView;
    TSharedPtr<class SProgressBar> BuildProgressBar;
    TStrongObjectPtr<UStageZeroManifestOverridesEditorObject> ManualOverridesEditorObject;
    TSharedPtr<IDetailsView> ManualOverridesDetailsView;
    TSharedPtr<STextBlock> BuildProgressText;
    FProcHandle ManifestProcHandle;
    FTSTicker::FDelegateHandle ManifestTickerHandle;
    bool bBuildInProgress = false;
    bool bProgressIndeterminate = true;
    float BuildProgressValue = 0.f;
    FString BuildProgressLabel;
    FString ManifestLogPath;
    FString ManifestLogTail;
    int64 ManifestLogReadOffset = 0;
    bool bManifestBuildLaunchCancelled = false;
};
