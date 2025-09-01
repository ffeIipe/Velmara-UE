// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SceneEvents/Trigger.h"
#include "SpectralTrap.generated.h"

class APlayerMain;

UCLASS()
class TESISUE_API ASpectralTrap : public ATrigger
{
	GENERATED_BODY()

private:
	void BeginPlay() override;

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

	void ApplyTrapDamage(const FVector& ImpactPoint);

	UPROPERTY(EditAnywhere, Category = "Stats | Trap")
	float Damage = 10.0f;
	
	UPROPERTY(EditAnywhere, Category = "Stats | Trap")
	float DamageInterval = .5f;

	UPROPERTY(EditAnywhere, Category = "Stats | SFX")
	USoundBase* TrapSFX;

	FTimerHandle ContinuousDamageTimerHandle;

	void DealContinuousDamage();
};
