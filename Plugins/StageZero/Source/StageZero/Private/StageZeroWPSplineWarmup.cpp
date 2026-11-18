// Copyright (c) 2025 71 Studios. All rights reserved.

#include "StageZeroWPSplineWarmup.h"

#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"

AStageZeroWPSplineWarmup::AStageZeroWPSplineWarmup()
{
    PrimaryActorTick.bCanEverTick = false;

#if WITH_EDITORONLY_DATA
    bIsEditorOnlyActor = true;
    SampleDistance = 3000.0f;
    SampleRadius = 4000.0f;
    GroupName = NAME_None;

    USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SceneRoot->SetMobility(EComponentMobility::Movable);
    RootComponent = SceneRoot;

    Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
    Spline->SetupAttachment(SceneRoot);
    Spline->SetMobility(EComponentMobility::Movable);
    Spline->bDrawDebug = true;
#else
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
#endif
}
