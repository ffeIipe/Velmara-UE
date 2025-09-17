// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Entities/Entity.h"
#include "PlayerMain.generated.h"

class UInputAction;
struct FInputActionValue;
class AEnemy;
class AItem;
class UAttributeComponent;
class UMementoComponent;
class UCombatComponent;
class UInventoryComponent;
class UCharacterStateComponent;
class UChangeModeComponent;
class UCameraComponent;
class USpringArmComponent;
class ACameraActor;
class UDamageType;
class USoundBase;

UCLASS()
class TESISUE_API APlayerMain : public AEntity
{
	GENERATED_BODY()

public:
	APlayerMain();
	
	virtual void GetHit(TScriptInterface<ICombatTargetInterface> DamageCauser, const FVector& ImpactPoint, FDamageEvent const& DamageEvent, const float DamageReceived) override;
	
	virtual float TakeDamage(
		float DamageAmount,
		FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser) override;
	
	virtual bool IsPossessed() override { return true; } //true by default, because it wouldn't be marked as a threat if not
	
	// --- Components ---
	UPROPERTY(BlueprintReadOnly, Category = "Components | Modes")
	UChangeModeComponent* ChangeModeComponent;

	UPROPERTY(EditAnywhere, Category = "Gameplay | SpectralMode | SpectralAttack | Targeting")
	AEnemy* SpectralTarget;

	UFUNCTION(BlueprintPure, Category = "Gameplay | SpectralMode | SpectralAttack")
	FORCEINLINE AEnemy* GetSpectralTarget() const { return SpectralTarget; }

	// --- State & Interaction ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State | Damage")
	bool bCanReceiveDamage = true;

	// --- Camera ---
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void ResetFollowCamera();

	// --- Forms ---
	UFUNCTION(BlueprintCallable, Category = "Gameplay | Forms")
	void ToggleForm();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void PerformDead();

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	// --- Life Cycle ---
	void Die(UAnimMontage* DeathAnim, FName Section) override;
	void Revive();
	void LoadLastCheckpoint() const;

	UFUNCTION()
	void ApplyHumanMode();

	UFUNCTION()
	void ApplySpectralMode();
};