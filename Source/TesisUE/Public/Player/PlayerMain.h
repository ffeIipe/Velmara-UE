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

UCLASS()
class TESISUE_API APlayerMain : public ACharacter
{
	GENERATED_BODY()

public:

	APlayerMain();

	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintPure, Category = "FSM")
	FORCEINLINE ECharacterStates GetCharacterState() const { return CharacterState; }

protected:
	/*
	* Base
	*/
	virtual void BeginPlay() override;
	
	/*
	* FSM
	*/
	UFUNCTION(BlueprintCallable, Category = "FSM")
	ECharacterStates SetCharacterState(ECharacterStates NewState);

	UFUNCTION(BlueprintPure, Category = "FSM")
	bool IsStateEqualToAny(const TArray<ECharacterStates>& StatesToCheck);

	/*
	* Dodge
	*/
	UPROPERTY(BlueprintReadWrite, Category = "Dodge")
	UAnimMontage* DodgeMontage;

	UPROPERTY(BlueprintReadWrite, Category = "Dodge")
	bool bSaveDodge = false;

	UFUNCTION(BlueprintCallable, Category = "Dodge")
	void PerformDodge();

	UPROPERTY()
	class UTimelineComponent* BufferTimeline;

	UPROPERTY(EditAnywhere, Category = "Buffer")
	class UCurveFloat* BufferCurve;

	UPROPERTY(EditAnywhere, Category = "Buffer")
	float BufferDistance;

	UFUNCTION(BlueprintCallable)
	void BufferEvent(float BufferAmount);

	UFUNCTION(BlueprintCallable)
	void StopBufferEvent();

	UFUNCTION(BlueprintCallable)
	void UpdateBuffer(float Alpha);

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

private:	
	
	ECharacterStates CharacterState = ECharacterStates::ECS_Nothing;

	UPROPERTY(EditAnywhere)
	UCameraComponent* MainCam;

	UPROPERTY(EditAnywhere)
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleDefaultsOnly, Category = "Montages")
	UAnimMontage* AttackMontage;
	
	UPROPERTY(VisibleDefaultsOnly, Category = "Montages")
	UAnimMontage* HeavyAttackMontage;

	void Move(const FInputActionValue& Value);

	void Look(const FInputActionValue& Value);

	//void Interact(const FInputActionValue& Value);
	
	void Attack(const FInputActionValue& Value);
	
	void HeavyAttack(const FInputActionValue& Value);
};
