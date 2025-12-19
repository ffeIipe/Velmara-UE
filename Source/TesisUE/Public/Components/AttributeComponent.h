// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "AttributeComponent.generated.h"

struct FAttributeData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDettachShieldSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEntityDeadSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEntityOutOfEnergy);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TESISUE_API UAttributeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAttributeComponent();

	void InitializeValues(const FAttributeData& AttributeData);

	bool RequiresEnergyForTag(FGameplayTag GameplayTag);
	
	void ConsumeEnergyForTag(FGameplayTag GameplayTag);

	UPROPERTY(BlueprintAssignable)
	FOnDettachShieldSignature OnDetachShield;

	UPROPERTY(BlueprintAssignable)
	FOnEntityDeadSignature OnEntityDead;

	UPROPERTY(BlueprintAssignable)
	FOnEntityOutOfEnergy OnOutOfEnergy;

	void ReceiveDamage(float Damage);

	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Health | Getters")
	float GetHealthPercent();

	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Health | Getters")
	FORCEINLINE float GetHealth() const { return Health; };

	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Health | Setters")
	FORCEINLINE void SetHealth(const float Amount) { Health = Amount; };

	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Health | Setters")
	FORCEINLINE void IncreaseHealth(const float Amount) { Health += Amount; };

	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Health")
	bool IsAlive();

	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Energy | Getters")
	FORCEINLINE float GetEnergy() const { return Energy; };

	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Energy | Getters")
	float GetEnergyPercent();

	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Energy | Setters")
	void IncreaseEnergy(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Energy | Setters")
	FORCEINLINE void SetEnergy(const float Amount) { Energy = Amount; };

	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Energy")
	void StartDecreaseEnergy();

	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Energy")
	void StopDecreaseEnergy();

	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Energy")
	bool HasEnergy();

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

	void ReceiveShieldDamage(float Damage);

	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Shield")
	FORCEINLINE UStaticMeshComponent* GetShieldMeshComponent() { return ShieldMeshComponent; };

	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Shield")
	FORCEINLINE float GetShieldHealthPercent() { return Shield / MaxShield; };

	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Shield")
	void AttachShield(USceneComponent* InParent, FName SocketName);

	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Shield")
	void DetachShield();

	UFUNCTION(BlueprintCallable, Category = "Actor Functions | Shield")
	bool IsShielded();

	UPROPERTY(EditAnywhere, Category = "Actor Properties | Shield")
	bool bIsDisarmed = false;

	void ResetAttributes();

	UFUNCTION(BlueprintImplementableEvent)
	void DissolveShield();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actor Properties | Shield")
	UStaticMeshComponent* ShieldMeshComponent;

private:
	UPROPERTY(SaveGame)
	float Health;

	UPROPERTY(SaveGame)
	float MaxHealth;

	UPROPERTY(SaveGame)
	float MaxEnergy;

	UPROPERTY(SaveGame)
	float Energy;

	UPROPERTY(SaveGame)
	float MaxShield;

	UPROPERTY(SaveGame)
	float Shield;
	
	float DrainTickValue = 2.f;
	float RegenerateTickValue = .5f;

	UPROPERTY()
	UStaticMesh* ShieldMesh;

	FTimerHandle EnergyDecreaseTimerHandle;
	FTimerHandle EnergyRegenerateTimerHandle;

	UPROPERTY()
	bool bIsDraining;

	void DrainTick();
	void RegenerateEnergy();
};