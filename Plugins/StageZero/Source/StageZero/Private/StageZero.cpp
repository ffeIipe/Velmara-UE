// Copyright (c) 2025 71 Studios for games Development. All rights reserved.

#include "StageZero.h"
#include "StageZeroWarmupManifest.h"

#include "CoreGlobals.h"
#if WITH_EDITOR
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Engine/AssetManager.h"
#include "GameDelegates.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Settings/ProjectPackagingSettings.h"
#endif

#define LOCTEXT_NAMESPACE "FStageZeroModule"

DEFINE_LOG_CATEGORY_STATIC(LogStageZeroModule, Log, All);

void FStageZeroModule::StartupModule()
{
    // Content plugins are mounted by IPluginManager before module startup. Stage Zero
    // deliberately does not register a duplicate mount point or unregister one at shutdown.
#if WITH_EDITOR
    RegisterCookPackageDelegate();
#endif
}

void FStageZeroModule::ShutdownModule()
{
#if WITH_EDITOR
    if (ModifyCookDelegateHandle.IsValid())
    {
        FGameDelegates::Get().GetModifyCookDelegate().Remove(ModifyCookDelegateHandle);
        ModifyCookDelegateHandle.Reset();
    }
#endif
}

#if WITH_EDITOR
void FStageZeroModule::RegisterCookPackageDelegate()
{
    if (!ModifyCookDelegateHandle.IsValid())
    {
        ModifyCookDelegateHandle = FGameDelegates::Get().GetModifyCookDelegate().AddRaw(
            this,
            &FStageZeroModule::AddStageZeroPackagesToCook);
    }
}

void FStageZeroModule::AddStageZeroPackagesToCook(
    TConstArrayView<const ITargetPlatform*> /*TargetPlatforms*/,
    TArray<FName>& PackagesToCook,
    TArray<FName>& PackagesToNeverCook)
{
    static const FString PluginMountPath(TEXT("/StageZero"));
    static const FName PluginMountPathName(*PluginMountPath);
    static const FString ManifestMountPath(TEXT("/Game/StageZero"));
    static const FName ManifestPackageName(TEXT("/Game/StageZero/StageZeroWarmupManifest"));

    FString DlcName;
    if (FParse::Value(FCommandLine::Get(), TEXT("DLCName="), DlcName)
        && !DlcName.TrimStartAndEnd().IsEmpty())
    {
        UE_LOG(
            LogStageZeroModule,
            Log,
            TEXT("Stage Zero skipped base-game cook injection for DLC '%s'; Stage Zero packages must come from the base release."),
            *DlcName);
        return;
    }

    IAssetRegistry& AssetRegistry =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    AssetRegistry.ScanPathsSynchronous(
        { PluginMountPath, ManifestMountPath },
        true,
        true);

    TSet<FName> NeverCookPackages;
    NeverCookPackages.Reserve(PackagesToNeverCook.Num());
    for (const FName PackageName : PackagesToNeverCook)
    {
        NeverCookPackages.Add(PackageName);
    }

    TSet<FName> PluginPackages;
    TArray<FAssetData> PluginAssets;
    const bool bPluginQuerySucceeded =
        AssetRegistry.GetAssetsByPath(PluginMountPathName, PluginAssets, true, true);
    for (const FAssetData& PluginAsset : PluginAssets)
    {
        if (!PluginAsset.IsValid()
            || PluginAsset.IsRedirector()
            || (PluginAsset.PackageFlags & PKG_EditorOnly) != 0
            || !PluginAsset.PackageName.ToString().StartsWith(
                PluginMountPath + TEXT("/"),
                ESearchCase::CaseSensitive))
        {
            continue;
        }

        PluginPackages.Add(PluginAsset.PackageName);
    }

    if (!bPluginQuerySucceeded)
    {
        UE_LOG(
            LogStageZeroModule,
            Error,
            TEXT("Stage Zero could not query on-disk Asset Registry data under %s after a synchronous scan."),
            *PluginMountPath);
    }
    else if (PluginPackages.IsEmpty())
    {
        UE_LOG(
            LogStageZeroModule,
            Error,
            TEXT("Stage Zero found no cookable on-disk packages under %s; required plugin content would be absent from the build."),
            *PluginMountPath);
    }

    TArray<FName> RequiredPackages = PluginPackages.Array();
    TArray<FAssetData> ManifestAssets;
    const bool bManifestQuerySucceeded = AssetRegistry.GetAssetsByPackageName(
        ManifestPackageName,
        ManifestAssets,
        true,
        false);
    const FTopLevelAssetPath ManifestClassPath =
        UStageZeroWarmupManifest::StaticClass()->GetClassPathName();
    const bool bManifestValid = ManifestAssets.ContainsByPredicate(
        [&ManifestClassPath](const FAssetData& ManifestAsset)
        {
            return ManifestAsset.IsValid()
                && !ManifestAsset.IsRedirector()
                && (ManifestAsset.PackageFlags & PKG_EditorOnly) == 0
                && ManifestAsset.AssetClassPath == ManifestClassPath;
        });

    if (bManifestValid)
    {
        RequiredPackages.Add(ManifestPackageName);
    }
    else if (!bManifestQuerySucceeded)
    {
        UE_LOG(
            LogStageZeroModule,
            Error,
            TEXT("Stage Zero's required manifest package %s has no on-disk Asset Registry data. Generate and save the manifest before cooking."),
            *ManifestPackageName.ToString());
    }
    else
    {
        UE_LOG(
            LogStageZeroModule,
            Error,
            TEXT("Stage Zero's required package %s does not contain a cookable %s asset. Redirectors, editor-only assets, and other classes are not valid manifests."),
            *ManifestPackageName.ToString(),
            *ManifestClassPath.ToString());
    }

    RequiredPackages.Sort(FNameLexicalLess());

    const UProjectPackagingSettings* PackagingSettings = GetDefault<UProjectPackagingSettings>();
    const auto FindPackagingDirectoryConflict =
        [PackagingSettings](FName PackageName, FString& OutSettingName, FString& OutDirectory) -> bool
        {
            FString PackageLocalPath = FPackageName::LongPackageNameToFilename(PackageName.ToString());
            PackageLocalPath = FPaths::ConvertRelativePathToFull(PackageLocalPath);
            FPaths::NormalizeFilename(PackageLocalPath);

            const auto FindInDirectories =
                [&PackageLocalPath, &OutSettingName, &OutDirectory](
                    const TArray<FDirectoryPath>& Directories,
                    const TCHAR* SettingName) -> bool
                {
                    for (const FDirectoryPath& Directory : Directories)
                    {
                        FString DirectoryLocalPath;
                        if (Directory.Path.IsEmpty()
                            || !FPackageName::TryConvertGameRelativePackagePathToLocalPath(
                                Directory.Path,
                                DirectoryLocalPath))
                        {
                            continue;
                        }

                        DirectoryLocalPath = FPaths::ConvertRelativePathToFull(DirectoryLocalPath);
                        FPaths::NormalizeDirectoryName(DirectoryLocalPath);
                        if (FPaths::IsSamePath(PackageLocalPath, DirectoryLocalPath)
                            || FPaths::IsUnderDirectory(PackageLocalPath, DirectoryLocalPath))
                        {
                            OutSettingName = SettingName;
                            OutDirectory = Directory.Path;
                            return true;
                        }
                    }
                    return false;
                };

            return FindInDirectories(
                    PackagingSettings->DirectoriesToNeverCook,
                    TEXT("DirectoriesToNeverCook"))
                || FindInDirectories(
                    PackagingSettings->TestDirectoriesToNotSearch,
                    TEXT("TestDirectoriesToNotSearch"));
        };

    int32 EligiblePackageCount = 0;
    int32 NewlyRequestedPackageCount = 0;
    for (const FName PackageName : RequiredPackages)
    {
        if (NeverCookPackages.Contains(PackageName))
        {
            UE_LOG(
                LogStageZeroModule,
                Error,
                TEXT("Stage Zero's required package %s is marked NeverCook. Remove the conflicting cook exclusion."),
                *PackageName.ToString());
            continue;
        }

        FString ConflictingSetting;
        FString ConflictingDirectory;
        if (FindPackagingDirectoryConflict(
                PackageName,
                ConflictingSetting,
                ConflictingDirectory))
        {
            UE_LOG(
                LogStageZeroModule,
                Error,
                TEXT("Stage Zero's required package %s is excluded by Project Packaging setting %s entry '%s'."),
                *PackageName.ToString(),
                *ConflictingSetting,
                *ConflictingDirectory);
            continue;
        }

        if (!UAssetManager::Get().VerifyCanCookPackage(nullptr, PackageName, false))
        {
            UE_LOG(
                LogStageZeroModule,
                Error,
                TEXT("Stage Zero's required package %s is rejected by the active Asset Manager cook policy."),
                *PackageName.ToString());
            continue;
        }

        const bool bWasAlreadyRequested = PackagesToCook.Contains(PackageName);
        PackagesToCook.AddUnique(PackageName);
        ++EligiblePackageCount;
        NewlyRequestedPackageCount += bWasAlreadyRequested ? 0 : 1;
    }

    UE_LOG(
        LogStageZeroModule,
        Display,
        TEXT("Stage Zero validated %d eligible required package(s) and added %d new cook request(s) (%d plugin package(s), manifest=%s)."),
        EligiblePackageCount,
        NewlyRequestedPackageCount,
        PluginPackages.Num(),
        bManifestValid ? TEXT("valid") : TEXT("invalid"));
}
#endif

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FStageZeroModule, StageZero)
