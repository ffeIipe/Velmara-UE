#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

//Interfaces
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
	/*
	* ----------Getters----------
	*/
	UFUNCTION(BlueprintPure, Category = "Components | Combat")
	FORCEINLINE UCombatComponent* GetCombatComponent() const { return CombatComponent; };

	UFUNCTION(BlueprintPure, Category = "Components | Attribute")
	FORCEINLINE UAttributeComponent* GetAttributeComponent() const { return AttributeComponent; };

	UFUNCTION(BlueprintPure, Category = "Components | Character State")
	FORCEINLINE UCharacterStateComponent* GetCharacterStateComponent() const { return CharacterStateComponent; };

	UFUNCTION(BlueprintPure, Category = "Components | Extra Movement")
	FORCEINLINE UExtraMovementComponent* GetExtraMovementComponent() const { return ExtraMovementComponent; };

	UFUNCTION(BlueprintPure, Category = "Components | Inventory")
	FORCEINLINE UInventoryComponent* GetInventoryComponent() const { return InventoryComponent; };

	UFUNCTION(BlueprintPure, Category = "Components | Memento")
	FORCEINLINE UMementoComponent* GetMementoComponent() const { return MementoComponent; };
	
	UFUNCTION(BlueprintPure, Category = "Components | Possession")
	FORCEINLINE UPossessionComponent* GetPossessionComponent() const { return PossessionComponent; };

	UFUNCTION(BlueprintPure, Category = "Components | Camera")
	FORCEINLINE USpringArmComponent* GetSpringArmComponent() const { return SpringArmComponent; };

	UFUNCTION(BlueprintCallable)
	FORCEINLINE ACharacter* GetCharacter() { return this; };

	UFUNCTION(BlueprintCallable)
	AActor* GetLastDamageCauser() { return LastDamageCauser; };

	/*
	* ----------Interface Implementations----------
	*/
	UPROPERTY(BlueprintAssignable)
	FOnEntityCanBeFinished OnCanBeFinished;
	
	UPROPERTY(BlueprintAssignable)
	FOnEntityShieldTakeDamage OnShieldTakeDamage;

	/*
	* ----------Interface Implementations----------
	*/
	virtual void GetHit_Implementation(
		AActor* DamageCauser,
		const FVector& ImpactPoint,
		TSubclassOf<UDamageType> DamageType,
		const float DamageReceived) override;

	void PlayCameraShake(const FVector& Epicenter, float InnerRadius, float OuterRadius);

	virtual bool CanBeFinished_Implementation() override;

	/*
	* ----------Gameplay Actions----------
	*/
	UFUNCTION(BlueprintCallable, Category = "Combat | Weapon")
	void SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled);

	void AttachFollowCamera(USpringArmComponent* AttachTarget);

	virtual void StunBehavior();

	virtual void RemoveStunBehavior();

	UFUNCTION()
	void OnWallCollision(const FHitResult& HitResult);

protected:
	virtual void BeginPlay() override;

	/*
	* ----------Input Mapping Context----------
	*/
	UPROPERTY(EditAnywhere, Category = "Input | Mapping")
	UInputMappingContext* CharacterContext;

	/*
	* ----------Camera----------
	*/	
	UPROPERTY(EditAnywhere, Category = "Camera")
	ACameraActor* FollowCamera;

	/*
	* ----------Internal Gameplay Logic----------
	*/
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void Move(const FInputActionValue& Value);

	virtual void Landed(const FHitResult& Hit) override;

	virtual void Jump() override;

	virtual void Dodge(const FInputActionValue& Value);

	virtual void Look(const FInputActionValue& Value);

	virtual void Interact(const FInputActionValue& Value);

	virtual void Attack();

	virtual void HeavyAttack();

	virtual void LaunchAttack();

	virtual void Block();

	virtual void ReleaseBlock();

	virtual void Execute();

	virtual void Die();

	virtual void Equipping(bool bIsSwordBeingEquipped) {};

	bool IsEquipping();

	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser) override;

	/*
	* ----------Montages----------
	*/
	UPROPERTY(EditAnywhere, Category = "Animation | Montages | Death")
	UAnimMontage* DeathMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory | Montages | Equip")
	UAnimMontage* EquipSwordMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory | Montages | Equip")
	UAnimMontage* EquipPistolMontage;

	/*
	* ----------Inherited Data----------
	*/
	AActor* LastDamageCauser;

	UPROPERTY(Transient)
	class APlayerController* PlayerControllerRef = nullptr;

private:
	/*
	* ----------SFX & VFX----------
	*/
	UPROPERTY(EditDefaultsOnly, Category = "Effects | SFX")
	USoundBase* ErrorSFX;

	UPROPERTY(EditDefaultsOnly, Category = "Effects | SFX")
	USoundBase* ReceiveDamageSFX;

	UPROPERTY(EditAnywhere, Category = "Effects | SFX");
	USoundBase* HitSound;

	UPROPERTY(EditAnywhere, Category = "Effects | SFX")
	USoundBase* ShieldImpactSFX;

	UPROPERTY(EditAnywhere, Category = "Effects | VFX");
	UNiagaraSystem* NiagaraSystem;

	UPROPERTY(EditAnywhere, Category = "Effects | CameraShake")
	TSubclassOf<UCameraShakeBase> CameraShake;

	/*
	* ----------Stats----------
	*/
	UPROPERTY(EditAnywhere, Category = "Gameplay | Interact")
	float InteractTraceLenght;

	UPROPERTY(EditAnywhere, Category = "Gameplay | Interact")
	float InteractTargetRadius;

	float DefaultMaxWalkSpeed = 700.f;

	UPROPERTY(EditAnywhere, Category = "Stats | Stun")
	float StunMaxWalkSpeed;

	TArray<AActor*> IgnoreActors;

	/*
	* ----------Components----------
	*/
	UPROPERTY(EditAnywhere, BlueprintGetter = GetCombatComponent, Category = "Components | Combat")
	 UCombatComponent* CombatComponent;

	 UPROPERTY(EditAnywhere, BlueprintGetter = GetAttributeComponent, Category = "Components | Attribute")
	 UAttributeComponent* AttributeComponent;

	 UPROPERTY(EditAnywhere, BlueprintGetter = GetCharacterStateComponent, Category = "Components | Character State")
	 UCharacterStateComponent* CharacterStateComponent;

	 UPROPERTY(EditAnywhere, BlueprintGetter = GetExtraMovementComponent, Category = "Components | Extra Movement")
	 UExtraMovementComponent* ExtraMovementComponent;

	 UPROPERTY(EditAnywhere, BlueprintGetter = GetInventoryComponent, Category = "Components | Inventory")
	 UInventoryComponent* InventoryComponent;

	 UPROPERTY(EditAnywhere, BlueprintGetter = GetMementoComponent, Category = "Components | Memento")
	 UMementoComponent* MementoComponent;

	 UPROPERTY(EditAnywhere, BlueprintGetter = GetPossessionComponent, Category = "Components | Possession")
	 UPossessionComponent* PossessionComponent;

	 UPROPERTY(EditAnywhere, BlueprintGetter = GetSpringArmComponent, Category = "Components | Camera")
	 USpringArmComponent* SpringArmComponent;

	/*
	* ----------Inputs----------
	*/
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
};
