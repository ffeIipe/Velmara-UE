// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectiles/Projectile.h"
#include "SpectralProjectile.generated.h"

/**
 * 
 */

class UTimelineComponent;

UCLASS()
class TESISUE_API ASpectralProjectile : public AProjectile
{
	GENERATED_BODY()
public:
	ASpectralProjectile();

protected:
	void BeginPlay() override;
	//void Tick(float DeltaTime) override;

	UPROPERTY(BlueprintReadWrite, Category = "Projectile | Target")
	AActor* Target;

private:

	UPROPERTY(EditAnywhere, Category = "Projectile | Target")
	float TrackTargetDistance;
	
	UPROPERTY(EditAnywhere, Category = "Projectile | Target")
	float TrackTargetRadius;

	UPROPERTY()
	UTimelineComponent* ProjectileTimeline;

	UPROPERTY(EditAnywhere, Category = "Projectile | Target")
	class UCurveFloat* ProjectileCurve;

	UFUNCTION(BlueprintCallable)
	void StartCurve();

	//UFUNCTION(BlueprintCallable)
	//void StopCurve();

	UFUNCTION(BlueprintCallable)
	void UpdateCurve(float Alpha);

	void TrackTarget();

	void PerformTrack(AActor* Target);
};
