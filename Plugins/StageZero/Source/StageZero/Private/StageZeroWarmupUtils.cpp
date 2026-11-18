// Copyright (c) 2025 71 Studios. All rights reserved.

#include "StageZeroWarmupUtils.h"

#include "StageZeroWarmupManifest.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"

DEFINE_LOG_CATEGORY(LogStageZeroWarmupManifest);

namespace
{
bool IsRuntimePackageNameAllowed(FString PackageName)
{
    PackageName.TrimStartAndEndInline();
    PackageName.ReplaceInline(TEXT("\\"), TEXT("/"));

    if (PackageName.IsEmpty() || !FPackageName::IsValidLongPackageName(PackageName))
    {
        return false;
    }

    static const TCHAR* DisallowedRoots[] =
    {
        TEXT("/Engine/Editor"),
        TEXT("/Engine/VREditor"),
        TEXT("/Engine/Developer"),
        TEXT("/Game/Developers/"),
        TEXT("/Temp/"),
        TEXT("/Memory/"),
        TEXT("/Script/")
    };
    for (const TCHAR* DisallowedRoot : DisallowedRoots)
    {
        if (PackageName.StartsWith(DisallowedRoot, ESearchCase::IgnoreCase))
        {
            return false;
        }
    }

    return !PackageName.Contains(TEXT("/Developers/"), ESearchCase::IgnoreCase);
}

namespace ManifestPaths
{
const TCHAR* PackagePath = TEXT("/Game/StageZero/StageZeroWarmupManifest");
const TCHAR* ObjectPath = TEXT("/Game/StageZero/StageZeroWarmupManifest.StageZeroWarmupManifest");
}
}

bool FStageZeroWarmupUtils::IsRuntimeWarmupPackageAllowed(const FName& PackageName)
{
    return PackageName != NAME_None && IsRuntimePackageNameAllowed(PackageName.ToString());
}

bool FStageZeroWarmupUtils::IsRuntimeWarmupAssetPathAllowed(const FSoftObjectPath& Path)
{
    if (!Path.IsValid() || Path.ToString().Contains(TEXT(":")))
    {
        return false;
    }

    const FString LongPackageName = Path.GetLongPackageName();
    return FPackageName::IsValidLongPackageName(LongPackageName) && IsRuntimePackageNameAllowed(LongPackageName);
}

bool FStageZeroWarmupUtils::IsRuntimeWarmupAssetAllowed(const UObject* Object)
{
    if (!Object || Object->IsEditorOnly())
    {
        return false;
    }

    const UPackage* Package = Object->GetOutermost();
    return Package && !Package->HasAnyPackageFlags(PKG_EditorOnly) && IsRuntimeWarmupPackageAllowed(Package->GetFName());
}

UStageZeroWarmupManifest* FStageZeroWarmupUtils::LoadAuthoredManifest()
{
    UStageZeroWarmupManifest* Manifest = LoadObject<UStageZeroWarmupManifest>(nullptr, GetManifestObjectPath());
    if (!Manifest)
    {
        UE_LOG(LogStageZeroWarmupManifest, Warning, TEXT("StageZero: Warmup manifest missing at %s. Runtime content mutation is disabled."), GetManifestObjectPath());
    }
    return Manifest;
}

UStageZeroWarmupManifest* FStageZeroWarmupUtils::GetOrBuildMatTexManifest(bool bForceRebuild)
{
    if (bForceRebuild)
    {
        UE_LOG(LogStageZeroWarmupManifest, Warning,
            TEXT("StageZero: GetOrBuildMatTexManifest(true) cannot rebuild content at runtime. Use FStageZeroManifestBuilder in the StageZeroEditor module."));
    }

    return LoadAuthoredManifest();
}

const TCHAR* FStageZeroWarmupUtils::GetManifestObjectPath()
{
    return ManifestPaths::ObjectPath;
}

const TCHAR* FStageZeroWarmupUtils::GetManifestPackagePath()
{
    return ManifestPaths::PackagePath;
}
