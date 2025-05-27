// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/HitInterface.h"
#include "Interfaces/MementoEntity.h"
#include "Subsystems/EnemyPoolManager.h"
#include "Enemy.generated.h"

class UAttributeComponent;
class USpringArmComponent;
class UInputAction;
struct FInputActionValue;
class UCameraComponent;
class UInputMappingContext;
class APlayerMain;
class AAIController;
class UPromptWidgetComponent;
class UMementoComponent;
class UCombatComponent;
class UCharacterStateComponent;
class UBehaviorTree;

class UNiagaraSystem;
class UNiagaraComponent;

UENUM(BlueprintType)
enum class EEnemyType : uint8
{
	Paladin UMETA(DisplayName = "Paladin"),
	Spectre UMETA(DisplayName = "Spectre"),
	ShieldedPaladin UMETA(DisplayName = "ShieldedPaladin")
};

UENUM(BlueprintType)
enum class EEnemyState : uint8
{
	EES_Launched UMETA(DisplayName = "Launched"),
	EES_Died UMETA(DisplayName = "Died"),
	EES_None UMETA(DisplayName = "None")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyDeactivated, AEnemy*, DeactivatedEnemy);

UCLASS()
class TESISUE_API AEnemy : public ACharacter, public IHitInterface, public IMementoEntity
{
	GENERATED_BODY()

public:
	AEnemy();

	// Called when an enemy is retrieved from the pool or freshly spawned for use
	UFUNCTION(BlueprintCallable, Category = "Pooling")
	virtual void ActivateEnemy(const FVector& Location, const FRotator& Rotation);

	// Called when an enemy is to be returned to the pool
	UFUNCTION(BlueprintCallable, Category = "Pooling")
	virtual void DeactivateEnemy();

	// New Die function for poolable enemies
	virtual void PoolableDie(AActor* DamageCauser);

	// Delegate broadcasted when this enemy is deactivated
	UPROPERTY(BlueprintAssignable, Category = "Pooling")
	FOnEnemyDeactivated OnDeactivated;

	// Existing TakeDamage - we'll modify its call to Die
	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser) override;

	// Existing Die - will be refactored or become the core of PoolableDie's immediate actions
	virtual void Die(AActor* DamageCauser);

	bool bWasPossessed = false;

	virtual void Tick(float DeltaTime) override;

	virtual void GetHit_Implementation(const FVector& ImpactPoint, TSubclassOf<UDamageType> DamageType) override;

	virtual void GetFinished_Implementation() override;

	virtual bool CanBeFinished_Implementation() override;

	virtual bool IsLaunchable_Implementation();

	virtual void LaunchUp_Implementation(const FVector& InstigatorLocation);

	virtual void ShieldHit_Implementation();

	virtual UMementoComponent* GetMementoComponent_Implementation() override;

	FORCEINLINE EEnemyType GetEnemyType() const { return EnemyType; }

	FORCEINLINE EEnemyState GetEnemyState() const { return EnemyState; }

	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	FORCEINLINE AActor* GetDamageCauserActor() const { return DamageCauserOf; }


	void SetEnemyState(EEnemyState NewState);

	UPROPERTY(VisibleAnywhere);
	USpringArmComponent* SpringArm;

	UFUNCTION(BlueprintCallable)
	void DisableAI();

	UFUNCTION(BlueprintCallable)
	void EnableAI();
	
	UFUNCTION()
	USpringArmComponent* GetSpringArm();
	
	UFUNCTION()
	void OnPossessed(APlayerMain* NewOwner, float OwnerEnergy);
	
	UFUNCTION()
	virtual void UnPossessBase();
	
	UFUNCTION()
	void UnPossess();

	UFUNCTION()
	void UnPossessAndKill();

	void NotifyThreat(AActor* ThreatActor);

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components | AttributesComponent");
	UAttributeComponent* Attributes;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components | MementoComponent");
	UMementoComponent* Memento;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components | PromptWidgetComponent")
	UPromptWidgetComponent* PromptWidgetComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components | CharacterStateComponent")
	UCharacterStateComponent* CharacterStateComponent;

	UFUNCTION(BlueprintCallable, Category = "Save System")
	FName GetUniqueSaveID() const { return UniqueSaveID; }

protected:
	virtual void BeginPlay() override;

	FTimerHandle ReturnToPoolTimerHandle;

	// Function to handle the actual return to pool process
	void RequestReturnToPool();

	// Initial collision settings (to restore on activation)
	ECollisionEnabled::Type InitialMeshCollisionEnabled;
	ECollisionEnabled::Type InitialCapsuleCollisionEnabled;
	TMap<TEnumAsByte<ECollisionChannel>, ECollisionResponse> InitialMeshCollisionResponses;
	TMap<TEnumAsByte<ECollisionChannel>, ECollisionResponse> InitialCapsuleCollisionResponses;
	bool bInitialMeshGenerateOverlapEvents;
	bool bInitialCapsuleGenerateOverlapEvents;

	// Store initial values for quick reset
	float DefaultMaxWalkSpeed;
	float DefaultGravityScale;
	float DefaultJumpZVelocity;
	bool bDefaultOrientRotationToMovement;
	bool bDefaultUseControllerDesiredRotation; // from AEnemy constructor
	bool bOriginalUseControllerRotationYaw; // from AEnemy constructor

	//UFUNCTION()
	//virtual void Die(AActor* DamageCauser);

	UFUNCTION(BlueprintCallable)
	virtual void DirectionalHitReact(const FVector& ImpactPoint, UAnimMontage* HitReactAnimMontage);

	UPROPERTY(EditAnywhere, Category = "Energy | Energy Drop");
	float MinEnergy = 1.f;
	
	UPROPERTY(EditAnywhere, Category = "Energy| Energy Drop");
	float MaxEnergy = 3.f;
	
	UPROPERTY(EditAnywhere, Category = "Energy| Energy Tax");
	float UnpossesEnergyTax = 3.f;
	
	UPROPERTY(EditAnywhere, Category = "Energy| Energy Tax");
	float UnpossesAndKillEnergyTax = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = "Montages");
	UAnimMontage* HitReactMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Montages");
	UAnimMontage* DeathMontage;
	
	UPROPERTY(EditDefaultsOnly, Category = "Montages");
	UAnimMontage* FinisherDeathMontage;

	UPROPERTY(EditAnywhere, Category = "Sounds");
	USoundBase* HitSound;
	
	UPROPERTY(EditAnywhere, Category = "Sounds");
	USoundBase* ErrorSFX;

	UPROPERTY(EditAnywhere, Category = "Visual Effects");
	UNiagaraSystem* NiagaraSystem;

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
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* UnPossessAndKillAction;

	virtual void Move(const FInputActionValue& Value);
	
	virtual void Look(const FInputActionValue& Value);

	APlayerMain* PossessionOwner;
	
	UPROPERTY(VisibleAnywhere)
	bool isLaunched = false;

	UFUNCTION(BlueprintCallable)
	void ResetEnemy();
		
	UFUNCTION()		
	virtual void ReactToDamage(EMainDamageTypes DamageType, const FVector& ImpactPoint);
	
	UPROPERTY();
	EMainDamageTypes LastDamageType;

	FName SelectRandomDieAnim();

	UPROPERTY()
	AActor* DamageCauserOf;

	UPROPERTY(EditAnywhere)
	UBehaviorTree* BTAsset;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Save System", meta = (DisplayName = "Unique Save ID"))
	FName UniqueSaveID;

private:
	AAIController* AIOriginalController;

	UPROPERTY()
	UMaterialInstanceDynamic* DynamicMaterial;

	UPROPERTY()
	FTimerHandle HitFlashTimerHandle;

	UFUNCTION()
	void HitFlash();

	UFUNCTION()
	void ResetColor();

	UFUNCTION(BlueprintCallable)
	void DeactivateEnemyCollision();
};