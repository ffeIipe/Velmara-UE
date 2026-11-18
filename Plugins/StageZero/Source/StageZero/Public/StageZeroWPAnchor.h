// Copyright (c) 2025 71 Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StageZeroWPAnchor.generated.h"

class USphereComponent;

/**
 * Editor-authored marker used to bake a deterministic World Partition asset
 * subset into the Stage Zero manifest. The actor is never traversed at runtime.
 */
UCLASS(BlueprintType, Blueprintable)
class STAGEZERO_API AStageZeroWPAnchor : public AActor
{
    GENERATED_BODY()

public:
    AStageZeroWPAnchor();

#if WITH_EDITORONLY_DATA
    /** Radius used to build the warmup bubble for this anchor. */
    UPROPERTY(EditAnywhere, Category = "Stage Zero|World Partition")
    float Radius;

    /** Optional name to group anchors together. */
    UPROPERTY(EditAnywhere, Category = "Stage Zero|World Partition")
    FName GroupName;

    /** Optional priority hint for tooling. */
    UPROPERTY(EditAnywhere, Category = "Stage Zero|World Partition")
    int32 Priority;

    /** Editor-only visualization sphere. */
    UPROPERTY(VisibleAnywhere, Category = "Stage Zero|World Partition")
    TObjectPtr<USphereComponent> PreviewSphere;
#endif

protected:
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    virtual void OnConstruction(const FTransform& Transform) override;
#endif

private:
#if WITH_EDITORONLY_DATA
    void RefreshPreview();
#endif
};
