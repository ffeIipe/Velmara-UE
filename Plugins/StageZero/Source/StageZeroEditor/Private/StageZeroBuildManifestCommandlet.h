// Copyright (c) 2025 71 Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "StageZeroBuildManifestCommandlet.generated.h"

UCLASS()
class UStageZeroBuildManifestCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UStageZeroBuildManifestCommandlet();

    virtual int32 Main(const FString& Params) override;
};
