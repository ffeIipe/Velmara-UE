#pragma once

#include "CoreMinimal.h"
#include "Enemy/Enemy.h"
#include "Paladin.generated.h"

class UBoxComponent;

UCLASS()
class TESISUE_API APaladin : public AEnemy
{
	GENERATED_BODY()

public:
	APaladin();

	virtual void ActivateEnemy(const FVector& Location, const FRotator& Rotation) override;

	virtual void DeactivateEnemy() override;
	
	virtual void LaunchUp() override;

	virtual void ShieldHit_Implementation() {}

protected:
	virtual void BeginPlay() override;

	virtual void OnConstruction(const FTransform& Transform) override;
	
	virtual void Die(UAnimMontage* DeathAnim, FName Section) override;

	UFUNCTION(BlueprintCallable)
	void CrashDown();
	
	UFUNCTION(BlueprintCallable)
	void HitInAir();

	virtual void ReactToDamage(EMeleeDamageTypes DamageType, const FVector& ImpactPoint) override;

	void Slash();

private:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UChildActorComponent* WeaponToEquip;
	
	UPROPERTY(EditAnywhere, Category = "SFX")
	USoundBase* ShieldDetachSFX;

	UFUNCTION()
	void ShieldHit();
};
