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

	bool IsLaunchable_Implementation(ACharacter* Character) override;

	bool CanBeFinished_Implementation() override;

	void GetHit_Implementation(AActor* DamageCauser, const FVector& ImpactPoint, TSubclassOf<UDamageType> DamageType, const float DamageReceived) override;

	virtual void OnPossessed(APlayerMain* NewOwner, float OwnerEnergy) override;

private:
	UPROPERTY(EditDefaultsOnly)
	TArray<UAnimMontage*> SpectralAttackMontages;

	void Attack(const FInputActionValue& Value) override;
};
