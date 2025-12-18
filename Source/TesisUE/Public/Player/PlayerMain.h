// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DataAssets/CombatStrategyDataAsset.h"
#include "Entities/Entity.h"
#include "PlayerMain.generated.h"


class UChangeModeComponent;
class AEnemy;

UCLASS()
class TESISUE_API APlayerMain : public AEntity
{
	GENERATED_BODY()

public:
	APlayerMain();
	
	virtual float TakeDamage(
		float DamageAmount,
		FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser) override;
	
	virtual bool IsPossessed() override { return true; } //true by default, because it wouldn't be marked as a threat if not
	
	// --- Components ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components | Modes")
	UChangeModeComponent* ChangeModeComponent;

	UPROPERTY(EditAnywhere, Category = "Gameplay | SpectralMode | SpectralAttack | Targeting")
	AEnemy* SpectralTarget;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input | Config")
	TObjectPtr<UInputMappingContext> DefaultInputContext;

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
	virtual void Die(UAnimMontage* DeathAnim, FName Section) override;
	
	void Revive();
	
	void LoadLastCheckpoint() const;
	
	void ApplyModeConfig(const FCharacterModeConfig& Config);

	UFUNCTION()
	void ApplyHumanMode();

	UFUNCTION()
	void ApplySpectralMode();
};