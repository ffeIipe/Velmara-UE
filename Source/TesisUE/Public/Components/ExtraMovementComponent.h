// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ExtraMovementComponent.generated.h"

struct FInputActionValue;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TESISUE_API UExtraMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UExtraMovementComponent();

	UPROPERTY(EditAnywhere, Category = "Montages | Dodge")
	UAnimMontage* DodgeMontage;

	UPROPERTY(EditAnywhere, Category = "Montages | Dodge")
	UAnimMontage* SpectralDodgeMontage;

	UPROPERTY(EditAnywhere, Category = "Montages | Jump")
	UAnimMontage* JumpMontage;

	UPROPERTY(EditAnywhere, Category = "Montages | DoubleJump")
	UAnimMontage* DoubleJumpMontage;
	
	UPROPERTY(EditAnywhere, Category = "Montages | TurnInPlace")
	UAnimMontage* TurnInPlaceMontage;

	UPROPERTY(BlueprintReadWrite, Category = "Dodge")
	bool bIsSaveDodge = false;
	
	UPROPERTY(BlueprintReadWrite, Category = "Dodge")
	bool bIsTurningInPlace = false;

	UFUNCTION(BlueprintCallable)
	void PlayTurnInPlaceMontage(const FVector& DesiredInputDirection);
	
	FVector LastInputDirection;

	/*UFUNCTION(BlueprintCallable)
	void OnTurnMontageEnded(UAnimMontage* Montage, bool bInterrupted);*/

	UFUNCTION(BlueprintCallable)
	void DodgeSaveEvent();

	UFUNCTION(BlueprintCallable, Category = "Dodge")
	void PerformDodge();

	void DodgeAnimBasedOnInput();

	UPROPERTY()
	class UTimelineComponent* BufferDodgeTimeline;

	UPROPERTY(EditAnywhere, Category = "Stats | Buffer")
	class UCurveFloat* BufferCurve;

	UPROPERTY(EditAnywhere, Category = "Stats | Buffer")
	float BufferDodgeDistance;

	UFUNCTION(BlueprintCallable)
	void DodgeBufferEvent(float BufferAmount);

	UFUNCTION(BlueprintCallable)
	void StopDodgeBufferEvent();

	UFUNCTION(BlueprintCallable)
	void UpdateDodgeBuffer(float Alpha);

	UFUNCTION(BlueprintCallable)
	void UpdateBuffer(float Alpha, float BufferDistance);

	UPROPERTY(EditAnywhere, Category = "Stats | DoubleJump")
	float LaunchStrenght = 800.f;

	float DefaultWalkSpeed;
	
	UPROPERTY(EditAnywhere, Category = "Stats | Movement")
	float MaxRunSpeed = 800.f;
	
	UPROPERTY(EditAnywhere, Category = "Stats | Movement")
	float MaxStrafeSpeed = 650.f;

	class UCharacterStateComponent* OwnerCharacterStateComponent;
	
	void Input_Move(const FInputActionValue& Value);
	
	void Input_Look(const FInputActionValue& Value);

	void Input_Run(const FInputActionValue& Value);

	void Input_DoubleJump();

	void Input_Dodge();

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	bool CanDoubleJump = true;

	void CustomInitialize(AEntity* NewEntity, UCharacterStateComponent* NewOwnerCharStateComp);

protected:
	virtual void BeginPlay() override;

	class AEntity* EntityOwner;

private:
	void UpdateAllowRunStrafe();

	bool bAllowRun = true;
	bool bAllowRunStrafe = true;

	bool bIsRunInputPressed = false;

	FVector2D MoveVector;

	FVector LastMovementInput;
};
