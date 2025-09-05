#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

// Interfaces
#include "Camera/CameraComponent.h"
#include "Components/AttributeComponent.h"
#include "Components/CharacterStateComponent.h"
#include "Components/CombatComponent.h"
#include "Components/ExtraMovementComponent.h"
#include "Components/InventoryComponent.h"
#include "Components/PossessionComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interfaces/AnimatorProvider.h"
#include "Interfaces/AttributeProvider.h"
#include "Interfaces/HitInterface.h"
#include "Interfaces/CameraProvider.h"
#include "Interfaces/CharacterStateProvider.h"
#include "Interfaces/CharacterMovementProvider.h"
#include "Interfaces/OwnerUtilsInterface.h"
#include "Interfaces/CombatTargetInterface.h"
#include "Interfaces/ControllerProvider.h"
#include "Interfaces/Weapon/WeaponProvider.h"

#include "Entity.generated.h"

class UCameraComponent;
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
class TESISUE_API AEntity : public ACharacter,
							public IHitInterface,
							public ICameraProvider,
							public IWeaponProvider,
                            public ICharacterStateProvider,
							public ICharacterMovementProvider,
							public IOwnerUtilsInterface,
							public ICombatTargetInterface,
							public IControllerProvider,
							public IAttributeProvider,
							public IAnimatorProvider
{
	GENERATED_BODY()

public:
	AEntity();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data")
	TObjectPtr<UEntityData> EntityData;

	UPROPERTY()
	UAnimMontage* HitReactMontage;

	// --- Getters ---
	UFUNCTION(BlueprintPure, Category = "Components | Combat")
	FORCEINLINE UCombatComponent* GetCombatComponent() const { return CombatComponent; };

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

	UFUNCTION(BlueprintPure, Category = "Components | Camera")
	FORCEINLINE UCameraComponent* GetCameraComponent() const { return CameraComponent; }
	
	UPROPERTY(BlueprintAssignable)
	FOnEntityCanBeFinished OnCanBeFinished;

	UPROPERTY(BlueprintAssignable)
	FOnEntityShieldTakeDamage OnShieldTakeDamage;

	// --- Interface Implementations ---
	virtual void GetHit(::TScriptInterface<ICombatTargetInterface> DamageCauser, const FVector& ImpactPoint,
	                    FDamageEvent const& DamageEvent, const float DamageReceived) override;
	virtual void GetFinished() override;
	virtual bool IsHittable() override;

	virtual UCameraComponent* GetEntityCamera() override { return GetCameraComponent(); }
	virtual FVector GetCameraLocation() override { return GetCameraComponent()->GetComponentLocation(); }
	virtual void AttachFollowCamera() override;

	UFUNCTION(BlueprintPure)
	virtual TScriptInterface<IWeaponInterface> GetWeaponEquipped() override { return GetInventoryComponent()->GetWeaponEquipped(); }

	virtual const FCharacterStates& GetCurrentCharacterState() override { return GetCharacterStateComponent()->CurrentStates; }
	virtual ECharacterWeaponStates SetWeaponState(ECharacterWeaponStates NewState) override;
	virtual ECharacterActionsStates SetAction(ECharacterActionsStates NewAction) override;
	virtual ECharacterModeStates SetMode(ECharacterModeStates NewForm) override;
	virtual bool IsWeaponStateEqualToAny(const TArray<ECharacterWeaponStates>& StatesToCheck) const override;
	virtual bool IsActionStateEqualToAny(const TArray<ECharacterActionsStates>& ActionsToCheck) const override;
	virtual bool IsModeStateEqualToAny(const TArray<ECharacterModeStates>& FormsToCheck) const override;

	virtual FVector GetLastMovementInputVector() override { return Super::GetLastMovementInputVector(); } 
	virtual FRotator GetControlRotation() override { return Super::GetControlRotation(); }
	virtual bool IsUsingControllerRotationYaw() override { return bUseControllerRotationYaw; }
	virtual float GetMaxWalkSpeed() override { return GetCharacterMovement()->MaxWalkSpeed; }
	virtual void AddMovementInput(const FVector& Vector, double X) override { Super::AddMovementInput(Vector, X); }
	virtual void AddControllerPitchInput(const double X) override { Super::AddControllerPitchInput(X); }
	virtual void AddControllerYawInput(const double X) override { Super::AddControllerYawInput(X); }
	virtual void LaunchCharacter(const FVector& Vector, const bool bCond, const bool bCond1) override { Super::LaunchCharacter(Vector, bCond, bCond1); }
	virtual FVector GetVelocity() override { return Super::GetVelocity(); }
	virtual FVector GetCurrentAcceleration() override { return GetCharacterMovement()->GetCurrentAcceleration(); }
	
	UFUNCTION(BlueprintPure)
	virtual TScriptInterface<ICombatTargetInterface> GetLastDamageCauser() override { return LastDamageCauser; }
	virtual bool IsFalling() override { return GetCharacterMovement()->IsFalling(); }
	virtual bool IsFlying() override { return GetCharacterMovement()->IsFlying(); }
	virtual bool IsMovingBackwards() override { return GetExtraMovementComponent()->IsMovingBackwards(); }
	virtual bool IsEquipped() override;
	virtual bool IsLocking() override { return GetCombatComponent()->bIsHardLocking; }
	virtual void SetMovementMode(const EMovementMode MovementMode) override { GetCharacterMovement()->SetMovementMode(MovementMode); }

	virtual FVector GetTargetActorLocation() override { return GetActorLocation(); }
	virtual bool IsAlive() override { return GetAttributeComponent()->IsAlive(); }
	virtual bool IsPossessed() override { return GetPossessionComponent()->IsPossessed(); }
	virtual bool IsPossessing() override { return GetPossessionComponent()->IsPossessing(); }
	virtual bool IsBlocking() override { return GetCombatComponent()->IsBlocking(); }
	virtual bool CanBeFinished() override;
	virtual bool IsLaunchable() override;
	virtual void GetDirectionalReact(const FVector& ImpactPoint) override; 
	virtual void LaunchUp() override { GetCombatComponent()->StartLaunchingUp(); }

	virtual AController* GetEntityController() override { return GetController(); }

	virtual void IncreaseEnergy(const float Percentage) override { GetAttributeComponent()->IncreaseEnergy(Percentage); }
	virtual bool RequiresEnergy(const float X) override { return GetAttributeComponent()->RequiresEnergy(X); }
	virtual void SetEnergy(const float EnergyFromPossessor) override { GetAttributeComponent()->SetEnergy(EnergyFromPossessor); }
	virtual void IncreaseHealth(const float X) override { GetAttributeComponent()->IncreaseHealth(X); }

	virtual float PlayAnimMontage(UAnimMontage* Montage, const float Rate = 1.f, const FName Section = "Default") override { return Super::PlayAnimMontage(Montage, Rate, Section); }
	virtual void StopAnimMontage(UAnimMontage* MontageToStop = nullptr) override { Super::StopAnimMontage(MontageToStop); }
    virtual USceneComponent* GetMeshComponent() override { return GetMesh(); }
    virtual void PauseAnims(const bool bEnable) override { GetMesh()->bPauseAnims = bEnable; }
	virtual void ChangeWeaponAnimationState() override; 
	
	void HitReactJumpToSection(FName Section);
	
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

	bool IsEquipping() const;

protected:
	virtual void BeginPlay() override;
	void InitializeComponentsData() const;
	virtual void OnConstruction(const FTransform &Transform) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void Jump() override;
	
	// --- Damage & Equipping ---
	virtual float TakeDamage(
		float DamageAmount,
		FDamageEvent const& DamageEvent,
		AController* EventInstigator,
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
	TScriptInterface<ICombatTargetInterface> LastDamageCauser;

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

	UPROPERTY(EditAnywhere, Category = "Input | Actions")
	const UInputAction* InputAction_ToggleWeapon;
	
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

	UPROPERTY(VisibleAnywhere, BlueprintGetter = GetCameraComponent, Category = "Components | Camera")
	UCameraComponent* CameraComponent;
};