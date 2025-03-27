// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/HitInterface.h"
#include "Enemy.generated.h"

class UAttributeComponent;
class UHealthBarComponent;

UCLASS()
class TESISUE_API AEnemy : public ACharacter, public IHitInterface
{
	GENERATED_BODY()

public:
	
	AEnemy();
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	virtual void GetHit_Implementation(const FVector& ImpactPoint) override;

	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser) override;

protected:
	virtual void BeginPlay() override;

	void Die();

	void DirectionalHitReact(const FVector& ImpactPoint);

	UPROPERTY(VisibleAnywhere);
	UAttributeComponent* Attributes;

	UPROPERTY(VisibleAnywhere);
	UHealthBarComponent* HealthBarWidget;

	UPROPERTY(EditDefaultsOnly, Category = "Montages");
	UAnimMontage* HitReactMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Montages");
	UAnimMontage* DeathMontage;

	UPROPERTY(EditAnywhere, Category = "Sounds");
	USoundBase* HitSound;

	UPROPERTY(EditAnywhere, Category = "Visual Effects");
	UParticleSystem* HitParticles;

};
