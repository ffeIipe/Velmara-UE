// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectiles/Projectile.h"
#include "EnemySpectralProjectile.generated.h"

/**
 * 
 */
UCLASS()
class TESISUE_API AEnemySpectralProjectile : public AProjectile
{
	GENERATED_BODY()
	
protected:

	void BeginPlay() override;

	void OnBoxOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult) override;
};
