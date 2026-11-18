#include "StageZeroManifestOverrideSerialization.h"

namespace StageZeroManifestOverrideSerialization
{
namespace
{
constexpr TCHAR PathSeparator = TEXT(';');
constexpr TCHAR FieldSeparator = TEXT('|');
constexpr TCHAR RecordSeparator = TEXT('~');

bool ContainsReservedDelimiter(const FString& Value)
{
    return Value.Contains(FString::Chr(PathSeparator))
        || Value.Contains(FString::Chr(FieldSeparator))
        || Value.Contains(FString::Chr(RecordSeparator));
}

bool ValidateSerializablePath(const FSoftObjectPath& Path, const TCHAR* Label, FString& OutError)
{
    const FString Value = Path.ToString().TrimStartAndEnd();
    if (Value.IsEmpty() || Path.GetLongPackageName().IsEmpty())
    {
        OutError = FString::Printf(TEXT("%s contains an empty or malformed asset path"), Label);
        return false;
    }
    if (ContainsReservedDelimiter(Value))
    {
        OutError = FString::Printf(TEXT("%s path '%s' contains a reserved delimiter"), Label, *Value);
        return false;
    }
    return true;
}
}

void NormalizePathList(TArray<FSoftObjectPath>& Paths)
{
    TMap<FString, FSoftObjectPath> UniquePaths;
    bool bContainsEmptyPath = false;
    for (const FSoftObjectPath& Path : Paths)
    {
        const FString Key = Path.ToString().TrimStartAndEnd();
        if (Key.IsEmpty())
        {
            bContainsEmptyPath = true;
        }
        else
        {
            UniquePaths.FindOrAdd(Key, FSoftObjectPath(Key));
        }
    }

    UniquePaths.GenerateValueArray(Paths);
    if (bContainsEmptyPath)
    {
        Paths.Add(FSoftObjectPath());
    }
    Paths.Sort([](const FSoftObjectPath& Left, const FSoftObjectPath& Right)
    {
        return Left.ToString() < Right.ToString();
    });
}

void NormalizeOptions(FStageZeroManifestBuildOptions& Options)
{
    NormalizePathList(Options.SelectedMaps);
    NormalizePathList(Options.GlobalAdditionalAssets);
    NormalizePathList(Options.GlobalExcludedAssets);

    TMap<FString, FStageZeroManifestMapAssetOverrides> MergedOverrides;
    for (FStageZeroManifestMapAssetOverrides& Override : Options.PerMapAssetOverrides)
    {
        NormalizePathList(Override.AdditionalAssets);
        NormalizePathList(Override.ExcludedAssets);

        const FString MapKey = Override.Map.ToString().TrimStartAndEnd();
        FStageZeroManifestMapAssetOverrides& Merged = MergedOverrides.FindOrAdd(MapKey);
        if (Merged.Map.IsNull())
        {
            Merged.Map = FSoftObjectPath(MapKey);
        }
        Merged.AdditionalAssets.Append(Override.AdditionalAssets);
        Merged.ExcludedAssets.Append(Override.ExcludedAssets);
        NormalizePathList(Merged.AdditionalAssets);
        NormalizePathList(Merged.ExcludedAssets);
    }

    MergedOverrides.GenerateValueArray(Options.PerMapAssetOverrides);
    Options.PerMapAssetOverrides.Sort([](
        const FStageZeroManifestMapAssetOverrides& Left,
        const FStageZeroManifestMapAssetOverrides& Right)
    {
        return Left.Map.ToString() < Right.Map.ToString();
    });
}

bool EncodePathList(const TArray<FSoftObjectPath>& Paths, FString& OutValue, FString& OutError)
{
    TArray<FSoftObjectPath> Normalized = Paths;
    NormalizePathList(Normalized);

    TArray<FString> Values;
    Values.Reserve(Normalized.Num());
    for (const FSoftObjectPath& Path : Normalized)
    {
        if (!ValidateSerializablePath(Path, TEXT("Asset override"), OutError))
        {
            OutValue.Reset();
            return false;
        }
        Values.Add(Path.ToString());
    }

    OutValue = FString::Join(Values, *FString::Chr(PathSeparator));
    OutError.Reset();
    return true;
}

bool DecodePathList(const FString& Value, TArray<FSoftObjectPath>& OutPaths, FString& OutError)
{
    OutPaths.Reset();
    if (Value.IsEmpty())
    {
        OutError.Reset();
        return true;
    }
    if (Value.Contains(FString::Chr(FieldSeparator)) || Value.Contains(FString::Chr(RecordSeparator)))
    {
        OutError = TEXT("Asset override list contains a reserved field or record delimiter");
        return false;
    }

    TArray<FString> Tokens;
    Value.ParseIntoArray(Tokens, *FString::Chr(PathSeparator), false);
    for (const FString& Token : Tokens)
    {
        const FString Trimmed = Token.TrimStartAndEnd();
        const FSoftObjectPath Path(Trimmed);
        if (!ValidateSerializablePath(Path, TEXT("Asset override"), OutError))
        {
            OutPaths.Reset();
            return false;
        }
        OutPaths.Add(Path);
    }

    NormalizePathList(OutPaths);
    OutError.Reset();
    return true;
}

bool EncodeMapOverride(const FStageZeroManifestMapAssetOverrides& Override, FString& OutValue, FString& OutError)
{
    if (!ValidateSerializablePath(Override.Map, TEXT("Per-map override map"), OutError))
    {
        OutValue.Reset();
        return false;
    }

    FString Additional;
    FString Excluded;
    if (!EncodePathList(Override.AdditionalAssets, Additional, OutError)
        || !EncodePathList(Override.ExcludedAssets, Excluded, OutError))
    {
        OutValue.Reset();
        return false;
    }

    OutValue = FString::Printf(TEXT("%s%c%s%c%s"),
        *Override.Map.ToString(),
        FieldSeparator,
        *Additional,
        FieldSeparator,
        *Excluded);
    OutError.Reset();
    return true;
}

bool DecodeMapOverride(const FString& Value, FStageZeroManifestMapAssetOverrides& OutOverride, FString& OutError)
{
    TArray<FString> Fields;
    Value.ParseIntoArray(Fields, *FString::Chr(FieldSeparator), false);
    if (Fields.Num() != 3)
    {
        OutError = TEXT("Per-map override must contain exactly a map, include list, and exclude list");
        return false;
    }

    FStageZeroManifestMapAssetOverrides Parsed;
    Parsed.Map = FSoftObjectPath(Fields[0].TrimStartAndEnd());
    if (!ValidateSerializablePath(Parsed.Map, TEXT("Per-map override map"), OutError)
        || !DecodePathList(Fields[1], Parsed.AdditionalAssets, OutError)
        || !DecodePathList(Fields[2], Parsed.ExcludedAssets, OutError))
    {
        return false;
    }

    OutOverride = MoveTemp(Parsed);
    OutError.Reset();
    return true;
}

bool EncodeMapOverrides(const TArray<FStageZeroManifestMapAssetOverrides>& Overrides, FString& OutValue, FString& OutError)
{
    FStageZeroManifestBuildOptions NormalizedOptions;
    NormalizedOptions.PerMapAssetOverrides = Overrides;
    NormalizeOptions(NormalizedOptions);

    TArray<FString> Records;
    Records.Reserve(NormalizedOptions.PerMapAssetOverrides.Num());
    for (const FStageZeroManifestMapAssetOverrides& Override : NormalizedOptions.PerMapAssetOverrides)
    {
        FString Record;
        if (!EncodeMapOverride(Override, Record, OutError))
        {
            OutValue.Reset();
            return false;
        }
        Records.Add(MoveTemp(Record));
    }

    OutValue = FString::Join(Records, *FString::Chr(RecordSeparator));
    OutError.Reset();
    return true;
}

bool DecodeMapOverrides(const FString& Value, TArray<FStageZeroManifestMapAssetOverrides>& OutOverrides, FString& OutError)
{
    OutOverrides.Reset();
    if (Value.IsEmpty())
    {
        OutError.Reset();
        return true;
    }

    TArray<FString> Records;
    Value.ParseIntoArray(Records, *FString::Chr(RecordSeparator), false);
    for (const FString& Record : Records)
    {
        FStageZeroManifestMapAssetOverrides Override;
        if (!DecodeMapOverride(Record, Override, OutError))
        {
            OutOverrides.Reset();
            return false;
        }
        OutOverrides.Add(MoveTemp(Override));
    }

    FStageZeroManifestBuildOptions NormalizedOptions;
    NormalizedOptions.PerMapAssetOverrides = MoveTemp(OutOverrides);
    NormalizeOptions(NormalizedOptions);
    OutOverrides = MoveTemp(NormalizedOptions.PerMapAssetOverrides);
    OutError.Reset();
    return true;
}
}
