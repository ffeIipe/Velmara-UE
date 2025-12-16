// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ExtraMovementComponent.generated.h"


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

	UPROPERTY(BlueprintReadWrite, Category = "Dodge")
	bool bIsSaveDodge = false;

	UFUNCTION(BlueprintCallable)
	void DodgeSaveEvent();

	UFUNCTION(BlueprintCallable, Category = "Dodge")
	void PerformDodge();

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

	UPROPERTY(EditAnywhere, Category = "Input | Movement")
	class UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, Category = "Input | Movement")
	class UInputAction* DodgeAction;

	UPROPERTY(EditAnywhere, Category = "Stats | DoubleJump")
	float LaunchStrenght = 800.f;

	ACharacter* OwningCharacter;

	class UCharacterStateComponent* OwnerCharacterStateComponent;

	void Input_DoubleJump();

	void Input_Dodge();

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	bool CanDoubleJump = true;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
};
