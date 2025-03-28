// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpectralReceiver.generated.h"

UCLASS()
class TESISUE_API ASpectralReceiver : public AActor
{
	GENERATED_BODY()
	
public:	
	ASpectralReceiver();

	void Open();
};
