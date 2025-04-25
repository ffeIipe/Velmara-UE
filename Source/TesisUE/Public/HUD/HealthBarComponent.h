// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "HealthBarComponent.generated.h"

/**
 * 
 */
UCLASS()
class TESISUE_API UHealthBarComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	UPROPERTY()
	class UHealthBar* HealthBarWidget;

	virtual void BeginPlay() override;
	
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	void SetHealthPercent(float Percent);

	void SetHealthBarActive(bool Param);

	void SetHealthOpacity(float DeltaTime);

	UPROPERTY(EditAnywhere, Category = "HealthBar")
	float MaxDistance = 1000.f;

	UPROPERTY(EditAnywhere, Category = "HealthBar")
	float MinDistance = 300.f; 

	UPROPERTY()
	AActor* PlayerMain;

	float TargetOpacity = 0.f;
	float CurrentOpacity = 0.f;
};
