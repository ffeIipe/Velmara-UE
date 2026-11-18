// Copyright (c) 2025 71 Studios. All rights reserved.

#include "StageZeroNumberFormatting.h"

EStageZeroNumberFormat FStageZeroNumberFormatting::GetConfiguredCountFormat()
{
    if (const UStageZeroProjectSettings* Settings = GetDefault<UStageZeroProjectSettings>())
    {
        return Settings->JobCountNumberFormat;
    }

    return EStageZeroNumberFormat::Comma;
}

FString FStageZeroNumberFormatting::FormatCount(int64 Value, EStageZeroNumberFormat Format)
{
    if (Format == EStageZeroNumberFormat::Plain)
    {
        return LexToString(Value);
    }

    const TCHAR Separator = (Format == EStageZeroNumberFormat::Dot) ? TEXT('.') : TEXT(',');
    const bool bNegative = Value < 0;
    const uint64 AbsValue = bNegative ? static_cast<uint64>(-(Value + 1)) + 1 : static_cast<uint64>(Value);
    const FString Digits = LexToString(AbsValue);

    if (Digits.Len() <= 3)
    {
        return bNegative ? FString::Printf(TEXT("-%s"), *Digits) : Digits;
    }

    FString Result;
    Result.Reserve(Digits.Len() + (Digits.Len() / 3) + (bNegative ? 1 : 0));

    if (bNegative)
    {
        Result.AppendChar(TEXT('-'));
    }

    int32 FirstGroupLength = Digits.Len() % 3;
    if (FirstGroupLength == 0)
    {
        FirstGroupLength = 3;
    }

    for (int32 Index = 0; Index < Digits.Len(); ++Index)
    {
        if (Index > 0 && Index >= FirstGroupLength && ((Index - FirstGroupLength) % 3) == 0)
        {
            Result.AppendChar(Separator);
        }
        Result.AppendChar(Digits[Index]);
    }

    return Result;
}

FString FStageZeroNumberFormatting::FormatConfiguredCount(int64 Value)
{
    return FormatCount(Value, GetConfiguredCountFormat());
}

FText FStageZeroNumberFormatting::FormatConfiguredCountText(int64 Value)
{
    return FText::FromString(FormatConfiguredCount(Value));
}
