// Fill out your copyright notice in the Description page of Project Settings.


#include "SpectralMode/SpectralObjectInteractable.h"
#include "Kismet/GameplayStatics.h"

ASpectralObjectInteractable::ASpectralObjectInteractable()
{
	VisibleTo = ECharacterForm::ECF_Spectral;
}

void ASpectralObjectInteractable::Use(ACharacter* Character)
{
	Super::Use(Character);

	for (UStaticMeshComponent* Door : DoorMeshArray)
	{
		if (!IsValid(Door) || !Door->GetStaticMesh()) break;

		else
		{
			Door->DestroyComponent();

			if (OpenDoorSFX)
			{
				UGameplayStatics::PlaySoundAtLocation(GetWorld(), OpenDoorSFX, Door->GetComponentLocation());
			}
		}
	}
}

void ASpectralObjectInteractable::SpectralInteract_Implementation(ACharacter* Character)
{
	Use(Character);
}
