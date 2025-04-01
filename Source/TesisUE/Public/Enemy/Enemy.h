// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/HitInterface.h"
#include "Enemy.generated.h"

class UAttributeComponent;
class UHealthBarComponent;

UENUM(BlueprintType)
enum class EEnemyType : uint8
{
	Paladin UMETA(DisplayName = "Paladin"),
	Spectre UMETA(DisplayName = "Spectre"),
	ShieldedPaladin UMETA(DisplayName = "ShieldedPaladin"),
};

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

	FORCEINLINE EEnemyType GetEnemyType() const { return EnemyType; }

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void Die();

	UFUNCTION()
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	EEnemyType EnemyType;

private:
	UPROPERTY()
	UMaterialInstanceDynamic* DynamicMaterial;

	UPROPERTY()
	FTimerHandle HitFlashTimerHandle;

	UFUNCTION()
	void HitFlash();

	UFUNCTION()
	void ResetColor();
};




