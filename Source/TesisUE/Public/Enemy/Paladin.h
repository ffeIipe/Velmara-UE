// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enemy/Enemy.h"
#include "Interfaces/LaunchableInterface.h"
#include "Paladin.generated.h"

class UBoxComponent;

UCLASS()
class TESISUE_API APaladin : public AEnemy, public ILaunchableInterface
{
	GENERATED_BODY()

public:
	APaladin();

	virtual bool IsLaunchable_Implementation() override;

	virtual void LaunchUp_Implementation() override;

	virtual AActor* GetInterfaceOwner_Implementation() override;
protected:
	void BeginPlay();
	
	TArray<AActor*> IgnoreActors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Damage;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* SwordMesh;
	
	UPROPERTY(VisibleAnywhere)
	UBoxComponent* SwordBoxCollider;

	UPROPERTY(EditDefaultsOnly)
	USceneComponent* BoxTraceStart;
	
	UPROPERTY(EditDefaultsOnly)
	USceneComponent* BoxTraceEnd;

	UFUNCTION(BlueprintCallable)
	virtual void SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled);

	UFUNCTION()
	virtual void OnBoxOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintImplementableEvent)
	void PerformAttackEvent();

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* AttackAction;

	void LaunchEnemyUp();

	UFUNCTION(BlueprintCallable)
	void CrashDown();
	
	UFUNCTION(BlueprintCallable)
	void HitInAir();

	void ReactToDamage(EMainDamageTypes DamageType, const FVector& ImpactPoint) override;

private:
	void Attack();
};
