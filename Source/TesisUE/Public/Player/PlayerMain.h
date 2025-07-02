#pragma once

#include "CoreMinimal.h"
#include "Entities/Entity.h"
#include "CharacterStates.h"
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

	//~PAGINATION____________________________________________________________//
	//~ Interface Implementations
	//~______________________________________________________________________//

	virtual void PerformSpectralAttack_Implementation() override;
	virtual void PerformSpectralBarrier_Implementation() override;
	virtual void ResetSpectralAttack_Implementation() override;

	void GetHit_Implementation(
		AActor* DamageCauser,
		const FVector& ImpactPoint,
		TSubclassOf<UDamageType> DamageType,
		const float DamageReceived) override;

	//~PAGINATION____________________________________________________________//
	//~ Core Overrides
	//~______________________________________________________________________//

	/*virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;*/

	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser) override;

	//~PAGINATION____________________________________________________________//
	//~ Components
	//~______________________________________________________________________//

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components | Forms")
	UPlayerFormComponent* PlayerFormComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components | Spectral Weapon")
	USpectralWeaponComponent* SpectralWeaponComponent;

	//~PAGINATION____________________________________________________________//
	//~ Spectral Mode & Attack
	//~______________________________________________________________________//

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

	//~PAGINATION____________________________________________________________//
	//~ State & Flags
	//~______________________________________________________________________//
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State | Damage")
	bool bCanReceiveDamage = true;


	//~PAGINATION____________________________________________________________//
	//~ Getters & Setters
	//~______________________________________________________________________//

	UFUNCTION(BlueprintPure, Category = "Gameplay | SpectralMode | SpectralAttack")
	FORCEINLINE AEnemy* GetSpectralTarget() const { return SpectralTarget; }

	FORCEINLINE void SetOverlappingItem(AItem* Item) { OverlappingItem = Item; }

	//~PAGINATION____________________________________________________________//
	//~ Gameplay Actions (Callable)
	//~______________________________________________________________________//

	/*UFUNCTION(BlueprintCallable, Category = "Gameplay | Possess")
	void ReleasePossession(AEnemy* EnemyPossessed);*/

	UFUNCTION(BlueprintCallable, Category = "Camera")
	void ResetFollowCamera();

	UFUNCTION(BlueprintCallable, Category = "Gameplay | Forms")
	void ToggleForm();

	UFUNCTION(BlueprintCallable, Category = "Gameplay | SpectralMode | SpectralAttack")
	void SearchTarget();

	//~PAGINATION____________________________________________________________//
	//~ Event Handlers (Bound via Delegates, etc.)
	//~______________________________________________________________________//

protected:
	virtual void BeginPlay() override;
	/*virtual void Landed(const FHitResult& Hit) override;
	virtual void Jump() override;*/

private:

	//~PAGINATION____________________________________________________________//
	//~ Configuration | Stats & Cooldowns
	//~______________________________________________________________________//

	UPROPERTY(EditDefaultsOnly, Category = "Stats | Forms | Cooldown")
	float TransformationCooldown;

	UPROPERTY(EditAnywhere, Category = "State | Forms | Cooldown")
	float LastTransformationTime;

	float DefaultMaxWalkSpeed = 700.f;

	//~PAGINATION____________________________________________________________//
	//~ Internal References & State
	//~______________________________________________________________________//
	UPROPERTY(VisibleInstanceOnly, Category = "Runtime | Interact")
	AItem* OverlappingItem;

	//~PAGINATION____________________________________________________________//
	//~ Internal Gameplay Logic & Input Handlers
	//~______________________________________________________________________//
	/*void Interact(const FInputActionValue& Value);*/

public:
	void Equipping(bool bIsSwordBeingEquipped);
	/*bool IsEquipping();*/

private:
	/*UFUNCTION(BlueprintCallable, Category = "Gameplay | Possess") 
	AEnemy* GetTargetEnemy();*/

	/*UFUNCTION(BlueprintCallable, Category = "Gameplay | Possess")
	void PossessEnemy();*/

	void Die();
	void Revive();
	void LoadLastCheckpoint();

	void ChangePrimaryWeapon();
	void ChangeSecondaryWeapon();

	UFUNCTION()
	void WithEnergy();

	UFUNCTION()
	void OutOfEnergy();
};