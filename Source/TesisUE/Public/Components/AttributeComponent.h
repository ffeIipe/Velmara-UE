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

	UFUNCTION(BlueprintCallable, Category = "Health | Getters")
	float GetHealthPercent();
	
	UFUNCTION(BlueprintCallable, Category = "Health | Getters")
	FORCEINLINE float GetHealth() { return Health; };
	
	UFUNCTION(BlueprintCallable, Category = "Health | Setters")
	FORCEINLINE void SetHealth(float Amount) { Health = Amount; };

	UFUNCTION(BlueprintCallable, Category = "Health")
	bool IsAlive();

	UFUNCTION(BlueprintCallable, Category = "Energy | Getters")
	FORCEINLINE float GetEnergy() { return Energy; };

	UFUNCTION(BlueprintCallable, Category = "Energy | Getters")
	float GetEnergyPercent();

	UFUNCTION(BlueprintCallable, Category = "Energy | Setters")
	void IncreaseEnergy(float Amount);
	
	UFUNCTION(BlueprintCallable, Category = "Energy | Setters")
	FORCEINLINE void SetEnergy(float Amount) { Energy = Amount; };

	UFUNCTION(BlueprintCallable, Category = "Energy")
	void StartDecreaseEnergy();

	UFUNCTION(BlueprintCallable, Category = "Energy")
	void StopDecreaseEnergy();

	UFUNCTION(BlueprintCallable, Category = "Energy")
	bool ItHasEnergy();
	
	UFUNCTION(BlueprintCallable, Category = "Energy")
	bool ItHasFullEnergy();

	UFUNCTION(BlueprintCallable, Category = "Energy")
	void RegenerateTick();

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
	
	UPROPERTY(EditAnywhere, Category = "Actor Properties|Energy")
	float DrainTickValue = 2.f;

	UPROPERTY(EditAnywhere, Category = "Actor Properties|Energy")
	float RegenerateTickValue = .5f;
	

	FTimerHandle EnergyDecreaseTimerHandle;
	
	FTimerHandle EnergyRegenerateTimerHandle;

	UPROPERTY()
	bool bIsDraining;

	void DrainTick();

	void RegenerateEnergy();
};
