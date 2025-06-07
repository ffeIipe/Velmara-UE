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
public:
	UFUNCTION(BlueprintCallable)
	void PerformSpectralAttack();

protected:
	ASpectre();

	void BeginPlay() override;

	float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser) override;

	bool CanBeFinished_Implementation() override;

	void GetHit_Implementation(const FVector& ImpactPoint, TSubclassOf<UDamageType> DamageType) override;

private:
	UPROPERTY(EditDefaultsOnly)
	TArray<UAnimMontage*> SpectralAttackMontages;

};
