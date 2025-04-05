// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SpectralMode/SpectralObject.h"
#include "SpectralTrap.generated.h"

class APlayerMain;

UCLASS()
class TESISUE_API ASpectralTrap : public ASpectralObject
{
	GENERATED_BODY()
	
	void OnSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult) override;

	void OnSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex) override;

	void ApplyTrapDamage();

	FTimerHandle DamageTimerHandle;
	APlayerMain* OverlappingPlayer = nullptr;

	UPROPERTY(EditAnywhere, Category = "Trap")
	float DamageInterval = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Trap")
	float DamagePerTick = 5.0f;
};
