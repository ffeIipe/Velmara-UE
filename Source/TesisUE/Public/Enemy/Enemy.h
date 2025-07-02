// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Entities/Entity.h"
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
class TESISUE_API AEnemy : public AEntity
{
	GENERATED_BODY()

public:
	AEnemy();

	UFUNCTION(BlueprintCallable, Category = "Pooling")
	virtual void ActivateEnemy(const FVector& Location, const FRotator& Rotation);

	UFUNCTION(BlueprintCallable, Category = "Pooling")
	virtual void DeactivateEnemy();

	virtual void PoolableDie(/*AActor* DamageCauser*/);

	UPROPERTY(BlueprintAssignable, Category = "Pooling")
	FOnEnemyDeactivated OnDeactivated;
	
	UPROPERTY(BlueprintAssignable)
	FOnEntityDead OnDead;

	UPROPERTY(BlueprintAssignable)
	FOnEntityDamaged OnDamaged;

	void Die(/*AActor* DamageCauser*/) override;

	bool bWasPossessed = false;

	virtual void GetHit_Implementation(AActor* DamageCauser, const FVector& ImpactPoint, TSubclassOf<UDamageType> DamageType, const float DamageReceived) override;

	void DropOrbs(const float DamageReceived, AActor* DamageCauser);

	virtual void GetFinished_Implementation() override;

	virtual bool CanBeFinished_Implementation() override;

	virtual bool IsLaunchable_Implementation(ACharacter* Character) override;

	//virtual void LaunchUp_Implementation(const FVector& InstigatorLocation) override;

	//virtual void ShieldHit_Implementation() {};

	void NotifyDamageTakenToBlackboard(AActor* DamageCauser);

	FORCEINLINE EEnemyType GetEnemyType() const { return EnemyType; }

	FORCEINLINE EEnemyState GetEnemyState() const { return EnemyState; }

	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	FORCEINLINE AActor* GetDamageCauserActor() const { return LastDamageCauser; }

	void SetEnemyState(EEnemyState NewState);

	UFUNCTION(BlueprintCallable)
	void DisableAI();

	UFUNCTION(BlueprintCallable)
	void EnableAI();

	void NotifyThreat(AActor* ThreatActor);

	UFUNCTION(BlueprintCallable, Category = "Save System")
	FName GetUniqueSaveID() const { return UniqueSaveID; }
	
	/*UFUNCTION()
	virtual void OnPossessed(AEntity* NewOwner, float OwnerEnergy);
	
	UFUNCTION()
	virtual void UnPossessBase();
	
	UFUNCTION()
	void UnPossess();

	UFUNCTION()
	void UnPossessAndKill();*/

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components | PromptWidgetComponent")
	UPromptWidgetComponent* PromptWidgetComponent;

protected:
	UPROPERTY(EditAnywhere)
	float RadiusToNotifyAllies = 2500.f;

	TArray<AEnemy*> GenerateSphereOverlapToDetectOtherEnemies(const FVector& Origin, AActor* HitEnemyToExclude);

	virtual void BeginPlay() override;

	void EnableFinisherWidget();

	FTimerHandle ReturnToPoolTimerHandle;

public:
	UFUNCTION()
	void RequestReturnToPool();

	AAIController* GetAIController() { return AIController; };

protected:
	ECollisionEnabled::Type InitialMeshCollisionEnabled;
	ECollisionEnabled::Type InitialCapsuleCollisionEnabled;
	TMap<TEnumAsByte<ECollisionChannel>, ECollisionResponse> InitialMeshCollisionResponses;
	TMap<TEnumAsByte<ECollisionChannel>, ECollisionResponse> InitialCapsuleCollisionResponses;
	bool bInitialMeshGenerateOverlapEvents;
	bool bInitialCapsuleGenerateOverlapEvents;

	float DefaultMaxWalkSpeed;
	float DefaultGravityScale;
	float DefaultJumpZVelocity;
	float DefaultDamage;
	bool bDefaultOrientRotationToMovement;
	bool bDefaultUseControllerDesiredRotation;
	bool bOriginalUseControllerRotationYaw;

	UPROPERTY(EditAnywhere, Category = "Energy | Energy Drop");
	float MinEnergy = 1.f;
	
	UPROPERTY(EditAnywhere, Category = "Energy| Energy Drop");
	float MaxEnergy = 3.f;
	
	UPROPERTY(EditAnywhere, Category = "Energy| Energy Tax");
	float UnpossesEnergyTax = 3.f;
	
	UPROPERTY(EditAnywhere, Category = "Energy| Energy Tax");
	float UnpossesAndKillEnergyTax = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = "Energy | OnPossession")
	float EnergyDivider = 2.f;

	UPROPERTY(EditAnywhere, Category = "DamageThreshold");
	float DamageThreshold = 30.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Montages");
	UAnimMontage* HitReactMontage;
	
	UPROPERTY(EditDefaultsOnly, Category = "Montages");
	UAnimMontage* FinisherDeathMontage;

	UPROPERTY(EditAnywhere, Category = "Effects | Dissolve")
	TArray<UMaterialInstanceDynamic*> DissolveMaterials;

	UPROPERTY(VisibleAnywhere)
	class UTimelineComponent* DissolveTimeline;

	UPROPERTY(EditDefaultsOnly, Category = "Effects | Dissolve")
	class UCurveFloat* DissolveCurve;
	
	UPROPERTY(EditDefaultsOnly, Category = "Effects | Dissolve")
	UNiagaraComponent* DissolveParticleComponent;

	UFUNCTION()
	void UpdateDissolveEffect(float Value);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnemyType")
	EEnemyType EnemyType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnemyState")
	EEnemyState EnemyState;
	
	UPROPERTY(VisibleAnywhere)
	bool isLaunched = false;

	UFUNCTION(BlueprintCallable)
	void ResetEnemy();
		
	UFUNCTION()		
	virtual void ReactToDamage(EMainDamageTypes DamageType, const FVector& ImpactPoint) {};
	
	UPROPERTY();
	EMainDamageTypes LastDamageType;

	FName SelectRandomDieAnim();

	UPROPERTY(EditAnywhere)
	UBehaviorTree* BTAsset;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Save System", meta = (DisplayName = "Unique Save ID"))
	FName UniqueSaveID;

	AAIController* AIController;
	
	class AEnemyAIController* EnemyAIController;

	virtual void GetDefaultParameters();

	virtual void SetOnPossessedParameters();

	class UBlackboardComponent* BBComponent;

private:
	UPROPERTY()
	FTimerHandle HitFlashTimerHandle;

	UFUNCTION(BlueprintCallable)
	void HandleEnemyCollision(ECollisionResponse CollisionResponse);
};