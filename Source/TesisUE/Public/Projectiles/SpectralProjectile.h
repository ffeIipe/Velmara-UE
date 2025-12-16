// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectiles/Projectile.h"
#include "SpectralProjectile.generated.h"

/**
 * 
 */

class UTimelineComponent;
class AEnemy;

UCLASS()
class TESISUE_API ASpectralProjectile : public AProjectile
{
	GENERATED_BODY()

public:
	ASpectralProjectile();

protected:
	void OnBoxOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult) override;

	UPROPERTY(BlueprintReadWrite, Category = "Projectile | Target")
	AEnemy* Target;
};
