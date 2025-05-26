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

	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Health | Getters")
	float GetHealthPercent();
	
	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Health | Getters")
	FORCEINLINE float GetHealth() { return Health; };
	
	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Health | Setters")
	FORCEINLINE void SetHealth(float Amount) { Health = Amount; };

	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Health")
	bool IsAlive();

	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Energy | Getters")
	FORCEINLINE float GetEnergy() { return Energy; };

	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Energy | Getters")
	float GetEnergyPercent();

	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Energy | Setters")
	void IncreaseEnergy(float Amount);
	
	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Energy | Setters")
	FORCEINLINE void SetEnergy(float Amount) { Energy = Amount; };

	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Energy")
	void StartDecreaseEnergy();

	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Energy")
	void StopDecreaseEnergy();

	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Energy")
	bool ItHasEnergy();
	
	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Energy")
	bool ItHasFullEnergy();

	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Energy")
	void RegenerateTick();
	
	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Energy")
	void StopRegenerateTick();

	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Energy")
	bool RequiresEnergy(float EnergyRequired);

	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Energy")
	void DecreaseEnergyBy(float EnergyToDecrease);

	TFunction<void()> OnDepletedCallback;

	void ReceiveShieldDamage(float Damage);
	
	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Shield")
	FORCEINLINE UStaticMeshComponent* GetShieldMeshComponent() { return ShieldMeshComponent; };

	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Shield")
	void AttachShield(USceneComponent* InParent, FName SocketName);
	
	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Shield")
	void DettachShield();

	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Shield")
	bool IsShielded();

	UPROPERTY(EditAnywhere, Category = "Actor Properties | Shield")
	bool bIsDisarmed = false;

	void ResetAttributes();

	UFUNCTION(BlueprintImplementableEvent)
	void DissolveShield();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actor Properties | Shield")
	class UStaticMeshComponent* ShieldMeshComponent;

private:
	UPROPERTY(EditAnywhere, Category = "Actor Properties | Health");
	float Health;

	UPROPERTY(EditAnywhere, Category = "Actor Properties | Health");
	float MaxHealth;
	
	UPROPERTY(EditAnywhere, Category = "Actor Properties | Energy")
	float Energy;
	
	UPROPERTY(EditAnywhere, Category = "Actor Properties | Energy")
	float DrainTickValue = 2.f;

	UPROPERTY(EditAnywhere, Category = "Actor Properties | Energy")
	float RegenerateTickValue = .5f;

	UPROPERTY(EditAnywhere, Category = "Actor Properties | Shield")
	float MaxShieldHealth = 100.f;
	
	UPROPERTY(EditAnywhere, Category = "Actor Properties | Shield")
	float CurrentShieldHealth;
	
	UPROPERTY(EditAnywhere, Category = "Actor Properties | Shield")
	class UStaticMesh* ShieldMesh;

	FTimerHandle EnergyDecreaseTimerHandle;
	
	FTimerHandle EnergyRegenerateTimerHandle;

	UPROPERTY()
	bool bIsDraining;

	void DrainTick();

	void RegenerateEnergy();
};
