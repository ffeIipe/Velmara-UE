// Copyright (c) 2025 71 Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "StageZeroProjectSettings.h"

struct STAGEZERO_API FStageZeroNumberFormatting
{
    static EStageZeroNumberFormat GetConfiguredCountFormat();
    static FString FormatCount(int64 Value, EStageZeroNumberFormat Format);
    static FString FormatConfiguredCount(int64 Value);
    static FText FormatConfiguredCountText(int64 Value);
};
