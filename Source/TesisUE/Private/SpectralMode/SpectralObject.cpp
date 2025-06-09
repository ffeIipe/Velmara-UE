#include "SpectralMode/SpectralObject.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SpectralObjectComponent.h"

ASpectralObject::ASpectralObject()
{
    PrimaryActorTick.bCanEverTick = false;

    ItemMesh->SetCollisionObjectType(ECC_GameTraceChannel1);

    SpectralObjectComponent = CreateDefaultSubobject<USpectralObjectComponent>(TEXT("SpectralObjectComponent"));
}

void ASpectralObject::BeginPlay()
{
    Super::BeginPlay();
}
