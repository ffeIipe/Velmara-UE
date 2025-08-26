// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Entities/Entity.h"
#include "CharacterStates.h" // Assuming this is a custom enum or struct
#include "Interfaces/FormInterface.h"
#include "Interfaces/HitInterface.h"
#include "Interfaces/CharacterState.h"
#include "Interfaces/MementoEntity.h"
#include "PlayerMain.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class AEnemy;
class AItem;
class UAttributeComponent;
class UMementoComponent;
class UCombatComponent;
class UInventoryComponent;
class UCharacterStateComponent;
class UPlayerFormComponent;
class UCameraComponent;
class USpringArmComponent;
class USpectralWeaponComponent;
class ACameraActor;
class UDamageType;
class USoundBase;

UCLASS()
class TESISUE_API APlayerMain : public AEntity, public IFormInterface
{
	GENERATED_BODY()

public:
	APlayerMain();

	// --- Interfaces ---
	virtual void PerformSpectralAttack_Implementation() override;
	virtual void PerformSpectralBarrier_Implementation() override;
	virtual void ResetSpectralAttack_Implementation() override;

	virtual void GetHit_Implementation(AEntity* DamageCauser,
		const FVector& ImpactPoint, FDamageEvent const& DamageEvent,
		const float DamageReceived) override;

	virtual float TakeDamage(
		float DamageAmount,
		FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser) override;

	// --- Components ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components | Forms")
	UPlayerFormComponent* PlayerFormComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components | Spectral Weapon")
	USpectralWeaponComponent* SpectralWeaponComponent;

	// --- Spectral Mode - Attack ---
	UPROPERTY(BlueprintReadWrite, Category = "Gameplay | SpectralMode | SpectralAttack")
	int SpectralAttackIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Animation | Montages | SpectralMode | SpectralAttack")
	TArray<UAnimMontage*> SpectralAttackCombo;

	UPROPERTY(BlueprintReadOnly, Category = "Animation | Montages | SpectralMode | SpectralAttack")
	UAnimMontage* SpectralHeavyAttack;

	UPROPERTY(EditAnywhere, Category = "Gameplay | SpectralMode | SpectralAttack | Targeting")
	AEnemy* SpectralTarget;

	UPROPERTY(EditAnywhere, Category = "Gameplay | SpectralMode | SpectralAttack | Targeting")
	float TrackTargetDistance;

	UPROPERTY(EditAnywhere, Category = "Gameplay | SpectralMode | SpectralAttack | Targeting")
	float TrackTargetRadius;

	UFUNCTION(BlueprintPure, Category = "Gameplay | SpectralMode | SpectralAttack")
	FORCEINLINE AEnemy* GetSpectralTarget() const { return SpectralTarget; }

	UFUNCTION(BlueprintCallable, Category = "Gameplay | SpectralMode | SpectralAttack")
	void SearchTarget();

	// --- State & Interaction ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State | Damage")
	bool bCanReceiveDamage = true;

	FORCEINLINE void SetOverlappingItem(AItem* Item) { OverlappingItem = Item; }

	void Equipping(bool bIsSwordBeingEquipped);

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

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	// --- Movement & Default Parameters ---
	float DefaultMaxWalkSpeed = 700.f;

	// --- Interaction ---
	UPROPERTY(VisibleInstanceOnly, Category = "Runtime | Interact")
	AItem* OverlappingItem;

	// --- Life Cycle ---
	void Die(UAnimMontage* DeathAnim, FName Section) override;
	void Revive();
	void LoadLastCheckpoint() const;
	void OutOfEnergy() override;

	// --- Weapon Management ---
	void ChangePrimaryWeapon();
	void ChangeSecondaryWeapon();

	UFUNCTION()
	void ApplyHumanMode();

	UFUNCTION()
	void ApplySpectralEffect();
};