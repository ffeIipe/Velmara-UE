// Copyright (c) 2025 71 Studios. All rights reserved.

#include "StageZeroWPAnchor.h"

#include "Components/SphereComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/CollisionProfile.h"

AStageZeroWPAnchor::AStageZeroWPAnchor()
{
    PrimaryActorTick.bCanEverTick = false;

#if WITH_EDITORONLY_DATA
    bIsEditorOnlyActor = true;
    Radius = 3000.0f;
    Priority = 0;
    GroupName = NAME_None;

    USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SceneRoot->SetMobility(EComponentMobility::Movable);
    RootComponent = SceneRoot;

    PreviewSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PreviewSphere"));
    PreviewSphere->SetupAttachment(SceneRoot);
    PreviewSphere->InitSphereRadius(Radius);
    PreviewSphere->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
    PreviewSphere->SetGenerateOverlapEvents(false);
    PreviewSphere->SetHiddenInGame(true);
    PreviewSphere->SetCanEverAffectNavigation(false);
    PreviewSphere->SetIsVisualizationComponent(true);
    PreviewSphere->SetMobility(EComponentMobility::Movable);
#else
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
#endif
}

#if WITH_EDITOR
void AStageZeroWPAnchor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    RefreshPreview();
}

void AStageZeroWPAnchor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    RefreshPreview();
}
#endif

#if WITH_EDITORONLY_DATA
void AStageZeroWPAnchor::RefreshPreview()
{
    if (!PreviewSphere)
    {
        return;
    }

    const float SafeRadius = FMath::Max(0.0f, Radius);
    PreviewSphere->SetSphereRadius(SafeRadius);
    PreviewSphere->SetRelativeLocation(FVector::ZeroVector);
#if WITH_EDITOR
    if (RootComponent)
    {
        RootComponent->SetMobility(EComponentMobility::Movable);
    }
#endif
}
#endif
