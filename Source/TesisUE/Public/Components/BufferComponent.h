// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BufferComponent.generated.h"


class UTimelineComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TESISUE_API UBufferComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBufferComponent();

	UFUNCTION(BlueprintCallable)
	void StartLocationBuffer(const float Distance, UCurveFloat* Curve);

	UFUNCTION(BlueprintCallable)
	void StopLocationBuffer() const;
	
	/*void StartRotationBuffer(const TObjectPtr<UCurveFloat>& Curve);
	void StopRotationBuffer() const;*/
	
protected:
	/*virtual void BeginPlay() override;*/

private:
	UFUNCTION()
	void UpdateLocationBuffer(float Alpha);

	/*UFUNCTION()
	void UpdateRotationBuffer(float Alpha);*/
	
	UPROPERTY()
	TObjectPtr<UTimelineComponent> BufferTimelineComp;
	
	float CurrentDistance;
	float BufferMultiplier = 1.f;

	UPROPERTY()
	UCurveFloat* CurrentCurve = nullptr;
};
