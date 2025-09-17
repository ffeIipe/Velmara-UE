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
	
	/*void InitializeValues(const FMovementData& MovementData);*/

	void SetCurrentStrategyValues(float DodgeDistance, float DoubleJumpForce, UAnimMontage* NewDodgeMontage);
	
	UFUNCTION()
	void ResetDodge() { bIsSaveDodge = false; }

	void PerformDoubleJump(UAnimMontage* DoubleJumpMontage);
	void PerformMove(const FVector2D& MoveVector, bool bIsTriggered);
	void PerformLook(const FVector2D& LookingVector) const;
	void PerformDodge(float DodgeDistance, UAnimMontage* DodgeAnim);

	UPROPERTY()
	UAnimMontage* CurrentDodgeMontage = nullptr;

	UFUNCTION(BlueprintCallable)
	void DodgeSaveEvent();
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	bool CanDoubleJump = true;

	void CustomInitialize(AEntity* NewEntity);
	
	void InitializeValues(const FMovementData& MovementData);
	bool IsMovingBackwards() const;

	bool bIsSaveDodge = false;

	FOnDodgeStarted OnDodgeStarted;
	FOnDodgeSaved OnDodgeSaved;
	FOnDoubleJumpStarted OnDoubleJumpStarted;
	
private:
	virtual void BeginPlay() override;
	
	void PlayDodgeAnim(UAnimMontage* DodgeMontage) const;

	UFUNCTION()
	void DodgeBufferEvent() const;

	void StopDodgeBufferEvent();

	UFUNCTION()
	void UpdateDodgeBuffer(float Alpha);

	void DodgeAnimBasedOnInput(UAnimMontage* DodgeMontage) const;

	// === Flags ===
	bool bIsMoving;
	
	// === Stats ===
	FVector2D CurrentMoveVector;
	
	float DoubleJumpStrength = 800.f;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = true))
	float BufferDodgeDistance;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	UCurveFloat* BufferCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCurveFloat> AnotherBufferCurve;
	
	TScriptInterface<IOwnerUtilsInterface> OwnerUtils;
	TScriptInterface<IAnimatorProvider> AnimatorProvider;
	TScriptInterface<ICharacterStateProvider> CharacterStateProvider;
	TScriptInterface<ICharacterMovementProvider> CharacterMovementProvider;
	TScriptInterface<IStrategyProvider> StrategyProvider;

	UPROPERTY()
	class UTimelineComponent* BufferDodgeTimeline;
};
