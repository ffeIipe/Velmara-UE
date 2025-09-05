// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/SpectralObjectsSubsystem.h"

#include "SpectralMode/Interfaces/Spectral.h"

void USpectralObjectsSubsystem::AddSpectralObject(const TScriptInterface<ISpectral>& SpectralObject)
{
	if (SpectralObject)
	{
		SpectralObjects.Add(SpectralObject);
	}
}

void USpectralObjectsSubsystem::ActivateSpectralObjects()
{
	for (const TScriptInterface SpectralObject : SpectralObjects)
	{
		SpectralObject->ActivateVisiblity();
	}
}

void USpectralObjectsSubsystem::DeactivateSpectralObjects()
{
	for (const TScriptInterface SpectralObject : SpectralObjects)
	{
		SpectralObject->DeactivateVisibility();
	}
}

void USpectralObjectsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	DeactivateSpectralObjects();
}
