// Fill out your copyright notice in the Description page of Project Settings.


#include "SpectralMode/SpectralObjectInteractable.h"
#include "SpectralMode/SpectralReceiver.h"
#include "Components/SphereComponent.h"
#include "Player/PlayerMain.h"

void ASpectralObjectInteractable::BeginPlay()
{
	Super::BeginPlay();
}

void ASpectralObjectInteractable::SetSpectralVisibility(bool bIsVisible)
{
	Super::SetSpectralVisibility(bIsVisible);
}

void ASpectralObjectInteractable::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{	
	Super::OnSphereBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
}

void ASpectralObjectInteractable::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	Super::OnSphereEndOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex);
}

void ASpectralObjectInteractable::SpectralInteract()
{
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 1.f, FColor::White, FString("No object to interact"));
	if (ObjectToInteract)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 1.f, FColor::Black, FString("There is an object to interact"));
		ObjectToInteract->Open();
	}
}