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

	for (AActor* Door : DoorArray)
	{
		if (!IsValid(Door)) break;

		else
		{
			//Door->Destroy();

			OpenDoors(Door);

			if (OpenDoorSFX)
			{
				UGameplayStatics::PlaySoundAtLocation(GetWorld(), OpenDoorSFX, Door->GetActorLocation());
			}
		}
	}
}

void ASpectralObjectInteractable::SpectralInteract_Implementation(ACharacter* Character)
{
	Use(Character);
}
