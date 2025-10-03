// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SceneEvents/Trigger.h"
#include "SpectralTrap.generated.h"

class IHitInterface;
class APlayerMain;

UCLASS()
class TESISUE_API ASpectralTrap : public ATrigger
{
	GENERATED_BODY()

public:
	ASpectralTrap();
	
protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, Category = "Properties | Trap")
	float MaxDamage = 10.f;

	UPROPERTY(EditInstanceOnly, Category = "Properties | Trap")
	float InitialDamage = 1.0f;
	
	UPROPERTY(EditInstanceOnly, Category = "Properties | Trap")
	float DamageInterval = .2f;

	UPROPERTY(EditInstanceOnly, Category = "Properties | Trap")
	UCurveFloat* DamageCurve = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Properties | Trap")
	float TimeOnTarget = 0.0f;
	
	UPROPERTY(EditAnywhere, Category = "Properties | SFX")
	USoundBase* TrapSFX;

	FTimerHandle ContinuousDamageTimerHandle;

	UFUNCTION()
	void StartDamage();
	
	UFUNCTION()
	void FinishDamage();
	
	void DealContinuousDamage();
};
