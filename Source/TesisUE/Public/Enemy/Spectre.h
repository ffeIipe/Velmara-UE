#pragma once

#include "CoreMinimal.h"
#include "Enemy/Enemy.h"
#include "Spectre.generated.h"

/**
 * 
 */
UCLASS()
class TESISUE_API ASpectre : public AEnemy
{
	GENERATED_BODY()
	
protected:
	float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser) override;

	void GetHit_Implementation(const FVector& ImpactPoint, TSubclassOf<UDamageType> DamageType) override;

	void Die(AActor* DamageCauser) override;
};
