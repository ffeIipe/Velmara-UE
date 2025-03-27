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

void ASpectralObject::SetSpectralVisibility(bool bIsPlayerInHumanForm)
{
    bool bShouldBeVisible = (VisibleTo == EPlayerForm::EPF_Spectral) ? bIsPlayerInHumanForm : !bIsPlayerInHumanForm;

    float AlphaValue = bShouldBeVisible ? 1.0f : 0.3f;
    SpectralMaterial->SetScalarParameterValue(FName("Alpha"), AlphaValue);

    Mesh->SetVisibility(bShouldBeVisible);
    Mesh->SetCollisionEnabled(bShouldBeVisible ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);

    BoxCollider->SetCollisionEnabled(bShouldBeVisible ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
}