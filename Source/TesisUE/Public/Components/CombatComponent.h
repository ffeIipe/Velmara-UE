#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

struct FCombatData;
class UEntityData;
class UTimelineComponent;
class UCurveFloat;
class IFormInterface;
class UCharacterStateComponent;
class UExtraMovementComponent;
class ASword;
class AEntity;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAttackEnd);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TESISUE_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// --- Delegates ---
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWallHitSignature, const FHitResult&, HitResult);

	UPROPERTY(BlueprintAssignable, Category = "Collision")
	FOnWallHitSignature OnWallHit;

	UPROPERTY()
	AController* OwnerController;

	UCombatComponent();

	void InitializeValues(const FCombatData& CombatData);
	// --- Getters & Setters ---
	UFUNCTION(BlueprintCallable, Category = "SoftLock")
	FORCEINLINE AActor* GetSoftLockTarget() const { return SoftLockTarget; }

	UFUNCTION(BlueprintCallable, Category = "SoftLock")
	FORCEINLINE void RemoveSoftLockTarget() { SoftLockTarget = nullptr; }

	// --- State Management ---
	UFUNCTION(BlueprintCallable, Category = "Attack")
	void ResetState();

	// --- Component References ---
	IFormInterface* SpectralAttacks;

	UPROPERTY()
	UCharacterStateComponent* CharacterStateComponent;

	UPROPERTY()
	UExtraMovementComponent* ExtraMovementComponent;

	// --- Finisher Locations ---
	UPROPERTY(EditAnywhere, Category = "Attack | Finisher")
	USceneComponent* CameraFinisherLocation;

	// --- Input Functions ---
	void Input_Attack();
	void Input_HeavyAttack();
	void Input_Launch();
	void Input_Block();
	void Input_ReleaseBlock();
	void ChangeHardLockTarget();
	void ToggleHardLock();
	
	UFUNCTION() //ufunction bc its called by an event that executes when you cant possess
	void Input_Execute();
	
	// --- Attack Events ---
	UFUNCTION()
	void LightAttackEvent();

	UFUNCTION()
	void HeavyAttackEvent();

	UFUNCTION()
	void Execute();
	
	UPROPERTY(BlueprintAssignable)
	FOnAttackEnd OnAttackEnd;

	// --- Combat Utility Functions ---
	void StartLaunchingUp();

	UFUNCTION()
	void GetDirectionalReact(const FVector& ImpactPoint);

	UFUNCTION()
	void HitReactJumpToSection(FName Section);

	UFUNCTION()
	AEntity* SphereTraceForEnemies(const FVector& Start, const FVector& End);

	// --- Buffer Distance ---
	UFUNCTION(BlueprintCallable)
	void StartAttackBufferEvent(float BufferAmount);

	UFUNCTION(BlueprintCallable)
	void StopAttackBufferEvent();
	
	// --- Attack State Flags ---
	UPROPERTY(VisibleAnywhere, Category = "Attack | LightAttack")
	bool bIsSaveLightAttack;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack | JumpAttack")
	bool bIsLaunched = false;

	UPROPERTY(BlueprintReadOnly, Category = "Attack | HardLock")
	bool bIsHardLocking = false;
	
protected:
	virtual void BeginPlay() override;
	
	// --- Attack Logic Functions ---
	UFUNCTION(BlueprintCallable, Category = "Attack | LightAttack")
	void LightAttack(int AttackIndex);

	UFUNCTION(BlueprintCallable, Category = "Attack | LightAttack")
	void ResetLightAttackStats();

	UFUNCTION(BlueprintCallable, Category = "Attack | JumpAttack")
	void JumpAttack(int AttackIndex);

	UFUNCTION(BlueprintCallable, Category = "Attack | JumpAttack")
	void ResetJumpAttackStats();

	UFUNCTION(BlueprintCallable, Category = "Attack | HeavyAttack")
	void HeavyAttack(int AttackIndex);

	UFUNCTION(BlueprintCallable, Category = "Attack | HeavyAttack")
	void ResetHeavyAttackStats();

	UFUNCTION(BlueprintCallable, Category = "Attack | ComboAttack")
	void PerformComboStarter(int AttackIndex);

	UFUNCTION(BlueprintCallable, Category = "Attack | ComboAttack")
	void PerformComboExtender(int AttackIndex);

	UFUNCTION(BlueprintCallable, Category = "Attack | JumpAttack")
	void LaunchCharacterUp();

	UFUNCTION(BlueprintCallable, Category = "Attack | JumpAttack")
	void Crasher();
	
	UFUNCTION()
	bool PickHardLockTarget();

	UFUNCTION()
	bool IsValidAndAlive(const AEntity* TargetToCheck);
	
	UFUNCTION()
	TArray<AEntity*> GetHardLockTargets(const float Radius) const;

	UFUNCTION()
	void RotateTowardsHardLockTarget(const AEntity* HardLockTarget, float DeltaTime) const;
	
	// --- Blocking ---
	UFUNCTION()
	void Block();

	UFUNCTION()
	void ReceiveBlock();

	UFUNCTION()
	void ReleaseBlock();

	// --- Soft Lock Targeting ---
	UFUNCTION(BlueprintCallable, Category = "SoftLock")
	void SoftLockOn();

	UFUNCTION(BlueprintCallable, Category = "SoftLock")
	void ValidateWall();

	UFUNCTION(BlueprintCallable, Category = "SoftLock")
	void RotationToTarget();

	UFUNCTION(BlueprintCallable, Category = "SoftLock")
	void UpdateSoftLockOn(float Alpha);

	// --- Launch Character Timeline ---
	UFUNCTION(BlueprintCallable, Category = "LaunchCharacter")
	void UpdateLaunchCharacterUp(float Alpha);

	// --- Soft Lock Properties ---
	UPROPERTY(BlueprintReadWrite, Category = "SoftLock")
	AActor* SoftLockTarget = nullptr;

	// --- Launch Character Properties ---
	UPROPERTY(BlueprintReadOnly, Category = "LaunchCharacter")
	FVector CurrentLocationLaunch;

	UPROPERTY(BlueprintReadOnly, Category = "LaunchCharacter")
	FVector UpVectorLaunch;

	// --- Timelines ---
	UPROPERTY()
	UTimelineComponent* SoftLockTimeline;

	UPROPERTY()
	UTimelineComponent* LaunchCharacterTimeline;

	UPROPERTY()
	UTimelineComponent* BufferAttackTimeline;

	// --- Buffer Attack ---
	UFUNCTION(BlueprintCallable)
	void UpdateAttackBuffer(float Alpha);

	UFUNCTION(BlueprintCallable)
	void UpdateBuffer(float Alpha, float BufferDistance);

	UPROPERTY(BlueprintReadWrite, Category = "Stats")
	float BufferMultiplier;
	
	// --- Animation Montages ---
	UPROPERTY(EditDefaultsOnly, Category = "Montages | ComboAttack")
	TArray<UAnimMontage*> ComboStarterAttack;

	UPROPERTY(EditDefaultsOnly, Category = "Montages | ComboAttack")
	TArray<UAnimMontage*> ComboExtenderAttack;

	// --- Tick Events ---
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	
private:
	// --- Internal State Variables ---
	UPROPERTY()
	ACharacter* OwningCharacter;
	
	int LightAttackIndex = 0;
	int JumpAttackIndex = 0;
	int HeavyAttackIndex = 0;
	int ComboExtenderIndex = 0;

	bool bIsSaveHeavyAttack;

	int HardLockTargetIndex = 0;
	float HardLockRadius = 1500.f;

	UPROPERTY()
	AEntity* CurrentHardLockTarget;
	
	UPROPERTY()
	TArray<AEntity*> HardLockTargets;
	
	// --- Internal Utility Functions ---
	bool CanAttack();

	bool CheckDistance(const FVector& Origin, const FVector& Target, float DistanceToCheck);
	
	// --- Save Attack Events ---
	UFUNCTION(BlueprintCallable, Category = "Attack | LightAttack", meta = (AllowPrivateAccess = "true"))
	void SaveLightAttackEvent();

	UFUNCTION(BlueprintCallable, Category = "Attack | HeavyAttack", meta = (AllowPrivateAccess = "true"))
	void SaveHeavyAttackEvent();

	UFUNCTION(BlueprintCallable, Category = "Attack | SaveAttack", meta = (AllowPrivateAccess = "true"))
	void ResetAttackSave();

	// --- Component References ---
	ASword* GetCurrentSword();

	UPROPERTY()
	AEntity* EntityOwner;
	
	// --- Stats Assigned By Data Asset ---
	float SoftLockDistance;
	float SoftLockRadius;
	float TrackTargetRadius;
	float BufferAttackDistance;
	
	UPROPERTY()
	UCurveFloat* SoftLockCurve;
	UPROPERTY()
	UCurveFloat* BufferCurve;
	UPROPERTY()
	TArray<UAnimMontage*> LightAttackCombo;
	UPROPERTY()
	TArray<UAnimMontage*> JumpAttackCombo;
	UPROPERTY()
	TArray<UAnimMontage*> HeavyAttackCombo;
	UPROPERTY()
	UAnimMontage* BlockMontage;
	UPROPERTY()
	UAnimMontage* FinisherMontage;
	UPROPERTY()
	UAnimMontage* CrasherMontage;
	UPROPERTY()
	UAnimMontage* LaunchMontage;
	UPROPERTY()
	UAnimMontage* HitReactMontage;
};