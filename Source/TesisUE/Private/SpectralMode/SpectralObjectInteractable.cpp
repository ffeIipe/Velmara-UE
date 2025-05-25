// Fill out your copyright notice in the Description page of Project Settings.


#include "SpectralMode/SpectralObjectInteractable.h"
#include "Kismet/GameplayStatics.h"

ASpectralObjectInteractable::ASpectralObjectInteractable()
{
	//DoorMeshArray = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));

	VisibleTo = ECharacterForm::ECF_Spectral;
}

void ASpectralObjectInteractable::BeginPlay()
{
	Super::BeginPlay();
}

void ASpectralObjectInteractable::SpectralInteract_Implementation()
{
	if (bIsDoorOpen) return;

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
	bIsDoorOpen = true;
	//DoorMesh->DestroyComponent();
}
