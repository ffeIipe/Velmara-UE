// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SpectralObjectsSubsystem.generated.h"

class ISpectral;
/**
 * 
 */
UCLASS()
class TESISUE_API USpectralObjectsSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()


public:
	void AddSpectralObject(const TScriptInterface<ISpectral>& SpectralObject);
	void ActivateSpectralObjects();
	void DeactivateSpectralObjects();

private:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	TArray<TScriptInterface<ISpectral>> SpectralObjects;
};
