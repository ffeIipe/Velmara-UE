#include "SpectralMode/SpectralObject.h"
#include "Components/StaticMeshComponent.h"
#include "Subsystems/SpectralObjectsSubsystem.h"

void ASpectralObject::BeginPlay()
{
    Super::BeginPlay();

    AddToSpectralObjects();
}

void ASpectralObject::ActivateVisiblity()
{
    SetActorEnableCollision(true);
    SetActorHiddenInGame(false);   
}

void ASpectralObject::DeactivateVisibility()
{
    SetActorEnableCollision(false);
    SetActorHiddenInGame(true);
}

void ASpectralObject::AddToSpectralObjects()
{
    if (USpectralObjectsSubsystem* SpectralObjectsSubsystem = GetWorld()->GetSubsystem<USpectralObjectsSubsystem>())
    {
        SpectralObjectsSubsystem->AddSpectralObject(this);
    }
}