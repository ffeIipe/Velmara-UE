// Copyright (c) 2025 71 Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StageZeroWPSplineWarmup.generated.h"

class USplineComponent;

/**
 * Editor-authored spline used to bake a deterministic World Partition route
 * asset subset into the Stage Zero manifest. The spline is never traversed at runtime.
 */
UCLASS(BlueprintType, Blueprintable)
class STAGEZERO_API AStageZeroWPSplineWarmup : public AActor
{
    GENERATED_BODY()

public:
    AStageZeroWPSplineWarmup();

#if WITH_EDITORONLY_DATA
    /** Editable spline used to define the warmup route. */
    UPROPERTY(VisibleAnywhere, Category = "Stage Zero|World Partition")
    TObjectPtr<USplineComponent> Spline;

    /** Distance between samples along the spline (centers for warmup spheres). */
    UPROPERTY(EditAnywhere, Category = "Stage Zero|World Partition")
    float SampleDistance;

    /** Radius of each sample sphere used to collect intersecting cells. */
    UPROPERTY(EditAnywhere, Category = "Stage Zero|World Partition")
    float SampleRadius;

    /** Optional group identifier for the route. */
    UPROPERTY(EditAnywhere, Category = "Stage Zero|World Partition")
    FName GroupName;
#endif
};
