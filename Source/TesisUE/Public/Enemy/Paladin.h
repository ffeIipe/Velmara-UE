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
	
	virtual bool IsLaunchable_Implementation(ACharacter* DamageCauser) override;

	virtual void LaunchUp_Implementation(const FVector& InstigatorLocation) override;

	void GetHit_Implementation(const FVector& ImpactPoint, TSubclassOf<UDamageType> DamageType) override;

	float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable)
	virtual void SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled);

protected:
	void BeginPlay();

	void Die(AActor* DamageCauser) override;
	
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

	TArray<AEnemy*> GenerateSphereOverlapToDetectOtherEnemies(const FVector& Origin, AActor* HitEnemyToExclude);

	UPROPERTY(EditAnywhere)
	float RadiusToNotifyAllies = 2500.f;

	void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintImplementableEvent)
	void PerformAttackEvent();

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* AttackAction;

	void LaunchEnemyUp(const FVector& InstigatorLocation);

	UFUNCTION(BlueprintCallable)
	void CrashDown();
	
	UFUNCTION(BlueprintCallable)
	void HitInAir();

	void ReactToDamage(EMainDamageTypes DamageType, const FVector& ImpactPoint) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraShake")
	TSubclassOf<UCameraShakeBase> CameraShake;

	UPROPERTY(EditAnywhere, Category = "SFX")
	USoundBase* ShieldImpactSFX;

	UFUNCTION()
	bool NotifyDamageTakenToBlackboard(AActor* DamageCauser);

private:
	UPROPERTY(EditAnywhere, Category = "SFX")
	USoundBase* ShieldDettachSFX;

	void ShieldHit();

	void Attack(const FInputActionValue& Value) override;

	void HeavyAttack(const FInputActionValue& Value);

	void Finisher(const FInputActionValue& Value);
};
