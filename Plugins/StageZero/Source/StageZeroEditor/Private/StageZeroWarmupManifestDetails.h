// Copyright (c) 2025 71 Studios. All rights reserved.

#pragma once

#include "IDetailCustomization.h"

class UStageZeroWarmupManifest;

class FStageZeroWarmupManifestDetails : public IDetailCustomization
{
public:
    static TSharedRef<IDetailCustomization> MakeInstance();

    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
    void AddSummary(IDetailLayoutBuilder& DetailBuilder, UStageZeroWarmupManifest* Manifest);
    void HideHeavyProperties(IDetailLayoutBuilder& DetailBuilder);
};
