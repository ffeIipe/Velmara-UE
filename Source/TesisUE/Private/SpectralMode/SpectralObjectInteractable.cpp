// Fill out your copyright notice in the Description page of Project Settings.


#include "SpectralMode/SpectralObjectInteractable.h"

ASpectralObjectInteractable::ASpectralObjectInteractable()
{
	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(GetRootComponent());
}

void ASpectralObjectInteractable::BeginPlay()
{
	Super::BeginPlay();
}

void ASpectralObjectInteractable::SpectralInteract_Implementation()
{
	DoorMesh->SetVisibility(false);
}
