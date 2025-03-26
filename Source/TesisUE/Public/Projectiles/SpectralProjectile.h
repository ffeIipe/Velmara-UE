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

	void BeginPlay() override;
	UPROPERTY(BlueprintReadWrite, Category = "Projectile | Target")
	AEnemy* Target;

	void OnProjectileImpact(AActor* OtherActor, const FHitResult& Hit) override;
};
