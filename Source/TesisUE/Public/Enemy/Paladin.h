// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enemy/Enemy.h"
#include "Interfaces/CharacterState.h"
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

	virtual void LaunchUp_Implementation(const FVector& InstigatorLocation) override;

	virtual void ShieldHit_Implementation() {};

	//void GetHit_Implementation(AActor* DamageCauser, const FVector& ImpactPoint, TSubclassOf<UDamageType> DamageType, const float DamageReceived) override;

	virtual void SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled);

protected:

	UPROPERTY(EditDefaultsOnly, Category = "Energy | OnPossession")
	float PossessionAttackCost;
	
	UPROPERTY(EditDefaultsOnly, Category = "Energy | OnPossession")
	float PossessionHeavyAttackCost;
	
	UPROPERTY(EditDefaultsOnly, Category = "Energy | OnPossession")
	float PossessionDamage;
	
	void BeginPlay();

	void NotifyIsNotShieldedToBlackboard();

	void Die() override;
	
	TArray<AActor*> IgnoreActors;

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

	void ReactToDamage(EMainDamageTypes DamageType, const FVector& ImpactPoint) override;

	void GetDefaultParameters() override;
	void SetOnPossessedParameters() override;

private:
	UPROPERTY(EditAnywhere, Category = "SFX")
	USoundBase* ShieldDettachSFX;

	void ShieldHit();
};
