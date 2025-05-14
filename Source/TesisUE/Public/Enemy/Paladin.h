// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enemy/Enemy.h"
#include "Interfaces/CharacterState.h"
#include "Paladin.generated.h"

class UBoxComponent;

UCLASS()
class TESISUE_API APaladin : public AEnemy, public ICharacterState
{
	GENERATED_BODY()

public:
	APaladin();

	virtual bool IsLaunchable_Implementation() override;

	virtual void LaunchUp_Implementation(const FVector& InstigatorLocation) override;

	void GetHit_Implementation(const FVector& ImpactPoint) override;

	virtual UCharacterStateComponent* GetCharacterStateComponent_Implementation() override;

	float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser) override;

protected:
	void BeginPlay();
	
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

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components | Combat")
	UCombatComponent* CombatComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components | CharacterStateComponent")
	UCharacterStateComponent* CharacterStateComponent;

	UFUNCTION(BlueprintCallable)
	virtual void SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled);

	UFUNCTION()
	virtual void OnSwordOverlap(
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

	void LaunchEnemyUp(const FVector& InstigatorLocation);

	UFUNCTION(BlueprintCallable)
	void CrashDown();
	
	UFUNCTION(BlueprintCallable)
	void HitInAir();

	void ReactToDamage(EMainDamageTypes DamageType, const FVector& ImpactPoint) override;

private:
	void ShieldHit();

	void Attack(const FInputActionValue& Value);

	void Finisher(const FInputActionValue& Value);
};
