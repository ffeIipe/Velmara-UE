#include "SpectralMode/SpectralObject.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"

ASpectralObject::ASpectralObject()
{
    PrimaryActorTick.bCanEverTick = false;

    ItemMesh->SetCollisionObjectType(ECC_GameTraceChannel1);
}

void ASpectralObject::BeginPlay()
{
    Super::BeginPlay();
}
