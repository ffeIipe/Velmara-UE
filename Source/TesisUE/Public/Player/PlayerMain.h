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
	
	FORCEINLINE void SetOverlappingItem(AItem* Item) { OverlappingItem = Item; }

	UFUNCTION(BlueprintCallable)
	void SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled);

	UPROPERTY(VisibleAnywhere, Category = "Forms")
	UPlayerFormComponent* PlayerFormComponent;

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

	/*
	* Dodge
	*/
	UPROPERTY(BlueprintReadWrite, Category = "Dodge")
	UAnimMontage* DodgeMontage;

	UPROPERTY(BlueprintReadWrite, Category = "Dodge")
	UAnimMontage* SpectralDodgeMontage;

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
	void AttackBufferEvent(float BufferAmount);

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

	UPROPERTY(BlueprintReadOnly, Category = "LightAttack")
	TArray<UAnimMontage*> LightAttackCombo;

	UPROPERTY(BlueprintReadWrite, Category = "LightAttack")
	bool IsSaveLightAttack;

	UFUNCTION(BlueprintCallable, Category = "LightAttack")
	void PerformLightAttack(int AttackIndex);

	UFUNCTION(BlueprintCallable, Category = "LightAttack")
	void ResetLightAttackStats();
	
	/*
	* Heavy Attack
	*/
	UPROPERTY(BlueprintReadWrite, Category = "HeavyAttack")
	int HeavyAttackIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "HeavyAttack")
	TArray<UAnimMontage*> HeavyAttackCombo;

	UPROPERTY(BlueprintReadWrite, Category = "HeavyAttack")
	bool IsSaveHeavyAttack;

	UFUNCTION(BlueprintCallable, Category = "HeavyAttack")
	void PerformHeavyAttack(int AttackIndex);

	UFUNCTION(BlueprintCallable, Category = "HeavyAttack")
	void ResetHeavyAttackStats();

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

private:	
	
	ECharacterActions CharacterAction = ECharacterActions::ECA_Nothing;
	ECharacterStates CharacterState = ECharacterStates::ECS_Unequipped;

	UPROPERTY(EditAnywhere)
	UCameraComponent* MainCam;

	UPROPERTY(EditAnywhere)
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleDefaultsOnly, Category = "Montages")
	UAnimMontage* AttackMontage;
	
	UPROPERTY(VisibleDefaultsOnly, Category = "Montages")
	UAnimMontage* HeavyAttackMontage;

	UPROPERTY(VisibleInstanceOnly)
	AItem* OverlappingItem;

	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	ASword* EquippedWeapon;



	void Move(const FInputActionValue& Value);

	void Look(const FInputActionValue& Value);

	void Interact(const FInputActionValue& Value);
	
	void Attack(const FInputActionValue& Value);
	
	void HeavyAttack(const FInputActionValue& Value);

	void ToggleForm();
};