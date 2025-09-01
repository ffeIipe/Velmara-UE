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

	virtual bool IsLaunchable() override;
	
	virtual void LaunchUp(const FVector& InstigatorLocation) override;

	virtual void ShieldHit_Implementation() {}
	
	virtual void SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled) override;

protected:

	UPROPERTY(EditDefaultsOnly, Category = "Energy | OnPossession")
	float PossessionAttackCost;
	
	UPROPERTY(EditDefaultsOnly, Category = "Energy | OnPossession")
	float PossessionHeavyAttackCost;
	
	UPROPERTY(EditDefaultsOnly, Category = "Energy | OnPossession")
	float PossessionDamage;
	
	UPROPERTY(EditDefaultsOnly, Category = "Movement | OnPossession")
	float PossessionMaxWalkSpeed = 700.f;

	virtual void BeginPlay() override;

	virtual void Die(UAnimMontage* DeathAnim, FName Section) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Damage;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* SwordMesh;
	
	UPROPERTY(VisibleAnywhere)
	UBoxComponent* SwordCollider;

	UPROPERTY(EditDefaultsOnly)
	USceneComponent* BoxTraceStart;
	
	UPROPERTY(EditDefaultsOnly)
	USceneComponent* BoxTraceEnd;

	UFUNCTION()
	virtual void OnSwordOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* AttackAction;

	void LaunchEnemyUp(const FVector& InstigatorLocation);

	UFUNCTION(BlueprintCallable)
	void CrashDown();
	
	UFUNCTION(BlueprintCallable)
	void HitInAir();

	virtual void ReactToDamage(EMainDamageTypes DamageType, const FVector& ImpactPoint) override;

	void Slash();

	virtual void ApplyPossessionParameters(bool bShouldEnable) override;

private:
	UPROPERTY(EditAnywhere, Category = "SFX")
	USoundBase* ShieldDetachSFX;

	UFUNCTION()
	void ShieldHit();
};
