#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

// Interfaces
#include "Interfaces/HitInterface.h"

#include "Entity.generated.h"

/*
 * ----------Forward Declarations----------
 */
class UCombatComponent;
class UAttributeComponent;
class UCharacterStateComponent;
class UExtraMovementComponent;
class UInventoryComponent;
class UMementoComponent;
class UPossessionComponent;
class USpringArmComponent;

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

class UNiagaraSystem;
class ACameraActor;
class USoundBase;
class UCameraShakeBase;
class UEntityData;

// --- Delegates ---
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEntityDead);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEntityDamaged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEntityCanBeFinished);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEntityShieldTakeDamage);

UCLASS()
class TESISUE_API AEntity : public ACharacter, public IHitInterface
{
	GENERATED_BODY()

public:
	AEntity();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data")
	TObjectPtr<UEntityData> EntityData;

	// --- Getters ---
	UFUNCTION(BlueprintPure, Category = "Components | Combat")
	FORCEINLINE UCombatComponent* GetCombatComponent() const { return CombatComponent; }

	UFUNCTION(BlueprintPure, Category = "Components | Attribute")
	FORCEINLINE UAttributeComponent* GetAttributeComponent() const { return AttributeComponent; }

	UFUNCTION(BlueprintPure, Category = "Components | Character State")
	FORCEINLINE UCharacterStateComponent* GetCharacterStateComponent() const { return CharacterStateComponent; }

	UFUNCTION(BlueprintPure, Category = "Components | Extra Movement")
	FORCEINLINE UExtraMovementComponent* GetExtraMovementComponent() const { return ExtraMovementComponent; }

	UFUNCTION(BlueprintPure, Category = "Components | Inventory")
	FORCEINLINE UInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	UFUNCTION(BlueprintPure, Category = "Components | Memento")
	FORCEINLINE UMementoComponent* GetMementoComponent() const { return MementoComponent; }

	UFUNCTION(BlueprintPure, Category = "Components | Possession")
	FORCEINLINE UPossessionComponent* GetPossessionComponent() const { return PossessionComponent; }

	UFUNCTION(BlueprintPure, Category = "Components | Camera")
	FORCEINLINE USpringArmComponent* GetSpringArmComponent() const { return SpringArmComponent; }

	UFUNCTION()
	FORCEINLINE ACameraActor* GetFollowCamera() const {return FollowCamera;}
	
	UFUNCTION(BlueprintCallable)
	FORCEINLINE ACharacter* GetCharacter() { return this; }

	UFUNCTION(BlueprintCallable)
	AActor* GetLastDamageCauser() { return LastDamageCauser; }

	UPROPERTY(BlueprintAssignable)
	FOnEntityCanBeFinished OnCanBeFinished;

	UPROPERTY(BlueprintAssignable)
	FOnEntityShieldTakeDamage OnShieldTakeDamage;

	// --- Interface Implementations ---
	virtual void GetHit_Implementation(AEntity* DamageCauser,
		const FVector& ImpactPoint, FDamageEvent const& DamageEvent,
		const float DamageReceived) override;

	virtual bool CanBeFinished_Implementation() override;

	virtual bool IsLaunchable_Implementation() override;
	
	// --- Gameplay Actions ---
	UFUNCTION(BlueprintCallable, Category = "Combat | Weapon")
	virtual void SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled);

	void AttachFollowCamera(USpringArmComponent* AttachTarget);

	virtual void StunBehavior();
	virtual void RemoveStunBehavior();

	UFUNCTION()
	void OnWallCollision(const FHitResult& HitResult);

	UFUNCTION()
	virtual void OutOfEnergy();

	UFUNCTION(BlueprintCallable)
	virtual void Die(UAnimMontage* DeathAnim, FName Section = NAME_None);

	void PlayCameraShake(const FVector& Epicenter, float InnerRadius, float OuterRadius);

	bool IsEquipping();

protected:
	virtual void BeginPlay() override;
	void InitializeComponentsData() const;
	void OnConstruction(const FTransform &Transform) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void Jump() override;

	// --- Input Handling ---
	virtual void Interact(const FInputActionValue& Value);

	// --- Damage & Equipping ---
	virtual void Equipping(bool bIsSwordBeingEquipped) {};

	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser) override;

	// --- Montages ---
	UPROPERTY(EditAnywhere, Category = "Animation | Montages | Death")
	UAnimMontage* DeathMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory | Montages | Equip")
	UAnimMontage* EquipSwordMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory | Montages | Equip")
	UAnimMontage* EquipPistolMontage;

	// --- Inherited Data ---
	UPROPERTY()
	AEntity* LastDamageCauser;

	UPROPERTY(Transient)
	APlayerController* PlayerControllerRef = nullptr;

	// --- Input Mapping Context ---
	UPROPERTY(EditAnywhere, Category = "Input | Mapping")
	UInputMappingContext* CharacterContext;

	// --- Camera ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	ACameraActor* FollowCamera;

	// --- Input Actions ---
	UPROPERTY(EditAnywhere, Category = "Input | Movement")
	UInputAction* InputAction_Move;

	UPROPERTY(EditAnywhere, Category = "Input | Movement")
	UInputAction* InputAction_Look;

	UPROPERTY(EditAnywhere, Category = "Input | Movement")
	UInputAction* InputAction_Crouch;

	UPROPERTY(EditAnywhere, Category = "Input | Movement")
	UInputAction* InputAction_Jump;

	UPROPERTY(EditAnywhere, Category = "Input | Movement")
	UInputAction* InputAction_Dodge;

	UPROPERTY(EditAnywhere, Category = "Input | Actions")
	UInputAction* InputAction_Interact;

	UPROPERTY(EditAnywhere, Category = "Input | Actions")
	UInputAction* InputAction_SwitchForm;

	UPROPERTY(EditAnywhere, Category = "Input | Actions")
	UInputAction* InputAction_Possess;

	UPROPERTY(EditAnywhere, Category = "Input | Actions")
	UInputAction* InputAction_Inventory;

	UPROPERTY(EditAnywhere, Category = "Input | Actions")
	UInputAction* InputAction_Attack;

	UPROPERTY(EditAnywhere, Category = "Input | Actions")
	UInputAction* InputAction_HeavyAttack;

	UPROPERTY(EditAnywhere, Category = "Input | Actions")
	UInputAction* InputAction_Launch;

	UPROPERTY(EditAnywhere, Category = "Input | Actions")
	UInputAction* InputAction_Block;

	UPROPERTY(EditAnywhere, Category = "Input | Actions")
	UInputAction* InputAction_ChangeHardLockTarget;
	
	UPROPERTY()
	TArray<AActor*> IgnoreActors;
	
private:
	// --- SFX & VFX ---
	UPROPERTY(EditDefaultsOnly, Category = "Effects | SFX")
	USoundBase* ErrorSFX;

	UPROPERTY(EditDefaultsOnly, Category = "Effects | SFX")
	USoundBase* ReceiveDamageSFX;

	UPROPERTY(EditAnywhere, Category = "Effects | SFX")
	USoundBase* HitSound;

	UPROPERTY(EditAnywhere, Category = "Effects | SFX")
	USoundBase* ShieldImpactSFX;

	UPROPERTY(EditAnywhere, Category = "Effects | VFX")
	UNiagaraSystem* ReceiveDamageFX;

	UPROPERTY(EditAnywhere, Category = "Effects | CameraShake")
	TSubclassOf<UCameraShakeBase> CameraShake;

	// --- Stats & Properties ---
	UPROPERTY(EditAnywhere, Category = "Gameplay | Interact")
	float InteractTraceLenght;

	UPROPERTY(EditAnywhere, Category = "Gameplay | Interact")
	float InteractTargetRadius;

	float DefaultMaxWalkSpeed = 700.f;

	UPROPERTY(EditAnywhere, Category = "Stats | Stun")
	float StunMaxWalkSpeed;

	// --- Components ---
	UPROPERTY(VisibleAnywhere, BlueprintGetter = GetCombatComponent, Category = "Components | Combat")
	UCombatComponent* CombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintGetter = GetAttributeComponent, Category = "Components | Attribute")
	UAttributeComponent* AttributeComponent;

	UPROPERTY(VisibleAnywhere, BlueprintGetter = GetCharacterStateComponent, Category = "Components | Character State")
	UCharacterStateComponent* CharacterStateComponent;

	UPROPERTY(VisibleAnywhere, BlueprintGetter = GetExtraMovementComponent, Category = "Components | Extra Movement")
	UExtraMovementComponent* ExtraMovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintGetter = GetInventoryComponent, Category = "Components | Inventory")
	UInventoryComponent* InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintGetter = GetMementoComponent, Category = "Components | Memento")
	UMementoComponent* MementoComponent;

	UPROPERTY(VisibleAnywhere, BlueprintGetter = GetPossessionComponent, Category = "Components | Possession")
	UPossessionComponent* PossessionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintGetter = GetSpringArmComponent, Category = "Components | Camera")
	USpringArmComponent* SpringArmComponent;
};