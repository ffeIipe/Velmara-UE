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
#include "Components/TargetingComponent.h"
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
#include "Interfaces/StrategyInterface.h"
#include "Interfaces/Weapon/WeaponProvider.h"

#include "Entity.generated.h"

class UMontagesData;
class UEffectsData;
class UInputData;
class UTargetingComponent;
class UCameraComponent;
/*
 * =========-Forward Declarations=========-
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

// === Delegates ===
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
							public IAnimatorProvider,
							public IStrategyProvider
{
	GENERATED_BODY()

public:
	AEntity();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data")
	TObjectPtr<UEntityData> EntityData;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data")
	TObjectPtr<UInputData> InputsData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data")
	TObjectPtr<UEffectsData> EffectsData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data")
	TObjectPtr<UMontagesData> MontagesData;

	// === Getters ===
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

	UFUNCTION(BlueprintPure, Category = "Components | Possession")
	FORCEINLINE UTargetingComponent* GetTargetingComponent() const { return TargetingComponent; }

	UFUNCTION(BlueprintPure, Category = "Components | Camera")
	FORCEINLINE USpringArmComponent* GetSpringArmComponent() const { return SpringArmComponent; }

	UFUNCTION(BlueprintPure, Category = "Components | Camera")
	FORCEINLINE UCameraComponent* GetCameraComponent() const { return CameraComponent; }
	
	UPROPERTY(BlueprintAssignable)
	FOnEntityCanBeFinished OnCanBeFinished;

	UPROPERTY(BlueprintAssignable)
	FOnEntityShieldTakeDamage OnShieldTakeDamage;

	// === Interface Implementations ===
	virtual void GetHit(::TScriptInterface<ICombatTargetInterface> DamageCauser, const FVector& ImpactPoint,
	                    FDamageEvent const& DamageEvent, const float DamageReceived) override;
	virtual void GetFinished() override;
	virtual bool IsHittable() override;

	virtual UCameraComponent* GetEntityCamera() override { return GetCameraComponent(); }
	virtual FVector GetCameraLocation() override { return GetCameraComponent()->GetComponentLocation(); }

	UFUNCTION(BlueprintPure)
	virtual TScriptInterface<IWeaponInterface> GetCurrentWeapon() override { return GetInventoryComponent()->GetCurrentWeapon(); }

	virtual const FCharacterStates& GetCurrentCharacterState() override { return GetCharacterStateComponent()->CurrentStates; }
	virtual ECharacterWeaponStates SetWeaponState(ECharacterWeaponStates NewState) override;
	virtual ECharacterActionsStates SetAction(ECharacterActionsStates NewAction) override;
	virtual ECharacterModeStates SetMode(ECharacterModeStates NewForm) override;
	virtual bool IsWeaponStateEqualToAny(const TArray<ECharacterWeaponStates>& StatesToCheck) override;
	virtual bool IsActionStateEqualToAny(const TArray<ECharacterActionsStates>& ActionsToCheck) override;
	virtual bool IsModeStateEqualToAny(const TArray<ECharacterModeStates>& FormsToCheck) override;

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
	virtual bool IsMovingBackwards() override { return ExtraMovementComponent->IsMovingBackwards(); }
	virtual bool IsEquipped() override;
	virtual bool IsLocking() override { return GetTargetingComponent()->IsLocking(); }
	virtual void SetMovementMode(const EMovementMode MovementMode) override { GetCharacterMovement()->SetMovementMode(MovementMode); }

	virtual FVector GetTargetActorLocation() override { return GetActorLocation(); }
	UFUNCTION(BlueprintCallable)
	virtual bool IsAlive() override { return AttributeComponent->IsAlive(); }
	virtual bool IsPossessed() override { return PossessionComponent->IsPossessed(); }
	virtual bool IsPossessing() override { return PossessionComponent->IsPossessing(); }
	virtual bool IsBlocking() override { return CombatComponent->IsBlocking(); }
	virtual bool CanBeFinished() override;
	virtual bool IsLaunchable() override;
	virtual void GetDirectionalReact(const FVector& ImpactPoint) override; 
	virtual void LaunchUp() override { CombatComponent->StartLaunchingUp(); }

	virtual AController* GetEntityController() override { return GetController(); }

	virtual void IncreaseEnergy(const float Percentage) override { AttributeComponent->IncreaseEnergy(Percentage); }
	virtual bool RequiresEnergy(const float X) override { return AttributeComponent->RequiresEnergy(X); }
	virtual void SetEnergy(const float EnergyFromPossessor) override { AttributeComponent->SetEnergy(EnergyFromPossessor); }
	virtual void IncreaseHealth(const float X) override { AttributeComponent->IncreaseHealth(X); }

	virtual float PlayAnimMontage(UAnimMontage* Montage, const float Rate = 1.f, const FName Section = "Default") override { return Super::PlayAnimMontage(Montage, Rate, Section); }
	virtual void StopAnimMontage(UAnimMontage* MontageToStop = nullptr) override { Super::StopAnimMontage(MontageToStop); }
    virtual USceneComponent* GetMeshComponent() override { return GetMesh(); }
    virtual void PauseAnims(const bool bEnable) override { GetMesh()->bPauseAnims = bEnable; }
	virtual void ChangeWeaponAnimationState() override; 

	virtual UCombatStrategy* GetCurrentStrategy() override { return CurrentStrategy; } 
	
	
	// === Gameplay Actions ===
	UFUNCTION(BlueprintCallable, Category = "Combat | Weapon")
	virtual void SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled);

	void HitReactJumpToSection(FName Section);
	
	/*void AttachFollowCamera(USpringArmComponent* AttachTarget);*/

	virtual void StunBehavior();
	virtual void RemoveStunBehavior();

	UFUNCTION()
	void OnWallCollision();
	
	bool IsEquipping() const;

protected:
	// === Actor Functions ===
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform &Transform) override;

	// === Input Setup ===
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// === Movement Inherited Functions ===
	virtual void Landed(const FHitResult& Hit) override;
	virtual void Jump() override;
	
	void SetCombatStrategy(ECharacterModeStates Mode);
	void InitializeComponentsData() const;

	// === Damage & Equipping ===
	virtual float TakeDamage(
		float DamageAmount,
		FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser) override;

	virtual void Die(UAnimMontage* DeathAnim, FName Section = NAME_None);
	
	// === Inherited Data ===
	UPROPERTY()
	TScriptInterface<ICombatTargetInterface> LastDamageCauser;

	UPROPERTY(Transient)
	APlayerController* PlayerControllerRef = nullptr;

	// === Input Mapping Context ===
	UPROPERTY(EditAnywhere, Category = "Input | Mapping")
	TObjectPtr<UInputMappingContext> CharacterContext;

	// === Camera ===
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	ACameraActor* FollowCamera;
	
	UPROPERTY()
	TArray<AActor*> IgnoreActors;
	
	// === Input Functions ===
	void Input_Move(const FInputActionValue& Value);
	
	void Input_Look(const FInputActionValue& Value);

	void Input_DoubleJump();

	UFUNCTION()
	void Input_Dodge();

	UFUNCTION()
	void Input_PrimaryAttack(const FInputActionValue& Value);

	UFUNCTION()
	void Input_SecondaryAttack();

	void Input_Block(const FInputActionValue& Value);

	void Input_ChangeHardLockTarget();

	void Input_ToggleHardLock();

	UFUNCTION()
	void Input_Execute();

	void Input_Interact(const FInputActionValue& InputActionValue);
	
private:
	// === Aux. Functions ===
	UFUNCTION()
	void PerformPrimaryAttack();
	void OnPrimaryAttackStarted();
	void OnPrimaryAttackCompleted();
	
	float TimeOfPrimaryAttackPressed;
	
	// === Components ===
	UPROPERTY(BlueprintGetter = GetCombatComponent)
	UCombatComponent* CombatComponent;

	UPROPERTY(BlueprintGetter = GetAttributeComponent)
	UAttributeComponent* AttributeComponent;

	UPROPERTY(BlueprintGetter = GetCharacterStateComponent)
	UCharacterStateComponent* CharacterStateComponent;

	UPROPERTY(VisibleAnywhere ,BlueprintGetter = GetExtraMovementComponent)
	UExtraMovementComponent* ExtraMovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintGetter = GetInventoryComponent)
	UInventoryComponent* InventoryComponent;

	UPROPERTY(BlueprintGetter = GetMementoComponent)
	UMementoComponent* MementoComponent;

	UPROPERTY(BlueprintGetter = GetPossessionComponent)
	UPossessionComponent* PossessionComponent;

	UPROPERTY(BlueprintGetter = GetTargetingComponent)
	UTargetingComponent* TargetingComponent;
	
	UPROPERTY(BlueprintGetter = GetSpringArmComponent)
	USpringArmComponent* SpringArmComponent;

	UPROPERTY(BlueprintGetter = GetCameraComponent)
	UCameraComponent* CameraComponent;
	
	UPROPERTY()
	UCombatStrategy* CurrentStrategy;
	UPROPERTY()
	UCombatStrategy* HumanStrategyInstance;
	UPROPERTY()
	UCombatStrategy* SpectralStrategyInstance;
};
