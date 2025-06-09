// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/HitInterface.h"
#include "Interfaces/MementoEntity.h"
#include "Interfaces/CharacterState.h"
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
	EES_None UMETA(DisplayName = "None"),
	EES_Walk UMETA(DisplayName = "Walk"),
	EES_Run UMETA(DisplayName = "Run")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyDeactivated, AEnemy*, DeactivatedEnemy);

UCLASS()
class TESISUE_API AEnemy : public ACharacter, public IHitInterface, public IMementoEntity, public ICharacterState
{
	GENERATED_BODY()

public:
	AEnemy();

	UFUNCTION(BlueprintCallable, Category = "Pooling")
	virtual void ActivateEnemy(const FVector& Location, const FRotator& Rotation);

	UFUNCTION(BlueprintCallable, Category = "Pooling")
	virtual void DeactivateEnemy();

	virtual void PoolableDie(AActor* DamageCauser);

	UPROPERTY(BlueprintAssignable, Category = "Pooling")
	FOnEnemyDeactivated OnDeactivated;

	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser) override;

	virtual void Die(AActor* DamageCauser);

	bool bWasPossessed = false;

	virtual void Tick(float DeltaTime) override;

	virtual void GetHit_Implementation(const FVector& ImpactPoint, TSubclassOf<UDamageType> DamageType) override;

	virtual void GetFinished_Implementation() override;

	virtual bool CanBeFinished_Implementation() override;

	virtual bool IsLaunchable_Implementation(ACharacter* Character) override;

	virtual void LaunchUp_Implementation(const FVector& InstigatorLocation) override;

	virtual void ShieldHit_Implementation();

	virtual UMementoComponent* GetMementoComponent_Implementation() override;

	virtual UCharacterStateComponent* GetCharacterStateComponent_Implementation() override;

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
	virtual void OnPossessed(APlayerMain* NewOwner, float OwnerEnergy);
	
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
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components | ExtraMovementComponent")
	class UExtraMovementComponent* ExtraMovementComponent;

	UFUNCTION(BlueprintCallable, Category = "Save System")
	FName GetUniqueSaveID() const { return UniqueSaveID; }

	APlayerMain* GetPossessionOwner() { return PossessionOwner; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components | Combat")
	UCombatComponent* CombatComponent;

	virtual void BeginPlay() override;

	virtual void Jump() override;

	FTimerHandle ReturnToPoolTimerHandle;

	void RequestReturnToPool();

	ECollisionEnabled::Type InitialMeshCollisionEnabled;
	ECollisionEnabled::Type InitialCapsuleCollisionEnabled;
	TMap<TEnumAsByte<ECollisionChannel>, ECollisionResponse> InitialMeshCollisionResponses;
	TMap<TEnumAsByte<ECollisionChannel>, ECollisionResponse> InitialCapsuleCollisionResponses;
	bool bInitialMeshGenerateOverlapEvents;
	bool bInitialCapsuleGenerateOverlapEvents;

	float DefaultMaxWalkSpeed;
	float DefaultGravityScale;
	float DefaultJumpZVelocity;
	bool bDefaultOrientRotationToMovement;
	bool bDefaultUseControllerDesiredRotation;
	bool bOriginalUseControllerRotationYaw;

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

	UPROPERTY(EditAnywhere, Category = "Visual Effects | Dissolve")
	TArray<UMaterialInstanceDynamic*> DissolveMaterials;

	UPROPERTY(VisibleAnywhere)
	class UTimelineComponent* DissolveTimeline;

	UPROPERTY(EditDefaultsOnly, Category = "Visual Effects | Dissolve")
	class UCurveFloat* DissolveCurve;
	
	UPROPERTY(EditDefaultsOnly, Category = "Visual Effects | Dissolve")
	UNiagaraComponent* DissolveParticleComponent;

	UFUNCTION()
	void UpdateDissolveEffect(float Value);

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

	virtual void Attack(const FInputActionValue& Value);

	virtual void DoubleJump(const FInputActionValue& Value);

	virtual void Dodge(const FInputActionValue& Value);

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
	FTimerHandle HitFlashTimerHandle;

	UFUNCTION(BlueprintCallable)
	void HandleEnemyCollision(ECollisionResponse CollisionResponse);
};