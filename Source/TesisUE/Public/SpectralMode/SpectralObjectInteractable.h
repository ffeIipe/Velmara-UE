// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SpectralMode/SpectralObject.h"
#include "SpectralReceiver.h"
#include "Interfaces/SpectralInteractable.h"
#include "SpectralObjectInteractable.generated.h"

class USphereComponent;
class APlayerMain;

UCLASS()
class TESISUE_API ASpectralObjectInteractable : public ASpectralObject, public ISpectralInteractable
{
	GENERATED_BODY()

public:
	void BeginPlay() override;
	
	void SetSpectralVisibility(bool bIsVisible) override;

	UPROPERTY(EditAnywhere)
	ASpectralReceiver* ObjectToInteract;

	virtual void OnSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);
	
	virtual void OnSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	UFUNCTION()
	void SpectralInteract() override;
};
