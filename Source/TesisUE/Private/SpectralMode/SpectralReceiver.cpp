// Fill out your copyright notice in the Description page of Project Settings.


#include "SpectralMode/SpectralReceiver.h"

ASpectralReceiver::ASpectralReceiver()
{
	PrimaryActorTick.bCanEverTick = false;

}

void ASpectralReceiver::Open()
{
	Destroy();
}


