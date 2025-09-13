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
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDoubleJumpStarted);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TESISUE_API UExtraMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UExtraMovementComponent();
	
	void InitializeValues(const FMovementData& MovementData);

	void SetCurrentStrategyValues(float DodgeDistance, float DoubleJumpForce, UAnimMontage* NewDodgeMontage);
	
	UFUNCTION()
	void ResetDodge() { bIsSaveDodge = false; }

	void PerformDoubleJump();
	void PerformMove(const FVector2D& MoveVector, bool bIsTriggered);
	void PerformLook(const FVector2D& LookingVector) const;
	void PerformDodge();

	UPROPERTY()
	UAnimMontage* CurrentDodgeMontage = nullptr;
	
	UPROPERTY(EditAnywhere, Category = "Montages | Jump")
	UAnimMontage* JumpMontage;

	UPROPERTY(EditAnywhere, Category = "Montages | DoubleJump")
	UAnimMontage* DoubleJumpMontage;

	UFUNCTION(BlueprintCallable)
	void DodgeSaveEvent();
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	bool CanDoubleJump = true;

	void CustomInitialize(AEntity* NewEntity);

	bool IsMovingBackwards() const;
	
	bool bIsSaveDodge = false;

	FOnDodgeStarted OnDodgeStarted;

	FOnDoubleJumpStarted OnDoubleJumpStarted;
	
private:
	virtual void BeginPlay() override;
	
	void PlayDodgeAnim() const;

	void DodgeBufferEvent() const;

	void StopDodgeBufferEvent() const;

	void UpdateDodgeBuffer(float Alpha) const;

	void UpdateBuffer(float Alpha, float BufferDistance) const;

	void DodgeAnimBasedOnInput() const;

	// === Stats ===
	FVector2D CurrentMoveVector;
	
	float DoubleJumpStrength = 800.f;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = true))
	float BufferDodgeDistance;
	
	UPROPERTY()
	UCurveFloat* DodgeCurve;
	
	TScriptInterface<IOwnerUtilsInterface> OwnerUtils;
	TScriptInterface<IAnimatorProvider> AnimatorProvider;
	TScriptInterface<ICharacterStateProvider> CharacterStateProvider;
	TScriptInterface<ICharacterMovementProvider> CharacterMovementProvider;
	TScriptInterface<IStrategyProvider> StrategyProvider;

	UPROPERTY()
	class UTimelineComponent* BufferDodgeTimeline;
};
