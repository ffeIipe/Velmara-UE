#include "Components/SpectralObjectComponent.h"
#include <Components/BoxComponent.h>

USpectralObjectComponent::USpectralObjectComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	VisibleTo = ECharacterForm::ECF_Spectral;
}

void USpectralObjectComponent::SetSpectralVisibility(bool bIsVisible)
{
	if (SpectralMaterial.Num() == 0) return;

	bool bShouldBeVisible = (VisibleTo == ECharacterForm::ECF_Spectral) ? bIsVisible : !bIsVisible;

	float AlphaValue = bShouldBeVisible ? 1.0f : 0.3f;

	for (int32 i = 0; i < Doors.Num(); i++)
	{
		SpectralMaterial[i]->SetScalarParameterValue(FName("Alpha"), AlphaValue);

		Doors[i]->SetCollisionEnabled(bShouldBeVisible ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);

		Doors[i]->SetVisibility(bShouldBeVisible);

		if (UBoxComponent* DoorColl = Doors[i]->GetOwner()->GetComponentByClass<UBoxComponent>())
		{
			DoorColl->SetCollisionEnabled(bShouldBeVisible ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		}
	}
}

void USpectralObjectComponent::BeginPlay()
{
	Super::BeginPlay();
	
	GetOwner()->GetComponents<UStaticMeshComponent>(Doors);

	for (int32 i = 0; i < Doors.Num(); i++)
	{
		if (Doors.IsValidIndex(i))
		{
			if (!Doors[i]->GetMaterial(0)) return;

			SpectralMaterial.Add(UMaterialInstanceDynamic::Create(Doors[i]->GetMaterial(0), this));
			Doors[i]->SetMaterial(0, SpectralMaterial[i]);
		}
	}

	SetSpectralVisibility(false);
}

