// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CharacterStates.h"
#include "PlayerMain.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class UTimelineComponent;
class AItem;
class ASword;
class UPlayerFormComponent;
class AEnemy;
class ASpectralObjectInteractable;
class UAttributeComponent;
class UEnergy;

UCLASS()
class TESISUE_API APlayerMain : public ACharacter
{
	GENERATED_BODY()

public:

	APlayerMain();

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintPure, Category = "FSM")
	FORCEINLINE ECharacterActions GetCharacterAction() const { return CharacterAction; }
	
	UFUNCTION(BlueprintPure, Category = "FSM")
	FORCEINLINE ECharacterStates GetCharacterState() const { return CharacterState; }

	UFUNCTION(BlueprintPure, Category = "SpectralAttack")
	FORCEINLINE AEnemy* GetSpectralTarget() const { return SpectralTarget; }
	
	UFUNCTION(BlueprintCallable, Category = "SoftLock")
	FORCEINLINE AActor* GetSoftLockTarget() const { return SoftLockTarget; }
	
	FORCEINLINE void SetOverlappingItem(AItem* Item) { OverlappingItem = Item; }

	UFUNCTION(BlueprintCallable)
	void SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled);

	UPROPERTY(VisibleAnywhere, Category = "Forms")
	UPlayerFormComponent* PlayerFormComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	bool CanDoubleJump = true;

	/*
	*  TakeDamage
	*/
	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable)
	void ReleasePossession();

	void ReceiveBlock();

protected:
	/*
	* Base
	*/
	virtual void BeginPlay() override;
	
	/*
	* FSM
	*/
	UFUNCTION(BlueprintCallable, Category = "FSM")
	ECharacterActions SetCharacterState(ECharacterActions NewState);

	UFUNCTION(BlueprintPure, Category = "FSM")
	bool IsActionEqualToAny(const TArray<ECharacterActions>& StatesToCheck);
	
	UFUNCTION(BlueprintPure, Category = "FSM")
	bool IsStateEqualToAny(const TArray<ECharacterStates>& StatesToCheck);
	
	UFUNCTION(BlueprintPure, Category = "PlayerForm")
	bool IsFormEqualToAny(const TArray<ECharacterForm>& StatesToCheck);

	/*
	* Interact
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpectralMode")
	float SpectralInteractDistance;

	/*
	* Dodge
	*/
	UPROPERTY(BlueprintReadWrite, Category = "Dodge")
	UAnimMontage* DodgeMontage;

	UPROPERTY(BlueprintReadWrite, Category = "Dodge")
	UAnimMontage* SpectralDodgeMontage;
	
	UPROPERTY(BlueprintReadWrite, Category = "DoubleJump")
	UAnimMontage* DoubleJumpMontage;

	UPROPERTY(BlueprintReadWrite, Category = "HitReact")
	UAnimMontage* HitReactMontage;
	
	UPROPERTY(BlueprintReadWrite, Category = "Death")
	UAnimMontage* DeathMontage;
	
	UPROPERTY(BlueprintReadWrite, Category = "Block")
	UAnimMontage* BlockMontage;
	
	UPROPERTY(BlueprintReadWrite, Category = "Crasher")
	UAnimMontage* CrasherMontage;

	UPROPERTY(BlueprintReadWrite, Category = "Dodge")
	bool bSaveDodge = false;

	UFUNCTION(BlueprintCallable, Category = "Dodge")
	void PerformDodge();

	UPROPERTY()
	UTimelineComponent* BufferDodgeTimeline;
	
	UPROPERTY()
	UTimelineComponent* BufferAttackTimeline;

	UPROPERTY(EditAnywhere, Category = "Buffer")
	class UCurveFloat* BufferCurve;

	UPROPERTY(EditAnywhere, Category = "Buffer")
	float BufferDodgeDistance;
	
	UPROPERTY(EditAnywhere, Category = "Buffer")
	float BufferAttackDistance;

	UFUNCTION(BlueprintCallable)
	void DodgeBufferEvent(float BufferAmount);
	
	UFUNCTION(BlueprintCallable)
	void StartAttackBufferEvent(float BufferAmount);

	UFUNCTION(BlueprintCallable)
	void StopDodgeBufferEvent();
	
	UFUNCTION(BlueprintCallable)
	void StopAttackBufferEvent();

	UFUNCTION(BlueprintCallable)
	void UpdateDodgeBuffer(float Alpha);
	
	UFUNCTION(BlueprintCallable)
	void UpdateAttackBuffer(float Alpha);

	UFUNCTION(BlueprintCallable)
	void UpdateBuffer(float Alpha, float BufferDistance);

	/*
	* Light Attack
	*/
	UPROPERTY(BlueprintReadWrite, Category = "LightAttack")
	int LightAttackIndex = 0;
	
	UPROPERTY(BlueprintReadWrite, Category = "SpectralMode | SpectralAttack")
	int SpectralAttackIndex = 0;

	UPROPERTY(BlueprintReadWrite, Category = "LightAttack")
	int JumpAttackIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "LightAttack")
	TArray<UAnimMontage*> LightAttackCombo;
	
	UPROPERTY(BlueprintReadOnly, Category = "SpectralMode | SpectralAttack")
	TArray<UAnimMontage*> SpectralAttackCombo;

	UPROPERTY(BlueprintReadOnly, Category = "JumpAttack")
	TArray<UAnimMontage*> JumpAttackCombo;

	UPROPERTY(BlueprintReadWrite, Category = "LightAttack")
	bool IsSaveLightAttack;

	UFUNCTION(BlueprintCallable, Category = "LightAttack")
	void PerformLightAttack(int AttackIndex);

	UFUNCTION(BlueprintCallable, Category = "SpectralMode | SpectralAttack")
	void PerformSpectralAttack(int AttackIndex);
	
	UFUNCTION(BlueprintCallable, Category = "SpectralMode | SpectralAttack")
	void PerformSpectralBarrier();

	UFUNCTION(BlueprintCallable, Category = "LightAttack")
	void ResetLightAttackStats();
	
	UFUNCTION(BlueprintCallable, Category = "SpectralMode | SpectralAttack")
	void ResetSpectralAttackStats();
	
	UFUNCTION(BlueprintCallable, Category = "JumpAttack")
	void ResetJumpAttackStats();
	
	/*
	* Heavy Attack
	*/
	UPROPERTY(BlueprintReadWrite, Category = "HeavyAttack")
	int HeavyAttackIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "HeavyAttack")
	TArray<UAnimMontage*> HeavyAttackCombo;

	UPROPERTY(BlueprintReadOnly, Category = "SpectralMode | SpectralAttack")
	UAnimMontage* SpectralHeavyAttack;

	UPROPERTY(BlueprintReadWrite, Category = "HeavyAttack")
	bool IsSaveHeavyAttack;

	UFUNCTION(BlueprintCallable, Category = "HeavyAttack")
	void PerformHeavyAttack(int AttackIndex);

	UFUNCTION(BlueprintCallable, Category = "HeavyAttack")
	void ResetHeavyAttackStats();

	UFUNCTION(BlueprintCallable, Category = "JumpAttack")
	void PerformJumpAttack(int AttackIndex);

	UFUNCTION(BlueprintCallable, Category = "SoftLockOn")
	void SoftLockOn();
	
	UFUNCTION(BlueprintCallable, Category = "SoftLockOn")
	void RotationToTarget();

	UFUNCTION(BlueprintCallable, Category = "SoftLockOn")
	void UpdateSoftLockOn(float Alpha);

	UPROPERTY(BlueprintReadWrite, Category = "SoftLockOn")
	AActor* SoftLockTarget;

	UPROPERTY(EditAnywhere, Category = "SoftLockOn")
	float SoftLockDistance;

	UPROPERTY(EditAnywhere, Category = "SoftLockOn")
	float SoftLockRadius;

	UPROPERTY()
	UTimelineComponent* SoftLockTimeline;
	
	UPROPERTY(EditAnywhere, Category = "SoftLockOn")
	class UCurveFloat* SoftLockCurve;

	UPROPERTY(EditAnywhere, Category = "SpectralAttack")
	AEnemy* SpectralTarget;

	UPROPERTY(EditAnywhere, Category = "SpectralAttack")
	float TrackTargetDistance;

	UPROPERTY(EditAnywhere, Category = "SpectralAttack")
	float TrackTargetRadius;

	UFUNCTION(BlueprintCallable, Category = "SpectralAttack")
	void SearchTarget();

	/*
	* Inputs
	*/
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputMappingContext* CharacterContext;

	UPROPERTY(EditAnywhere, Category = "Input | Movement")
	UInputAction* MoveAction;
	
	UPROPERTY(EditAnywhere, Category = "Input | Movement")
	UInputAction* LookAction;
	
	UPROPERTY(EditAnywhere, Category = "Input | Movement")
	UInputAction* JumpAction;
	
	UPROPERTY(EditAnywhere, Category = "Input | Movement")
	UInputAction* DodgeAction;
	
	UPROPERTY(EditAnywhere, Category = "Input | Movement")
	UInputAction* CrouchAction;

	UPROPERTY(EditAnywhere, Category = "Input | Actions")
	UInputAction* InteractAction;

	UPROPERTY(EditAnywhere, Category = "Input | Actions")
	UInputAction* AttackAction;
	
	UPROPERTY(EditAnywhere, Category = "Input | Actions")
	UInputAction* HeavyAttackAction;

	UPROPERTY(EditAnywhere, Category = "Input | Actions")
	UInputAction* ChangeFormAction;
	
	UPROPERTY(EditAnywhere, Category = "Input | Actions")
	UInputAction* BlockAction;
	
	UPROPERTY(EditAnywhere, Category = "Input | Actions")
	UInputAction* PossessAction;
	
	UPROPERTY(EditAnywhere, Category = "Input | Actions")
	UInputAction* RestartAction;
	
	UPROPERTY(EditAnywhere, Category = "Input | Actions")
	UInputAction* GoToMenuAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USpringArmComponent* CameraBoom;

private:	

	APlayerController* PlayerControllerRef = nullptr;

	bool bIsDead = false;

	UPROPERTY(VisibleAnywhere);
	UAttributeComponent* Attributes;

	ECharacterActions CharacterAction = ECharacterActions::ECA_Nothing;

	ECharacterStates CharacterState = ECharacterStates::ECS_Unequipped;
	
	UPROPERTY(EditAnywhere)
	ACameraActor* FollowCamera;


	UPROPERTY(VisibleInstanceOnly)
	AItem* OverlappingItem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess))
	ASword* EquippedWeapon;
	
	UFUNCTION(BlueprintCallable)
	void HitStop(float Duration, float TimeScale);
	
	UFUNCTION(BlueprintCallable)
	void ResetTimeDilation();
	
	bool bIsPossessing = false;
	FVector StoredLocation;
	FRotator StoredRotation;
	AEnemy* PossessedEnemy;

	UFUNCTION(BlueprintCallable)
	AEnemy* GetTargetEnemy();
	
	UFUNCTION(BlueprintCallable)
	void PossessEnemy();

	void Move(const FInputActionValue& Value);

	void Look(const FInputActionValue& Value);

	void Jump() override;

	void DoubleJump();

	void Landed(const FHitResult& Hit) override;

	void Interact(const FInputActionValue& Value);
	
	void Attack(const FInputActionValue& Value);
	
	void HeavyAttack(const FInputActionValue& Value);

	void ToggleForm();

	void Die();

	void ResetPlayer();

	void GetDirectionalReact();

	void Block();

	void ReleaseBlock();

	UFUNCTION(BlueprintCallable)
	void LaunchCharacterUp();
	
	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	bool isLaunched = false;

	UFUNCTION(BlueprintCallable)
	void Crasher();

	AActor* SphereTraceForEnemies(FVector Start, FVector End);

	UFUNCTION()
	void RestartLevel();
	
	UFUNCTION()
	void GoToMainMenu();
};