// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Entities/Entity.h"
#include "Enemy.generated.h"

class UCameraComponent;
class UInputMappingContext;
class AAIController;
class UPromptWidgetComponent;
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

	// === Pooling ===
	UFUNCTION(BlueprintCallable, Category = "Pooling")
	virtual void ActivateEnemy(const FVector& Location, const FRotator& Rotation);

	UFUNCTION(BlueprintCallable, Category = "Pooling")
	virtual void DeactivateEnemy();

	UPROPERTY(BlueprintAssignable, Category = "Pooling")
	FOnEnemyDeactivated OnDeactivated;

	UFUNCTION(BlueprintCallable)
	void RequestReturnToPool();

	// === Delegates ===
	UPROPERTY(BlueprintAssignable)
	FOnEntityDead OnDead;

	UPROPERTY(BlueprintAssignable)
	FOnEntityDamaged OnDamaged;

	// === Combat & Damage ===
	virtual void Die(UAnimMontage* DeathAnim, FName Section) override;

	virtual void GetHit(TScriptInterface<ICombatTargetInterface> DamageCauser, const FVector& ImpactPoint, FDamageEvent const& DamageEvent, const float DamageReceived) override;
	
	void DropOrbs(const float DamageReceived, const TScriptInterface<ICombatTargetInterface>& DamageCauser) const;

	void FinishedDamage();

	virtual bool IsLaunchable() override;

	void NotifyDamageTakenToBlackboard(const TScriptInterface<ICombatTargetInterface>& DamageCauser);

	// === AI & State ===
	FORCEINLINE EEnemyType GetEnemyType() const { return EnemyType; }

	FORCEINLINE EEnemyState GetEnemyState() const { return EnemyState; }

	UFUNCTION(BlueprintCallable)
	EEnemyState SetEnemyState(EEnemyState NewState);

	UFUNCTION(BlueprintCallable)
	void DisableAI();

	void ClearBlackboardValues();

	UFUNCTION(BlueprintCallable)
	void EnableAI();

	void NotifyThreat(const TScriptInterface<ICombatTargetInterface>& ThreatActor) const;

	AAIController* GetAIController() const { return AIController; };

	UFUNCTION()
	void OnPossessed();
	
	UFUNCTION()
	void OnUnpossessed();

	// === Components ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components | PromptWidgetComponent")
	UPromptWidgetComponent* PromptWidgetComponent;

	// === Status Flags ===
	bool bWasPossessed = false;

	//=== Implementable Events ===
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void ResetKeyBool(float Duration, struct FBlackboardKeySelector Key, bool SetBool, APawn* TargetPawn);

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	void PerformDead();
	
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// === AI ===
	UPROPERTY(EditAnywhere)
	float RadiusToNotifyAllies = 1700.f;

	UFUNCTION()
	void ReturnAttackTokenToTarget();

	UFUNCTION()
	void NotifyIsNotShieldedToBlackboard();

	TArray<TScriptInterface<ICombatTargetInterface>> GenerateSphereOverlapToDetectOtherEnemies(
		const FVector& Origin, float Radius, AActor* HitEnemyToExclude);

	// === Combat & Damage ===
	UFUNCTION()
	void EnableFinisherWidget();

	UPROPERTY(EditAnywhere, Category = "DamageThreshold");
	float DamageThreshold = 30.f;

	UPROPERTY();
	EMeleeDamageTypes LastDamageType;

	UFUNCTION(BlueprintCallable)
	virtual void ReactToDamage(EMeleeDamageTypes DamageType, const FVector& ImpactPoint) {};

	// === Energy & Orbs ===
	UPROPERTY(EditAnywhere, Category = "Energy | Energy Drop");
	float MinEnergy = 1.f;

	UPROPERTY(EditAnywhere, Category = "Energy | Energy Drop");
	float MaxEnergy = 3.f;

	UPROPERTY(EditAnywhere, Category = "Energy | Energy Tax");
	float UnpossesEnergyTax = 3.f;

	UPROPERTY(EditAnywhere, Category = "Energy | Energy Tax");
	float ReleaseAndExecuteEnergyTax = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = "Energy | OnPossession")
	float EnergyDivider = 2.f;

	// === Animation Montages ===
	UPROPERTY(EditDefaultsOnly, Category = "Montages");
	UAnimMontage* FinisherDeathMontage;

	static FName SelectRandomDieAnim();

	// === Dissolve Effect ===
	UPROPERTY(EditAnywhere, Category = "Effects | Dissolve")
	TArray<UMaterialInstanceDynamic*> DissolveMaterials;

	UPROPERTY()
	UTimelineComponent* DissolveTimeline;

	UPROPERTY(EditDefaultsOnly, Category = "Effects | Dissolve")
	UCurveFloat* DissolveCurve;

	UPROPERTY(EditDefaultsOnly, Category = "Effects | Dissolve")
	UNiagaraComponent* DissolveParticleComponent;

	UFUNCTION()
	void UpdateDissolveEffect(float Value);
	
	// === HitFlash Effect ===
	void HitFlash(float Duration, float Amount);

	void DeactivateHitFlash();

	UPROPERTY()
	FTimerHandle HitFlashTimerHandle;
	

	// === Enemy Properties ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnemyType")
	EEnemyType EnemyType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnemyState")
	EEnemyState EnemyState;

	UPROPERTY(VisibleAnywhere)
	bool bIsLaunched = false;

	// === Resetting & Defaults ===
	UFUNCTION(BlueprintCallable)
	void ResetEnemy();

	// === Initial Component States ===
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

	UPROPERTY()
	bool bShouldDropOrbs = true; //this is for inheritance xd
	
	virtual void ApplyPossessionParameters(bool bShouldEnable) {};

	// === AI Controllers & Blackboard ===
	UPROPERTY(EditAnywhere)
	UBehaviorTree* BTAsset;

	UPROPERTY()
	AAIController* AIController;

	UPROPERTY()
	class AEnemyAIController* EnemyAIController;

	UPROPERTY()
	class UBlackboardComponent* BBComponent;

	// === Timers ===
	FTimerHandle ReturnToPoolTimerHandle;

	UFUNCTION(BlueprintCallable)
	virtual void HandleEnemyCollision(bool bEnable);

private:
	UFUNCTION()
	void GetExecuted();
};