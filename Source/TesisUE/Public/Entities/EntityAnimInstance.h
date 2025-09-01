// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "EntityAnimInstance.generated.h"

enum class ECharacterSpectralStates : uint8;
enum class ECharacterMode : uint8;
enum class ECharacterHumanStates : uint8;
class ICharacterStateProvider;
class ICharacterMovementProvider;
class IOwnerUtilsInterface;
/**
 * 
 */
UCLASS()
class TESISUE_API UEntityAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaTime) override;

	UPROPERTY()
	APawn* Owner;
	
	UPROPERTY(BlueprintReadOnly)
	TScriptInterface<IOwnerUtilsInterface> OwnerUtils;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	TScriptInterface<ICharacterMovementProvider> CharacterMovementProvider;

	TScriptInterface<ICharacterStateProvider> CharacterStateProvider;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float GroundSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool IsFalling;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bHasAcceleration;

	UPROPERTY(BlueprintReadWrite, Category = "Movement")
	float Direction;
	
	UPROPERTY(BlueprintReadWrite, Category = "Movement")
	float WalkRight;
	
	UPROPERTY(BlueprintReadWrite, Category = "Movement")
	float WalkForward;
	
	UPROPERTY(BlueprintReadWrite, Category = "Movement")
	float MaxWalkSpeed;
	
	UPROPERTY(BlueprintReadWrite, Category = "Movement")
	float MaxRunSpeed;

	UPROPERTY(BlueprintReadWrite, Category = "Combat")
	bool bIsLocking;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement | Character State")
	ECharacterHumanStates CharacterHumanState;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement | Character Form")
	ECharacterMode CharacterMode;

	UPROPERTY(BlueprintReadOnly, Category = "Movement | Spectral Weapon State")
	ECharacterSpectralStates SpectralState;
};
