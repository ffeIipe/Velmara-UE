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

	virtual void HandleEnemyCollision(bool bEnable) override;

	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser) override;

	virtual bool IsLaunchable() override;

	virtual bool CanBeFinished() override { return false; }

	virtual void ApplyPossessionParameters(bool bShouldEnable) override;

private:
	UPROPERTY(EditDefaultsOnly)
	TArray<UAnimMontage*> SpectralAttackMontages;
};