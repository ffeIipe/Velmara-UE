// Copyright (c) 2025 71 Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Runtime/Launch/Resources/Version.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "UObject/SoftObjectPath.h"

namespace StageZeroAssetRegistryCompat
{
inline void AddClassToFilter(FARFilter& Filter, const UClass* Class)
{
    if (!Class)
    {
        return;
    }

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 0
    Filter.ClassNames.Add(Class->GetFName());
#else
    Filter.ClassPaths.Add(Class->GetClassPathName());
#endif
}

inline bool GetAssetsByClass(IAssetRegistry& AssetRegistry, const UClass* Class, TArray<FAssetData>& OutAssets, bool bSearchSubClasses)
{
    if (!Class)
    {
        OutAssets.Reset();
        return false;
    }

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 0
    return AssetRegistry.GetAssetsByClass(Class->GetFName(), OutAssets, bSearchSubClasses);
#else
    return AssetRegistry.GetAssetsByClass(Class->GetClassPathName(), OutAssets, bSearchSubClasses);
#endif
}

inline bool GetAssets(IAssetRegistry& AssetRegistry, const FARFilter& Filter, TArray<FAssetData>& OutAssets, bool bSkipARFilteredAssets = true)
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 0
    return AssetRegistry.GetAssets(Filter, OutAssets);
#else
    return AssetRegistry.GetAssets(Filter, OutAssets, bSkipARFilteredAssets);
#endif
}

inline bool IsAssetClassExact(const FAssetData& Asset, const UClass* Class)
{
    if (!Class || !Asset.IsValid())
    {
        return false;
    }

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 0
    return Asset.AssetClass == Class->GetFName();
#else
    return Asset.AssetClassPath == Class->GetClassPathName();
#endif
}

inline bool IsAssetClassOrChild(const FAssetData& Asset, const UClass* Class)
{
    if (IsAssetClassExact(Asset, Class))
    {
        return true;
    }

    if (!Class || !Asset.IsValid())
    {
        return false;
    }

    if (UClass* AssetClass = Asset.GetClass())
    {
        return AssetClass->IsChildOf(Class);
    }

    return false;
}

inline FAssetData GetAssetByObjectPath(IAssetRegistry& AssetRegistry, const FSoftObjectPath& ObjectPath)
{
    if (!ObjectPath.IsValid())
    {
        return FAssetData();
    }

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 0
    return AssetRegistry.GetAssetByObjectPath(FName(*ObjectPath.ToString()));
#else
    return AssetRegistry.GetAssetByObjectPath(ObjectPath);
#endif
}

inline void GetPackageDependencies(IAssetRegistry& AssetRegistry, const FName& PackageName, TArray<FName>& OutDependencies)
{
    OutDependencies.Reset();

    using namespace UE::AssetRegistry;
    FDependencyQuery Query;
    AssetRegistry.GetDependencies(PackageName, OutDependencies, EDependencyCategory::Package, Query);
}
} // namespace StageZeroAssetRegistryCompat
