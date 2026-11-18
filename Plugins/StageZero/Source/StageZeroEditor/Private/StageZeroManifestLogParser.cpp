#include "StageZeroManifestLogParser.h"

#include "StageZeroManifestBuilder.h"

namespace StageZeroManifestLog
{
namespace
{
enum class ESummaryField : uint32
{
    Schema = 1u << 0,
    Requested = 1u << 1,
    Resolved = 1u << 2,
    Missing = 1u << 3,
    Processed = 1u << 4,
    MapsWithAssets = 1u << 5,
    ZeroAssetMaps = 1u << 6,
    WorldLoadFailures = 1u << 7,
    Saved = 1u << 8,
    Preserved = 1u << 9,
    ForcedAllZeroSave = 1u << 10,
    Succeeded = 1u << 11,
    MissingMaps = 1u << 12,
    MissingReasons = 1u << 13,
    Reason = 1u << 14,
    Complete = 1u << 15,
};

constexpr uint32 RequiredFields = (1u << 16) - 1u;

bool ParseInteger(const FString& Value, int32& OutValue)
{
    if (Value.IsEmpty())
    {
        return false;
    }

    const uint64 Limit = static_cast<uint64>(MAX_int32);
    uint64 Accumulator = 0;
    for (int32 Cursor = 0; Cursor < Value.Len(); ++Cursor)
    {
        const TCHAR Character = Value[Cursor];
        if (Character < TEXT('0') || Character > TEXT('9'))
        {
            return false;
        }
        const uint64 Digit = static_cast<uint64>(Character - TEXT('0'));
        if (Accumulator > (Limit - Digit) / 10ull)
        {
            return false;
        }
        Accumulator = Accumulator * 10ull + Digit;
    }

    OutValue = static_cast<int32>(Accumulator);
    return true;
}

void ParseList(const FString& Value, TArray<FString>& OutValues)
{
    OutValues.Reset();
    if (!Value.Equals(TEXT("None"), ESearchCase::CaseSensitive))
    {
        Value.ParseIntoArray(OutValues, TEXT(";"), true);
    }
}
}

bool ParseSummary(const FString& LogContents, FStageZeroManifestBuildResult& OutResult)
{
    static const FString Prefix(TEXT("StageZeroBuildManifestSummary:"));
    const int32 FoundIndex = LogContents.Find(Prefix, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
    if (FoundIndex == INDEX_NONE)
    {
        return false;
    }

    FString Line = LogContents.Mid(FoundIndex);
    int32 NewlineIndex = INDEX_NONE;
    if (Line.FindChar(TEXT('\n'), NewlineIndex))
    {
        Line = Line.Left(NewlineIndex);
    }

    TArray<FString> Tokens;
    Line.Mid(Prefix.Len()).TrimStartAndEnd().ParseIntoArrayWS(Tokens);
    FStageZeroManifestBuildResult Parsed;
    uint32 SeenFields = 0;
    int32 Schema = 0;
    int32 Complete = 0;
    bool bInvalid = false;

    auto ParseField = [&SeenFields, &bInvalid](const FString& Token, const TCHAR* PrefixText, ESummaryField Field, int32& OutValue)
    {
        const FString FieldPrefix(PrefixText);
        if (!Token.StartsWith(FieldPrefix, ESearchCase::CaseSensitive))
        {
            return false;
        }
        const uint32 FieldBit = static_cast<uint32>(Field);
        if ((SeenFields & FieldBit) != 0 || !ParseInteger(Token.Mid(FieldPrefix.Len()), OutValue))
        {
            bInvalid = true;
            return true;
        }
        SeenFields |= FieldBit;
        return true;
    };

    auto AcceptBoolean = [&bInvalid](int32 Value, bool& OutValue)
    {
        if (Value != 0 && Value != 1)
        {
            bInvalid = true;
        }
        OutValue = Value == 1;
    };

    for (const FString& Token : Tokens)
    {
        int32 BoolValue = 0;
        if (ParseField(Token, TEXT("Schema="), ESummaryField::Schema, Schema)) {}
        else if (ParseField(Token, TEXT("Requested="), ESummaryField::Requested, Parsed.NumRequestedMaps)) {}
        else if (ParseField(Token, TEXT("Resolved="), ESummaryField::Resolved, Parsed.NumResolvedMaps)) {}
        else if (ParseField(Token, TEXT("Missing="), ESummaryField::Missing, Parsed.NumMissingRequestedMaps)) {}
        else if (ParseField(Token, TEXT("Processed="), ESummaryField::Processed, Parsed.NumProcessedMaps)) {}
        else if (ParseField(Token, TEXT("MapsWithAssets="), ESummaryField::MapsWithAssets, Parsed.NumMapsWithAssets)) {}
        else if (ParseField(Token, TEXT("ZeroAssetMaps="), ESummaryField::ZeroAssetMaps, Parsed.NumZeroAssetMaps)) {}
        else if (ParseField(Token, TEXT("WorldLoadFailures="), ESummaryField::WorldLoadFailures, Parsed.NumWorldLoadFailures)) {}
        else if (ParseField(Token, TEXT("Saved="), ESummaryField::Saved, BoolValue)) { AcceptBoolean(BoolValue, Parsed.bManifestSaved); }
        else if (ParseField(Token, TEXT("Preserved="), ESummaryField::Preserved, BoolValue)) { AcceptBoolean(BoolValue, Parsed.bExistingManifestPreserved); }
        else if (ParseField(Token, TEXT("ForcedAllZeroSave="), ESummaryField::ForcedAllZeroSave, BoolValue)) { AcceptBoolean(BoolValue, Parsed.bForcedAllZeroSave); }
        else if (ParseField(Token, TEXT("Succeeded="), ESummaryField::Succeeded, BoolValue)) { AcceptBoolean(BoolValue, Parsed.bSucceeded); }
        else if (Token.StartsWith(TEXT("MissingMaps="), ESearchCase::CaseSensitive))
        {
            if ((SeenFields & static_cast<uint32>(ESummaryField::MissingMaps)) != 0)
            {
                bInvalid = true;
            }
            ParseList(Token.Mid(12), Parsed.MissingRequestedMaps);
            SeenFields |= static_cast<uint32>(ESummaryField::MissingMaps);
        }
        else if (Token.StartsWith(TEXT("MissingReasons="), ESearchCase::CaseSensitive))
        {
            if ((SeenFields & static_cast<uint32>(ESummaryField::MissingReasons)) != 0)
            {
                bInvalid = true;
            }
            ParseList(Token.Mid(15), Parsed.MissingRequestedMapReasons);
            SeenFields |= static_cast<uint32>(ESummaryField::MissingReasons);
        }
        else if (Token.StartsWith(TEXT("Reason="), ESearchCase::CaseSensitive))
        {
            if ((SeenFields & static_cast<uint32>(ESummaryField::Reason)) != 0)
            {
                bInvalid = true;
            }
            Parsed.FailureReason = Token.Mid(7);
            if (Parsed.FailureReason.Equals(TEXT("None"), ESearchCase::CaseSensitive))
            {
                Parsed.FailureReason.Reset();
            }
            else
            {
                Parsed.FailureReason.ReplaceInline(TEXT("_"), TEXT(" "));
            }
            SeenFields |= static_cast<uint32>(ESummaryField::Reason);
        }
        else if (ParseField(Token, TEXT("Complete="), ESummaryField::Complete, Complete)) {}
        else
        {
            bInvalid = true;
        }
    }

    if (bInvalid || SeenFields != RequiredFields || Schema != 2 || Complete != 1)
    {
        return false;
    }

    const bool bCountsNonNegative = Parsed.NumRequestedMaps >= 0
        && Parsed.NumResolvedMaps >= 0
        && Parsed.NumMissingRequestedMaps >= 0
        && Parsed.NumProcessedMaps >= 0
        && Parsed.NumMapsWithAssets >= 0
        && Parsed.NumZeroAssetMaps >= 0
        && Parsed.NumWorldLoadFailures >= 0;
    const bool bCountRelationshipsValid = Parsed.NumRequestedMaps == Parsed.NumResolvedMaps + Parsed.NumMissingRequestedMaps
        && Parsed.NumProcessedMaps <= Parsed.NumResolvedMaps
        && Parsed.NumMapsWithAssets + Parsed.NumZeroAssetMaps == Parsed.NumProcessedMaps
        && Parsed.NumWorldLoadFailures <= Parsed.NumProcessedMaps
        && Parsed.NumMissingRequestedMaps == Parsed.MissingRequestedMaps.Num()
        && Parsed.NumMissingRequestedMaps == Parsed.MissingRequestedMapReasons.Num();
    if (!bCountsNonNegative || !bCountRelationshipsValid)
    {
        return false;
    }

    if (Parsed.bSucceeded)
    {
        const bool bSuccessfulContract = Parsed.bManifestSaved
            && Parsed.NumMissingRequestedMaps == 0
            && Parsed.NumProcessedMaps == Parsed.NumResolvedMaps
            && Parsed.NumResolvedMaps > 0
            && Parsed.NumWorldLoadFailures == 0
            && Parsed.NumMapsWithAssets > 0
            && !Parsed.bForcedAllZeroSave
            && Parsed.FailureReason.IsEmpty();
        if (!bSuccessfulContract)
        {
            return false;
        }
    }
    else if (Parsed.bManifestSaved || Parsed.bForcedAllZeroSave || Parsed.FailureReason.IsEmpty())
    {
        return false;
    }

    OutResult = MoveTemp(Parsed);
    return true;
}
}
