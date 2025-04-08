// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttributeComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TESISUE_API UAttributeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAttributeComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void ReceiveDamage(float Damage);

	UFUNCTION(BlueprintCallable)
	float GetHealthPercent();

	UFUNCTION(BlueprintCallable)
	bool IsAlive();

	UFUNCTION(BlueprintCallable)
	FORCEINLINE float GetEnergy() { return Energy; };

	UFUNCTION(BlueprintCallable)
	float GetEnergyPercent();

	UFUNCTION(BlueprintCallable)
	void IncreaseEnergy(float Amount);

	UFUNCTION(BlueprintCallable)
	void StartDecreaseEnergy();

	UFUNCTION(BlueprintCallable)
	void StopDecreaseEnergy();

	UFUNCTION(BlueprintCallable)
	bool ItHasEnergy();

	TFunction<void()> OnDepletedCallback;

protected:

	virtual void BeginPlay() override;

private:

	UPROPERTY(EditAnywhere, Category = "Actor Properties|Health");
	float Health;

	UPROPERTY(EditAnywhere, Category = "Actor Properties|Health");
	float MaxHealth;
	
	UPROPERTY(EditAnywhere, Category = "Actor Properties|Energy")
	float Energy;

	FTimerHandle EnergyTimerHandle;

	UPROPERTY()
	bool bIsDraining;

	void DrainTick();

};
