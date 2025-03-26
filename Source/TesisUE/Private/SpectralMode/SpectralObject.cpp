#include "SpectralMode/SpectralObject.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"

ASpectralObject::ASpectralObject()
{
    PrimaryActorTick.bCanEverTick = false;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = Mesh;

    Mesh->SetCollisionObjectType(ECC_GameTraceChannel1);

    BoxCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollider"));
    BoxCollider->SetupAttachment(Mesh);
}

void ASpectralObject::BeginPlay()
{
    Super::BeginPlay();

    if (Mesh->GetMaterial(0))
    {
        SpectralMaterial = UMaterialInstanceDynamic::Create(Mesh->GetMaterial(0), this);
        Mesh->SetMaterial(0, SpectralMaterial);
    }

    SetSpectralVisibility(false);
}

void ASpectralObject::SetSpectralVisibility(bool bIsVisible)
{
    if (SpectralMaterial)
    {
        float AlphaValue = bIsVisible ? 1.0f : 0.3f;
        SpectralMaterial->SetScalarParameterValue(FName("Alpha"), AlphaValue);
    }

    Mesh->SetVisibility(bIsVisible);
    Mesh->SetCollisionEnabled(bIsVisible ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);

    BoxCollider->SetCollisionEnabled(bIsVisible ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
}
