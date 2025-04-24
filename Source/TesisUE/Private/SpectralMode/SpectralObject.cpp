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

   if (ItemMesh->GetMaterial(0))
   {
       SpectralMaterial = UMaterialInstanceDynamic::Create(ItemMesh->GetMaterial(0), this);
       ItemMesh->SetMaterial(0, SpectralMaterial);
   }
   
   SetSpectralVisibility(false);
}

void ASpectralObject::SetSpectralVisibility(bool bIsPlayerInHumanForm)
{
    if (!SpectralMaterial)
    {
        UE_LOG(LogTemp, Error, TEXT("SpectralMaterial is nullptr in ASpectralObject::SetSpectralVisibility"));
        return;
    }

    bool bShouldBeVisible = (VisibleTo == ECharacterForm::ECF_Spectral) ? bIsPlayerInHumanForm : !bIsPlayerInHumanForm;

    float AlphaValue = bShouldBeVisible ? 1.0f : 0.3f;
    SpectralMaterial->SetScalarParameterValue(FName("Alpha"), AlphaValue);

    ItemMesh->SetVisibility(bShouldBeVisible);
    ItemMesh->SetCollisionEnabled(bShouldBeVisible ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);

    BoxCollider->SetCollisionEnabled(bShouldBeVisible ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
}