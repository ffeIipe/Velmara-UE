// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/HitInterface.h"
#include "Enemy.generated.h"

class UAttributeComponent;
class UHealthBarComponent;
class USpringArmComponent;
class UInputAction;
struct FInputActionValue;
class UCameraComponent;
class UInputMappingContext;
class APlayerMain;
class AAIController;

UENUM(BlueprintType)
enum class EEnemyType : uint8
{
	Paladin UMETA(DisplayName = "Paladin"),
	Spectre UMETA(DisplayName = "Spectre"),
	ShieldedPaladin UMETA(DisplayName = "ShieldedPaladin"),
};

UENUM(BlueprintType)
enum class EEnemyState : uint8
{
	EES_Launched UMETA(DisplayName = "Launched")
};

UCLASS()
class TESISUE_API AEnemy : public ACharacter, public IHitInterface
{
	GENERATED_BODY()

public:
	
	AEnemy();
	virtual void Tick(float DeltaTime) override;

	virtual void GetHit_Implementation(const FVector& ImpactPoint) override;

	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser) override;

	FORCEINLINE EEnemyType GetEnemyType() const { return EnemyType; }

	FORCEINLINE EEnemyState GetEnemyState() const { return EnemyState; }

	UPROPERTY(VisibleAnywhere);
	USpringArmComponent* SpringArm;

	UFUNCTION(BlueprintCallable)
	void DisableAI();

	UFUNCTION(BlueprintCallable)
	void EnableAI();
	
	UFUNCTION()
	USpringArmComponent* GetSpringArm();
	
	UFUNCTION()
	void OnPossessed(APlayerMain* NewOwner);
	
	UFUNCTION()
	void UnPossess();

	UFUNCTION(BlueprintCallable)
	void LaunchEnemyUp();

	UFUNCTION(BlueprintCallable)
	void CrashDown();
	
	UFUNCTION(BlueprintCallable)
	void HitInAir();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void Die();

	UFUNCTION()
	virtual void DirectionalHitReact(const FVector& ImpactPoint);

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnemyType")
	EEnemyType EnemyType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnemyState")
	EEnemyState EnemyState;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* MoveAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* LookAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* JumpAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* UnPossessAction;

	virtual void Move(const FInputActionValue& Value);
	
	virtual void Look(const FInputActionValue& Value);

	APlayerMain* PossessionOwner;
	
	UPROPERTY(VisibleAnywhere)
	bool isLaunched = false;

	UFUNCTION(BlueprintCallable)
	void ResetEnemy();
		
	UFUNCTION()		
	void ReactToDamage(EMainDamageTypes DamageType, const FVector& ImpactPoint);

private:

	UPROPERTY()
	EMainDamageTypes LastDamageType = EMainDamageTypes::EMDT_None;


	AAIController* AIOriginalController;

	UPROPERTY()
	UMaterialInstanceDynamic* DynamicMaterial;

	UPROPERTY()
	FTimerHandle HitFlashTimerHandle;

	UFUNCTION()
	void HitFlash();

	UFUNCTION()
	void ResetColor();
};




