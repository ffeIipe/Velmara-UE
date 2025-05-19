// Fill out your copyright notice in the Description page of Project Settings.


#include "SpectralMode/SpectralObjectInteractable.h"
#include "Kismet/GameplayStatics.h"

ASpectralObjectInteractable::ASpectralObjectInteractable()
{
	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));

	VisibleTo = ECharacterForm::ECF_Spectral;
}

void ASpectralObjectInteractable::BeginPlay()
{
	Super::BeginPlay();
}

void ASpectralObjectInteractable::SpectralInteract_Implementation()
{
	if (bIsDoorOpen) return;

	if (OpenDoorSFX)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), OpenDoorSFX, DoorMesh->GetComponentLocation());
	}
	DoorMesh->DestroyComponent();
	bIsDoorOpen = true;
}
