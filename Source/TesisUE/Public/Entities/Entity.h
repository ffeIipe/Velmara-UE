#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "Camera/CameraComponent.h"
#include "Components/AttributeComponent.h"
#include "Components/CharacterStateComponent.h"
#include "Components/CombatComponent.h"
#include "Components/ExtraMovementComponent.h"
#include "Components/InventoryComponent.h"
#include "Components/PossessionComponent.h"
#include "Components/TargetingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

// Interfaces
#include "Interfaces/AnimatorProvider.h"
#include "Interfaces/BufferComponentProvider.h"
#include "Interfaces/HitInterface.h"
#include "Interfaces/CameraProvider.h"
#include "Interfaces/CharacterStateProvider.h"
#include "Interfaces/CharacterMovementProvider.h"
#include "Interfaces/CombatComponentProvider.h"
#include "Interfaces/OwnerUtilsInterface.h"
#include "Interfaces/CombatTargetInterface.h"
#include "Interfaces/ControllerProvider.h"
#include "Interfaces/FieldCreationComponentProvider.h"
#include "Interfaces/StrategyInterface.h"
#include "Interfaces/Weapon/WeaponProvider.h"
#include "Interfaces/SaveInterface.h"

#include "GameplayTagAssetInterface.h"
#include "AbilitySystemInterface.h"

#include "Entity.generated.h"

/*
 * =========-Forward Declarations=========-
 */

class UGameplayEffect;
class UVelmaraAttributeSet;
class UBufferComponent;
class UFieldCreationComponent;
class UMontagesData;
class UEffectsData;
class UInputData;
class UTargetingComponent;
class UCameraComponent;
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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEntityDead, AEntity*, EntityDead);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEntityDamaged, AEntity*, LastDamageCauser);
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
							public IAnimatorProvider,
							public IStrategyProvider,
							public ICombatComponentProvider,
							public IFieldCreationComponentProvider,
							public IBufferComponentProvider,
							public ISaveInterface,
							public IGameplayTagAssetInterface,
							public IAbilitySystemInterface
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
	
	// === Save System ===
	virtual FName GetUniqueSaveID_Implementation() override { return UniqueSaveID; }
	
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override { TagContainer = EntityTags; }

	UFUNCTION(BlueprintCallable, Category = "State | Tags")
	void AddGameplayTag(const FGameplayTag Tag) { EntityTags.AddTag(Tag); }

	UFUNCTION(BlueprintCallable, Category = "State | Tags")
	void RemoveGameplayTag(const FGameplayTag Tag) { EntityTags.RemoveTag(Tag); }

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	virtual void OnSaveGame_Implementation(FEntitySaveData& OutData) override;

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	virtual void OnLoadGame_Implementation(const FEntitySaveData& InData) override;
	
	UFUNCTION(BlueprintPure, Category = "Components")
	virtual UCombatComponent* GetCombatComponent_Implementation() override { return CombatComponent; };

	UFUNCTION(BlueprintPure, Category = "Components")
	FORCEINLINE UAttributeComponent* GetAttributeComponent() const { return AttributeComponent; }

	UFUNCTION(BlueprintPure, Category = "Components")
	virtual UCharacterStateComponent* GetCharacterStateComponent_Implementation() override { return CharacterStateComponent; }
	
	UFUNCTION(BlueprintPure, Category = "Components")
	virtual UExtraMovementComponent* GetExtraMovementComponent_Implementation() override { return ExtraMovementComponent; }

	UFUNCTION(BlueprintPure, Category = "Components")
	FORCEINLINE UInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	UFUNCTION(BlueprintPure, Category = "Components")
	FORCEINLINE UPossessionComponent* GetPossessionComponent() const { return PossessionComponent; }

	UFUNCTION(BlueprintPure, Category = "Components")
	FORCEINLINE UTargetingComponent* GetTargetingComponent() const { return TargetingComponent; }

	UFUNCTION(BlueprintPure, Category = "Components")
	FORCEINLINE USpringArmComponent* GetSpringArmComponent() const { return SpringArmComponent; }

	UFUNCTION(BlueprintPure, Category = "Components")
	FORCEINLINE UCameraComponent* GetCameraComponent() const { return CameraComponent; }

	UFUNCTION(BlueprintPure, Category = "Components")
	virtual UFieldCreationComponent* GetFieldCreationComponent_Implementation() override { return FieldCreationComponent; }

	UFUNCTION(BlueprintPure, Category = "Components")
	virtual UBufferComponent* GetBufferComponent_Implementation() override { return BufferComponent; }
	
	UPROPERTY(BlueprintAssignable)
	FOnEntityCanBeFinished OnCanBeFinished;

	UPROPERTY(BlueprintAssignable)
	FOnEntityShieldTakeDamage OnShieldTakeDamage;
	
	// === Hit Interface ===
	virtual void GetHit(TScriptInterface<ICombatTargetInterface> DamageCauser, const FVector& ImpactPoint,
	                    FDamageEvent const& DamageEvent, const float DamageReceived) override;
	virtual void GetFinished() override;
	virtual bool IsHittable() override;
	virtual void AddStunBehavior() override;
	virtual void RemoveStunBehavior() override;
	
	// === Camera Provider Interface ===
	virtual UCameraComponent* GetEntityCamera() override { return GetCameraComponent(); }
	virtual FVector GetCameraLocation() override { return GetCameraComponent()->GetComponentLocation(); }

	// === Weapon Provider Interface ===
	UFUNCTION(BlueprintPure)
	virtual TScriptInterface<IWeaponInterface> GetCurrentWeapon_Implementation() override { return GetInventoryComponent()->GetCurrentWeapon(); }

	// === Combat Target Interface ===
	UFUNCTION(BlueprintPure)
	virtual TScriptInterface<ICombatTargetInterface> GetLastDamageCauser() override { return LastDamageCauser; }
	virtual bool IsFalling() override { return GetCharacterMovement()->IsFalling(); }
	virtual bool IsFlying() override { return GetCharacterMovement()->IsFlying(); }
	virtual bool IsMovingBackwards() override { return ExtraMovementComponent->IsMovingBackwards(); }
	virtual bool IsEquipped() override;
	virtual bool IsLocking() override { return GetTargetingComponent()->IsLocking(); }
	virtual void SetMovementMode(const EMovementMode MovementMode) override { GetCharacterMovement()->SetMovementMode(MovementMode); }

	// === Character Movement Provider ===
	virtual ACharacter* GetCharacter_Implementation() override { return this; }
	virtual UCharacterMovementComponent* GetCharacterMovementComponent_Implementation() override { return GetCharacterMovement(); }
	
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

	virtual float PlayAnimMontage_Implementation(UAnimMontage* Montage, const float Rate = 1.f, const FName Section = "Default") override { return Super::PlayAnimMontage(Montage, Rate, Section); }
	virtual void StopAnimMontage_Implementation(UAnimMontage* MontageToStop = nullptr) override { Super::StopAnimMontage(MontageToStop); }
    virtual USceneComponent* GetMeshComponent() override { return GetMesh(); }
    virtual void PauseAnims(const bool bEnable) override { GetMesh()->bPauseAnims = bEnable; }
	virtual void ChangeWeaponAnimationState() override; 

	virtual UCombatStrategy* GetCurrentStrategy() override { return CurrentStrategy; } 
	
	// === Gameplay Actions ===
	UFUNCTION(BlueprintCallable, Category = "Combat | Weapon")
	virtual void SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled);

	void HitReactJumpToSection(FName Section);

	UFUNCTION()
	void OnWallCollision();
	
	bool IsEquipping() const;

	// === Delegates ===
	UPROPERTY(BlueprintAssignable)
	FOnEntityDead OnDead;

	UPROPERTY(BlueprintAssignable)
	FOnEntityDamaged OnDamaged;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State | Tags")
	FGameplayTagContainer EntityTags;

	virtual void InitializeAttributes();
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TSubclassOf<UGameplayEffect> DefaultAttributeEffect;
	
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

	UFUNCTION()
	void Input_Dodge();

	UFUNCTION()
	void Input_PrimaryAttack(const FInputActionValue& Value);

	/*void OnPrimaryAttackReleased(const FInputActionValue& Value);*/
	
	UFUNCTION()
	void Input_SecondaryAttack();

	UFUNCTION()
	void Input_Ability();
	
	void Input_Block(const FInputActionValue& Value);

	void Input_ChangeHardLockTarget();

	void Input_ToggleHardLock();

	void Input_Interact(const FInputActionValue& InputActionValue);

	void Input_ToggleWeapon();

	// === Components ===
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UAbilitySystemComponent* AbilitySystemComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UCombatComponent* CombatComponent;

	UPROPERTY(BlueprintGetter = GetAttributeComponent)
	UAttributeComponent* AttributeComponent;

	UPROPERTY(BlueprintReadOnly)
	UCharacterStateComponent* CharacterStateComponent;

	UPROPERTY(BlueprintReadOnly)
	UExtraMovementComponent* ExtraMovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintGetter = GetInventoryComponent, SaveGame) //???
	UInventoryComponent* InventoryComponent;

	UPROPERTY(BlueprintGetter = GetPossessionComponent)
	UPossessionComponent* PossessionComponent;

	UPROPERTY(BlueprintGetter = GetTargetingComponent)
	UTargetingComponent* TargetingComponent;
	
	UPROPERTY(BlueprintGetter = GetSpringArmComponent)
	USpringArmComponent* SpringArmComponent;

	UPROPERTY(BlueprintGetter = GetCameraComponent)
	UCameraComponent* CameraComponent;

	UPROPERTY()
	UVelmaraAttributeSet* AttributeSet;
	
	UPROPERTY()
	TObjectPtr<UFieldCreationComponent> FieldCreationComponent;

	UPROPERTY()
	TObjectPtr<UBufferComponent> BufferComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName UniqueSaveID;
	
	UPROPERTY(Transient)
	TMap<ECharacterModeStates, TObjectPtr<UCombatStrategy>> StrategyInstances;

	UPROPERTY()
	TObjectPtr<UCombatStrategy> CurrentStrategy;
	
private:
	bool bPrimaryInputHeld;

	UFUNCTION()
	void EnableControllerRotationYaw();

	UFUNCTION()
	void DisableControllerRotationYaw();
};
