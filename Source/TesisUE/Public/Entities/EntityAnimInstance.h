// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Interfaces/EntityAnimInstanceProvider.h"
#include "EntityAnimInstance.generated.h"

class AEntity;
enum class ECharacterWeaponStates : uint8;
enum class ECharacterModeStates : uint8;
class ICharacterStateProvider;
class ICharacterMovementProvider;
class IOwnerUtilsInterface;
/**
 * 
 */
UCLASS()
class TESISUE_API UEntityAnimInstance : public UAnimInstance, public IEntityAnimInstanceProvider
{
	GENERATED_BODY()
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaTime) override;

	UPROPERTY(BlueprintReadOnly)
	AEntity* EntityOwner;
	
	UPROPERTY(BlueprintReadOnly)
	TScriptInterface<IOwnerUtilsInterface> OwnerUtils;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	TScriptInterface<ICharacterMovementProvider> CharacterMovementProvider;

	TScriptInterface<ICharacterStateProvider> CharacterStateProvider;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float GroundSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	bool IsFalling;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bHasAcceleration;

	UPROPERTY(BlueprintReadWrite, Category = "Movement")
	float Direction;
	
	UPROPERTY(BlueprintReadWrite, Category = "Movement")
	float WalkRight;
	
	UPROPERTY(BlueprintReadWrite, Category = "Movement")
	float WalkForward;
	
	UPROPERTY(BlueprintReadWrite, Category = "Movement")
	float MaxWalkSpeed;

	UPROPERTY(BlueprintReadWrite, Category = "Combat")
	bool bIsLocking;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement | Character State")
	ECharacterWeaponStates CharacterWeaponState;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement | Character Form")
	ECharacterModeStates CharacterMode;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Movement | Animation States")
	bool bIsMeleeWeapon;
	
	virtual void SetAnimationState(TScriptInterface<IWeaponInterface> WeaponEquipped) override;
};
