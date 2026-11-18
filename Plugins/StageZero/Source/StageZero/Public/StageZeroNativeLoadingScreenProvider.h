// Copyright (c) 2025 71 Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "StageZeroNativeLoadingScreenProvider.generated.h"

class SWidget;
class UStageZeroProjectSettings;

struct STAGEZERO_API FStageZeroNativeLoadingScreenContext
{
    FString MapName;
    bool bHasMovies = false;
    bool bIsPostLoadCover = false;
    const UStageZeroProjectSettings* Settings = nullptr;
};

/**
 * Optional C++ extension point for the blocking map-travel loading screen.
 *
 * MoviePlayer runs before the destination world and viewport UMG are fully stable, so this extension
 * intentionally returns native Slate. Use TravelPostLoadWidgetClass for UMG after the destination map loads.
 */
UCLASS(Abstract, BlueprintType)
class STAGEZERO_API UStageZeroNativeLoadingScreenProvider : public UObject
{
    GENERATED_BODY()

public:
    virtual TSharedPtr<SWidget> BuildLoadingScreenWidget(const FStageZeroNativeLoadingScreenContext& Context);
};
