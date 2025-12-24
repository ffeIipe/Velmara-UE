#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "Camera/CameraComponent.h"
#include "Components/CombatComponent.h"
#include "Components/ExtraMovementComponent.h"
#include "Components/InventoryComponent.h"
#include "Components/TargetingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Interfaces/HitInterface.h"
#include "Interfaces/SaveInterface.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagAssetInterface.h"

#include "Entity.generated.h"


struct FInputActionValue;
class UVelmaraAttributeSet;
class UInputMappingContext;
class UGameplayEffect;
class USpringArmComponent;
class UMontagesData;
class UEffectsData;
class UEntityData;

// === Delegates ===
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEntityDead, AEntity*, EntityDead);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEntityDamaged, AEntity*, LastDamageCauser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEntityCanBeFinished);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEntityShieldTakeDamage);

UCLASS()
class TESISUE_API AEntity : public ACharacter, public IHitInterface, public ISaveInterface, public IGameplayTagAssetInterface, public IAbilitySystemInterface
{
	
	GENERATED_BODY()

public:
	AEntity();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data")
	TObjectPtr<UEntityData> EntityData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data")
	TObjectPtr<UEffectsData> EffectsData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data")
	TObjectPtr<UMontagesData> MontagesData;

	// === Save System ===
	virtual FName GetUniqueSaveID_Implementation() override { return UniqueSaveID; }
	
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;

	UFUNCTION(BlueprintCallable, Category = "State | Tags")
	void AddGameplayTag(const FGameplayTag Tag) const;

	UFUNCTION(BlueprintCallable, Category = "State | Tags")
	void RemoveGameplayTag(const FGameplayTag Tag) const;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	virtual void OnSaveGame_Implementation(FEntitySaveData& OutData) override;

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	virtual void OnLoadGame_Implementation(const FEntitySaveData& InData) override;
	
	UFUNCTION(BlueprintPure, Category = "Components")
	virtual UCombatComponent* GetCombatComponent() { return CombatComponent; };

	UFUNCTION(BlueprintPure, Category = "Components")
	virtual UExtraMovementComponent* GetExtraMovementComponent() { return ExtraMovementComponent; }

	UFUNCTION(BlueprintPure, Category = "Components")
	FORCEINLINE UInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	UFUNCTION(BlueprintPure, Category = "Components")
	FORCEINLINE UTargetingComponent* GetTargetingComponent() const { return TargetingComponent; }

	UFUNCTION(BlueprintPure, Category = "Components")
	FORCEINLINE USpringArmComponent* GetSpringArmComponent() const { return SpringArmComponent; }

	UFUNCTION(BlueprintPure, Category = "Components")
	FORCEINLINE UCameraComponent* GetCameraComponent() const { return CameraComponent; }
	
	UPROPERTY(BlueprintAssignable)
	FOnEntityCanBeFinished OnCanBeFinished;

	UPROPERTY(BlueprintAssignable)
	FOnEntityShieldTakeDamage OnShieldTakeDamage;
	
	// === Hit Interface ===
	virtual void GetHit(AActor* DamageCauser, const FVector& ImpactPoint,
	                    FDamageEvent const& DamageEvent, const float DamageReceived) override;
	virtual void GetFinished() override;
	virtual bool IsHittable() override;
	virtual void AddStunBehavior() override;
	virtual void RemoveStunBehavior() override;

	UFUNCTION(BlueprintPure)
	TScriptInterface<IWeaponInterface> GetCurrentWeapon() const; 

	UFUNCTION(BlueprintPure)
	AActor* GetLastDamageCauser() const { return LastDamageCauser; }

	virtual ACharacter* GetCharacter() { return this; }
	virtual UCharacterMovementComponent* GetCharacterMovementComponent() { return GetCharacterMovement(); }
	
	UFUNCTION(BlueprintCallable)
	virtual void GetDirectionalReact(const FVector& ImpactPoint); 
	virtual void LaunchUp() { CombatComponent->StartLaunchingUp(); }

	// === Gameplay Actions ===
	UFUNCTION(BlueprintCallable, Category = "Combat | Weapon")
	virtual void SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled);

	void HitReactJumpToSection(FName Section);

	//UFUNCTION()
	//void OnWallCollision();
	void PerformDoubleJump(UAnimMontage* DoubleJumpMontage);

	// === Delegates ===
	UPROPERTY(BlueprintAssignable)
	FOnEntityDead OnDead;

	UPROPERTY(BlueprintAssignable)
	FOnEntityDamaged OnDamaged;

	UFUNCTION(BlueprintCallable, Category = "Entity Flag")
	bool IsAlive() const;

	UFUNCTION(BlueprintCallable, Category = "Entity Flag")
	bool IsShielded() const;

	UFUNCTION(BlueprintCallable, Category = "Entity Flag")
	bool HasEnergy() const;

	UFUNCTION(BlueprintCallable, Category = "Entity Flag")
	bool IsStunned() const;

	UFUNCTION(BlueprintCallable, Category = "Entity Flag")
	bool IsBlocking() const;

	UFUNCTION(BlueprintCallable, Category = "Entity Flag")
	bool IsLaunchable() const;

	UFUNCTION(BlueprintCallable, Category = "Entity Flag")
	bool CanBeFinished();

	UFUNCTION(BlueprintCallable, Category = "Entity Flag")
	bool IsEquipped() const;

	UFUNCTION(BlueprintCallable, Category = "Entity Flag")
	bool IsInAir();
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State | Tags")
	FGameplayTagContainer EntityTags;

	virtual void InitializeAttributeSet();
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TSubclassOf<UGameplayEffect> DefaultAttributeEffect;

	void GiveDefaultAbilities();
	
	// === Actor Functions ===
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform &Transform) override;
	
	// === Movement Inherited Functions ===
	virtual void Landed(const FHitResult& Hit) override;
	virtual void Jump() override;
	
	void InitializeComponentsData() const;

	virtual void Die(UAnimMontage* DeathAnim, FName Section = NAME_None);
	
	// === Inherited Data ===
	UPROPERTY()
	AActor* LastDamageCauser;

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

	UFUNCTION(BlueprintCallable)
	void Interact();

	// === Components ===
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities") //fkn god
	UAbilitySystemComponent* AbilitySystemComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UCombatComponent* CombatComponent;

	UPROPERTY(BlueprintReadOnly)
	UExtraMovementComponent* ExtraMovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintGetter = GetInventoryComponent)
	UInventoryComponent* InventoryComponent;

	UPROPERTY(BlueprintGetter = GetTargetingComponent)
	UTargetingComponent* TargetingComponent;
	
	UPROPERTY(BlueprintGetter = GetSpringArmComponent)
	USpringArmComponent* SpringArmComponent;

	UPROPERTY(BlueprintGetter = GetCameraComponent)
	UCameraComponent* CameraComponent;

	UPROPERTY()
	UVelmaraAttributeSet* AttributeSet;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName UniqueSaveID;
	
private:
	bool bPrimaryInputHeld;

	UFUNCTION()
	void EnableControllerRotationYaw();

	UFUNCTION()
	void DisableControllerRotationYaw();

	bool CanDoubleJump;
	
};
