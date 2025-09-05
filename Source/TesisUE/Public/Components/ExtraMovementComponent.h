// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ExtraMovementComponent.generated.h"

class ICharacterStateProvider;
class IAnimatorProvider;
class IOwnerUtilsInterface;
struct FMovementData;
class AEntity;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDodgeStarted);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TESISUE_API UExtraMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UExtraMovementComponent();
	
	void InitializeValues(const FMovementData& MovementData);

	UFUNCTION()
	void ResetDodge() { bIsSaveDodge = false; }

	UPROPERTY(EditAnywhere, Category = "Montages | Dodge")
	UAnimMontage* DodgeMontage;

	UPROPERTY(EditAnywhere, Category = "Montages | Dodge")
	UAnimMontage* SpectralDodgeMontage;

	UPROPERTY(EditAnywhere, Category = "Montages | Jump")
	UAnimMontage* JumpMontage;

	UPROPERTY(EditAnywhere, Category = "Montages | DoubleJump")
	UAnimMontage* DoubleJumpMontage;

	UFUNCTION(BlueprintCallable)
	void DodgeSaveEvent();

	void PerformDodge();

	void DodgeAnimBasedOnInput() const;

	UPROPERTY()
	class UTimelineComponent* BufferDodgeTimeline;

	void DodgeBufferEvent(float BufferAmount) const;

	void StopDodgeBufferEvent() const;

	void UpdateDodgeBuffer(float Alpha);

	void UpdateBuffer(float Alpha, float BufferDistance) const;
	
	void Input_Move(const FInputActionValue& Value);
	
	void Input_Look(const FInputActionValue& Value);

	void Input_DoubleJump();

	void Input_Dodge();

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	bool CanDoubleJump = true;

	void CustomInitialize(AEntity* NewEntity);

	bool IsMovingBackwards() const;

	FOnDodgeStarted OnDodgeStarted;
		
private:
	virtual void BeginPlay() override;

	UPROPERTY()
	TScriptInterface<IOwnerUtilsInterface> OwnerUtils;
	UPROPERTY()
	TScriptInterface<IAnimatorProvider> AnimatorProvider;
	UPROPERTY()
	TScriptInterface<ICharacterStateProvider> CharacterStateProvider;
	UPROPERTY()
	TScriptInterface<class ICharacterMovementProvider> CharacterMovementProvider;

	bool bIsSaveDodge = false;
	
	FVector2D MoveVector;
	
	float DoubleJumpStrength = 800.f;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = true))
	float BufferDodgeDistance;
	
	UPROPERTY()
	UCurveFloat* DodgeCurve;
};
