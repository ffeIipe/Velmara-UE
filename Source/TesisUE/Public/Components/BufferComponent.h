// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BufferComponent.generated.h"

DECLARE_DYNAMIC_DELEGATE(FOnBufferStarted);
DECLARE_DYNAMIC_DELEGATE(FOnBufferFinished);
DECLARE_DYNAMIC_DELEGATE(FOnBufferStopped);

class UTimelineComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TESISUE_API UBufferComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBufferComponent();

	UFUNCTION(BlueprintCallable, meta=(AutoCreateRefTerm = "EventToCall"))
	void StartLocationBuffer(const float Distance, UCurveFloat* Curve, bool bIsCameraForwardVectorUsed, FOnBufferStarted OnBufferStarted, FOnBufferFinished
	                         OnBufferFinished, FOnBufferStopped OnBufferStopped);

	UFUNCTION(BlueprintCallable)
	void StopLocationBuffer();
	
	/*void StartRotationBuffer(const TObjectPtr<UCurveFloat>& Curve);
	void StopRotationBuffer() const;*/
	
	bool bIsCameraForwardVector = false;

	FOnBufferStarted OnBufferStarted_Internal;
	
	FOnBufferFinished OnBufferFinished_Internal;
	
	FOnBufferStopped OnBufferStopped_Internal;
	
protected:
	/*virtual void BeginPlay() override;*/

private:
	UFUNCTION()
	void UpdateLocationBuffer(float Alpha);

	UFUNCTION()
	void SetDefaultMovement();
	
	/*UFUNCTION()
	void UpdateRotationBuffer(float Alpha);*/
	
	UPROPERTY()
	TObjectPtr<UTimelineComponent> BufferTimelineComp;
	
	float CurrentDistance;
	float BufferMultiplier = 1.f;

	UPROPERTY()
	UCurveFloat* CurrentCurve = nullptr;
};
