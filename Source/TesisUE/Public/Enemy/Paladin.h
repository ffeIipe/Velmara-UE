// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enemy/Enemy.h"
#include "Paladin.generated.h"

/**
 * 
 */
UCLASS()
class TESISUE_API APaladin : public AEnemy
{
	GENERATED_BODY()
public:


protected:
	void GetHit_Implementation(const FVector& ImpactPoint) override;
	
	float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser) override;

private:
	void Disarm();
};
