#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

class UTimelineComponent;
class UCurveFloat;
class IFormInterface;
class ICharacterState;
class UCharacterStateComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TESISUE_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWallHitSignature, const FHitResult&, HitResult);
	
	UPROPERTY(BlueprintAssignable, Category = "Collision")
	FOnWallHitSignature OnWallHit;

	UCombatComponent();

	UFUNCTION(BlueprintCallable, Category = "SoftLock")
	FORCEINLINE AActor* GetSoftLockTarget() const { return SoftLockTarget; }
	
	UFUNCTION(BlueprintCallable, Category = "SoftLock")
	FORCEINLINE void RemoveSoftLockTarget() { SoftLockTarget = nullptr; }
	
	UFUNCTION(BlueprintCallable, Category = "Attack")
	void ResetState();

	ICharacterState* CharacterStateInterface;

	IFormInterface* SpectralAttacks;

	UCharacterStateComponent* CharacterStateComponent;

	class UExtraMovementComponent* ExtraMovementComponent;

	UPROPERTY(EditAnywhere, Category = "Attack | Finisher")
	USceneComponent* FinisherLocation;

	UPROPERTY(EditAnywhere, Category = "Attack | Finisher")
	USceneComponent* CameraFinisherLocation;

protected:
	void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Attack | LightAttack")
	void LightAttack(int AttackIndex);

	UFUNCTION(BlueprintCallable, Category = "Attack | LightAttack")
	void ResetLightAttackStats();

	UFUNCTION(BlueprintCallable, Category = "Attack | JumpAttack")
	void ResetJumpAttackStats();

	UFUNCTION(BlueprintCallable, Category = "Attack | JumpAttack")
	void JumpAttack(int AttackIndex);

	UFUNCTION(BlueprintCallable, Category = "Attack | HeavyAttack")
	void HeavyAttack(int AttackIndex);

	UFUNCTION(BlueprintCallable, Category = "Attack | HeavyAttack")
	void ResetHeavyAttackStats();

	UFUNCTION(BlueprintCallable, Category = "Attack | ComboAttack")
	void PerformComboStarter(int AttackIndex);

	UFUNCTION(BlueprintCallable, Category = "Attack | ComboAttack")
	void PerformComboExtender(int AttackIndex);

	UFUNCTION(BlueprintCallable, Category = "SoftLock")
	void SoftLockOn();
	
	UFUNCTION(BlueprintCallable, Category = "SoftLock")
	void ValidateWall();

	UFUNCTION(BlueprintCallable, Category = "SoftLock")
	void RotationToTarget();

	UFUNCTION(BlueprintCallable, Category = "SoftLock")
	void UpdateSoftLockOn(float Alpha);

	UPROPERTY(BlueprintReadWrite, Category = "SoftLock")
	AActor* SoftLockTarget = nullptr;

	UPROPERTY(EditAnywhere, Category = "SoftLock")
	float SoftLockDistance;

	UPROPERTY(EditAnywhere, Category = "SoftLock")
	float SoftLockRadius;

	UPROPERTY()
	UTimelineComponent* SoftLockTimeline;

	UPROPERTY(EditAnywhere, Category = "SoftLock")
	UCurveFloat* SoftLockCurve;

	UFUNCTION(BlueprintCallable, Category = "Attack | JumpAttack")
	void LaunchCharacterUp();

	UFUNCTION(BlueprintCallable, Category = "Attack | JumpAttack")
	void Crasher();

	UFUNCTION()
	void Block();
	
	UFUNCTION()
	void ReceiveBlock();
	
	UFUNCTION()
	void ReleaseBlock();
	
public:
	UFUNCTION()
	void Execute();

	UFUNCTION()
	void GetDirectionalReact(const FVector& ImpactPoint, TSubclassOf<UDamageType> DamageType);
	
	UFUNCTION()
	void HitReactJumpToSection(FName Section);

	UFUNCTION()
	AActor* SphereTraceForEnemies(FVector Start, FVector End);

private:
	/*
	] Variables Section
	*/

	ACharacter* OwningCharacter;

	UPROPERTY(VisibleAnywhere, Category = "Attack | LightAttack")
	int LightAttackIndex = 0;

	UPROPERTY(VisibleAnywhere, Category = "Attack | JumpAttack")
	int JumpAttackIndex = 0;

	UPROPERTY(VisibleAnywhere, Category = "Attack | HeavyAttack")
	int HeavyAttackIndex = 0;

	UPROPERTY(VisibleAnywhere, Category = "Attack | ComboAttack")
	int ComboExtenderIndex = 0;

	bool CanAttack();

public:
	UPROPERTY(VisibleAnywhere, Category = "Attack | LightAttack")
	bool bIsSaveLightAttack;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack | JumpAttack")
	bool bIsLaunched = false;

private:
	UPROPERTY(VisibleAnywhere, Category = "Attack | HeavyAttack")
	bool bIsSaveHeavyAttack;

	UPROPERTY(EditAnywhere, Category = "SoftLock")
	float TrackTargetRadius;

	UPROPERTY(EditAnywhere, Category = "Attack | Buffer")
	float BufferAttackDistance;

	/*
	] Components Section
	*/
	UPROPERTY()
	UTimelineComponent* BufferAttackTimeline;

	UPROPERTY(EditAnywhere, Category = "Buffer")
	UCurveFloat* BufferCurve;

	/*
	] Buffer Section
	*/
	UFUNCTION(BlueprintCallable)
	void StartAttackBufferEvent(float BufferAmount);

	UFUNCTION(BlueprintCallable)
	void StopAttackBufferEvent();

	UFUNCTION(BlueprintCallable)
	void UpdateAttackBuffer(float Alpha);

	UFUNCTION(BlueprintCallable)
	void UpdateBuffer(float Alpha, float BufferDistance);

	/*
	] Animation Montages Section
	*/

	UPROPERTY(EditDefaultsOnly, Category = "Montages | LightAttack")
	TArray<UAnimMontage*> LightAttackCombo;

	UPROPERTY(EditDefaultsOnly, Category = "Montages | JumpAttack")
	TArray<UAnimMontage*> JumpAttackCombo;
	
	UPROPERTY(EditDefaultsOnly, Category = "Montages | HeavyAttack")
	TArray<UAnimMontage*> HeavyAttackCombo;

	UPROPERTY(EditDefaultsOnly, Category = "Montages | ComboAttack")
	TArray<UAnimMontage*> ComboStarterAttack;

	UPROPERTY(EditDefaultsOnly, Category = "Montages | ComboAttack")
	TArray<UAnimMontage*> ComboExtenderAttack;

	UPROPERTY(EditDefaultsOnly, Category = "Montages | Block")
	UAnimMontage* BlockMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Montages | Finisher")
	UAnimMontage* FinisherMontage;
		
	UPROPERTY(EditDefaultsOnly, Category = "Montages | Crasher")
	UAnimMontage* CrasherMontage;
	
	UPROPERTY(EditDefaultsOnly, Category = "Montages | Launch")
	UAnimMontage* LaunchMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Montages | HitReact")
	UAnimMontage* HitReactMontage;

public:
	/*
	] Section
	*/
	void Input_Attack();
	
	void Input_HeavyAttack();
	
	void Input_Launch();

	void Input_Block();
	
	void Input_ReleaseBlock();

	void Input_Execute();

	UFUNCTION()
	void LightAttackEvent();
	
	UFUNCTION()
	void HeavyAttackEvent();

private:
	UFUNCTION(BlueprintCallable, Category = "Attack | LightAttack", meta = (AllowPrivateAccess = "true"))
	void SaveLightAttackEvent();

	UFUNCTION(BlueprintCallable, Category = "Attack | HeavyAttack", meta = (AllowPrivateAccess = "true"))
	void SaveHeavyAttackEvent();
	
	UFUNCTION(BlueprintCallable, Category = "Attack | SaveAttack", meta = (AllowPrivateAccess = "true"))
	void ResetAttackSave();

	class ASword* GetCurrentSword();

	class AEntity* EntityOwner;
};
