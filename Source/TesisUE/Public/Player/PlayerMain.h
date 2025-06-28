#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
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
class TESISUE_API APlayerMain : public ACharacter, public IHitInterface, public ICharacterState, public IFormInterface, public IMementoEntity
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
	virtual void GetHit_Implementation(AActor* DamageCauser, const FVector& ImpactPoint, TSubclassOf<UDamageType> DamageType, const float DamageReceived) override;
	virtual UCharacterStateComponent* GetCharacterStateComponent_Implementation() override;
	virtual UMementoComponent* GetMementoComponent_Implementation() override;

	//~PAGINATION____________________________________________________________//
	//~ Core Overrides
	//~______________________________________________________________________//

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser) override;

	//~PAGINATION____________________________________________________________//
	//~ Components
	//~______________________________________________________________________//

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	UCombatComponent* CombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components | Forms")
	UPlayerFormComponent* PlayerFormComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components | Attributes")
	UAttributeComponent* Attributes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components | Memento")
	UMementoComponent* MementoComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components | Inventory")
	UInventoryComponent* InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components | State")
	UCharacterStateComponent* CharacterStateComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components | Spectral Weapon")
	USpectralWeaponComponent* SpectralWeaponComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components | Camera")
	USpringArmComponent* CameraBoom;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components | ExtraMovementComponent")
	class UExtraMovementComponent* ExtraMovementComponent;

	//~PAGINATION____________________________________________________________//
	//~ Input Actions
	//~______________________________________________________________________//

	UPROPERTY(EditAnywhere, Category = "Input | Mapping")
	UInputMappingContext* CharacterContext;

	UPROPERTY(EditAnywhere, Category = "Input | Movement")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, Category = "Input | Movement")
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, Category = "Input | Movement")
	UInputAction* CrouchAction;

	UPROPERTY(EditAnywhere, Category = "Input | Actions")
	UInputAction* InteractAction;

	UPROPERTY(EditAnywhere, Category = "Input | Actions")
	UInputAction* ChangeFormAction;

	UPROPERTY(EditAnywhere, Category = "Input | Actions")
	UInputAction* PossessAction;

	UPROPERTY(EditAnywhere, Category = "Input | Actions")
	UInputAction* InventoryAction;

	UPROPERTY(EditAnywhere, Category = "Input | System")
	UInputAction* RestartAction;

	UPROPERTY(EditAnywhere, Category = "Input | System")
	UInputAction* GoToMenuAction;


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
	//~ Interaction
	//~______________________________________________________________________//
	UPROPERTY(EditAnywhere, Category = "Gameplay | Interact | Targeting")
	float InteractTargetRadius;

	//~PAGINATION____________________________________________________________//
	//~ Camera
	//~______________________________________________________________________//
	UPROPERTY(EditAnywhere, Category = "Camera")
	ACameraActor* FollowCamera;

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

	UFUNCTION(BlueprintPure, Category = "Components | Attributes")
	FORCEINLINE UAttributeComponent* GetAttributes() { return Attributes; };

	//~PAGINATION____________________________________________________________//
	//~ Gameplay Actions (Callable)
	//~______________________________________________________________________//

	UFUNCTION(BlueprintCallable, Category = "Combat | Weapon")
	void SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled);

	UFUNCTION(BlueprintCallable, Category = "Gameplay | Possess")
	void ReleasePossession(AEnemy* EnemyPossessed);

	UFUNCTION(BlueprintCallable, Category = "Camera")
	void ResetFollowCamera();

	UFUNCTION(BlueprintCallable, Category = "Gameplay | Forms")
	void ToggleForm();

	UFUNCTION(BlueprintCallable, Category = "Gameplay | SpectralMode | SpectralAttack")
	void SearchTarget();

	//~PAGINATION____________________________________________________________//
	//~ Event Handlers (Bound via Delegates, etc.)
	//~______________________________________________________________________//
	UFUNCTION()
	void OnWallCollision(const FHitResult& HitResult);

protected:
	//~PAGINATION____________________________________________________________//
	//~ Core Overrides
	//~______________________________________________________________________//
	virtual void BeginPlay() override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void Jump() override;

	//~PAGINATION____________________________________________________________//
	//~ Interaction
	//~______________________________________________________________________//
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay | Interact | Trace")
	float InteractTraceLenght;

	//~PAGINATION____________________________________________________________//
	//~ Animation Montages
	//~______________________________________________________________________//
	UPROPERTY(EditAnywhere, Category = "Animation | Montages | Death")
	UAnimMontage* DeathMontage;
public:
	UPROPERTY(EditDefaultsOnly, Category = "Inventory | Montages | Equip")
	UAnimMontage* EquipSwordMontage;

private:	
	UPROPERTY(EditDefaultsOnly, Category = "Inventory | Montages | Equip")
	UAnimMontage* EquipPistolMontage;

private:
	//~PAGINATION____________________________________________________________//
	//~ Configuration | SFX
	//~______________________________________________________________________//
	UPROPERTY(EditDefaultsOnly, Category = "Audio | SFX")
	USoundBase* ErrorSFX;
	
	UPROPERTY(EditDefaultsOnly, Category = "Audio | SFX")
	USoundBase* ReceiveDamageSFX;

	//~PAGINATION____________________________________________________________//
	//~ Configuration | Stats & Cooldowns
	//~______________________________________________________________________//

	UPROPERTY(EditDefaultsOnly, Category = "Stats | Forms | Cooldown")
		float TransformationCooldown;

	UPROPERTY(EditAnywhere, Category = "State | Forms | Cooldown")
		float LastTransformationTime;

	/* Possess Stats */
	UPROPERTY(EditAnywhere, Category = "Stats | Possess")
	float PossessDistance;

	float DefaultMaxWalkSpeed = 700.f;

	UPROPERTY(EditAnywhere, Category = "Stats | Stun")
	float StunMaxWalkSpeed;

	//~PAGINATION____________________________________________________________//
	//~ Internal References & State
	//~______________________________________________________________________//
	
	UPROPERTY(Transient)
	class APlayerController* PlayerControllerRef = nullptr;

	UPROPERTY(VisibleInstanceOnly, Category = "State | Lifecycle")
	bool bIsDead = false;

	UPROPERTY(VisibleInstanceOnly, Category = "State | Possess")
	bool bIsPossessing = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Runtime | Interact")
	AItem* OverlappingItem;

	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Runtime | Possess")
	AEnemy* PossessedEnemy;

	//~PAGINATION____________________________________________________________//
	//~ Internal Gameplay Logic & Input Handlers
	//~______________________________________________________________________//

	void Move(const FInputActionValue& Value);
	void Dodge(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Interact(const FInputActionValue& Value);

public:
	void Equipping(bool bIsSwordBeingEquipped);
	bool IsEquipping();

private:
	void Attack(const FInputActionValue& Value);
	void HeavyAttack(const FInputActionValue& Value);
	void LaunchAttack(const FInputActionValue& Value);
	void Block(const FInputActionValue& Value);
	void ReleaseBlock(const FInputActionValue& Value);
	void Execute(const FInputActionValue& Value);
	void StunBehavior();

public:
	void RemoveStunBehavior();

private:
	UFUNCTION(BlueprintCallable, Category = "Gameplay | Effects")
	void HitStop(float Duration, float TimeScale);

	UFUNCTION(BlueprintCallable, Category = "Gameplay | Effects")
	void ResetTimeDilation();

	UFUNCTION(BlueprintCallable, Category = "Gameplay | Possess") 
	AEnemy* GetTargetEnemy();

	UFUNCTION(BlueprintCallable, Category = "Gameplay | Possess")
	void PossessEnemy();

	void Die();
	void Revive();
	void LoadLastCheckpoint();

	void ChangePrimaryWeapon();
	void ChangeSecondaryWeapon();

	UFUNCTION()
	void WithEnergy();

	UFUNCTION()
	void OutOfEnergy();

	UFUNCTION()
	void RestartLevel();
};