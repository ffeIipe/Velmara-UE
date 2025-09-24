// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ExtraMovementComponent.generated.h"

class IStrategyProvider;
class ICharacterMovementProvider;
class ICharacterStateProvider;
class IAnimatorProvider;
class IOwnerUtilsInterface;
struct FMovementData;
class AEntity;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDodgeStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDodgeSaved);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDoubleJumpStarted);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TESISUE_API UExtraMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UExtraMovementComponent();
	
	UFUNCTION()
	void ResetDodge() { bIsSaveDodge = false; }

	void PerformDoubleJump(UAnimMontage* DoubleJumpMontage);
	void PerformMove(const FVector2D& MoveVector, bool bIsTriggered);
	void PerformLook(const FVector2D& LookingVector) const;

	UFUNCTION(BlueprintCallable)
	void DodgeSaveEvent();
	
	bool IsMovingBackward() const;

	bool IsMoving() const { return bIsMoving; }

	UFUNCTION(BlueprintCallable)
	void PlayDodgeAnim(UAnimMontage* DodgeMontage) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	bool CanDoubleJump = true;
	float DoubleJumpForce = 800.f;
	bool bIsSaveDodge = false;

	FOnDodgeStarted OnDodgeStarted;
	FOnDodgeSaved OnDodgeSaved;
	FOnDoubleJumpStarted OnDoubleJumpStarted;
	
private:
	virtual void BeginPlay() override;
	
	void DodgeAnimBasedOnInput(UAnimMontage* DodgeMontage) const;

	bool bIsMoving = false;
	FVector2D CurrentMoveVector = FVector2D::ZeroVector;
	
	TScriptInterface<IAnimatorProvider> AnimatorProvider;
	TScriptInterface<ICharacterStateProvider> CharacterStateProvider;
	TScriptInterface<ICharacterMovementProvider> CharacterMovementProvider;
};
