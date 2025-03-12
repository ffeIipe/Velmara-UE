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
	//Base
	virtual void BeginPlay() override;
	
	//Character states
	UPROPERTY(BlueprintReadWrite, Category = "LightAttack")
	int LightAttackIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "LightAttack")
	TArray<UAnimMontage*> LightAttackCombo;

	UPROPERTY(BlueprintReadWrite, Category = "LightAttack")
	bool IsSaveLightAttack;

	UFUNCTION(BlueprintCallable, Category = "FSM")
	void PerformLightAttack(int AttackIndex);

	UFUNCTION(BlueprintCallable, Category = "FSM")
	ECharacterStates SetCharacterState(ECharacterStates NewState);

	UFUNCTION(BlueprintPure, Category = "FSM")
	bool IsStateEqualToAny(const TArray<ECharacterStates>& StatesToCheck);

	//Inputs
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputMappingContext* CharacterContext;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* MoveAction;
	
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* LookAction;
	
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* JumpAction;
	
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* DodgeAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* InteractAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* AttackAction;

	//UFUNCTION(BlueprintImplementableEvent)
	//void LightAttackEvent;

private:	
	
	ECharacterStates CharacterState = ECharacterStates::ECS_Nothing;

	UPROPERTY(EditAnywhere)
	UCameraComponent* MainCam;

	UPROPERTY(EditAnywhere)
	USpringArmComponent* CameraBoom;

	UPROPERTY(EditDefaultsOnly, Category = "Montages")
	UAnimMontage* AttackMontage;

	void Move(const FInputActionValue& Value);

	void Look(const FInputActionValue& Value);

	//void Interact(const FInputActionValue& Value);
	
	void Attack(const FInputActionValue& Value);

	
};
