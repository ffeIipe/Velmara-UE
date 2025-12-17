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

	//void BeginPlay() override;

	void HandleEnemyCollision(bool bEnable) override;
	
	float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser) override;

	bool IsLaunchable_Implementation(ACharacter* Character) override;

	bool CanBeFinished_Implementation() override;

	//virtual void OnPossessed(AEntity* NewOwner, float OwnerEnergy) override;

private:
	UPROPERTY(EditDefaultsOnly)
	TArray<UAnimMontage*> SpectralAttackMontages;
};