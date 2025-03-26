// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectiles/Projectile.h"
#include "SpectralBarrier.generated.h"

/**
 * 
 */

class AEnemy;

UCLASS()
class TESISUE_API ASpectralBarrier : public AProjectile
{
	GENERATED_BODY()

public:
	ASpectralBarrier();

protected:
	void BeginPlay() override;

	UFUNCTION()
	void OnProjectileImpact(AActor* OtherActor, const FHitResult& Hit) override;

private:
	UFUNCTION(BlueprintCallable)
	void OnOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	TArray<AEnemy*> EnemyTargets;

};
