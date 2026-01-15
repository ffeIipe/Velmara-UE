#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "AbilitySystemInterface.h"
#include "GameplayTagAssetInterface.h"
#include "Features/SaveSystem/Interfaces/SaveInterface.h"

#include "Entity.generated.h"

class UMantleComponent;
class IPickable;
class UInventoryComponent;
class UTargetingComponent;
class UCameraComponent;
class UMotionWarpingComponent;
struct FHitReactDefinition;
struct FInputActionValue;
class UVelmaraAttributeSet;
class UInputMappingContext;
class UGameplayEffect;
class USpringArmComponent;
class UEntityData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEntityDead, AEntity*, EntityDead);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEntityDamaged, AEntity*, LastDamageCauser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEntityCanBeFinished);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEntityShieldTakeDamage);

UCLASS()
class TESISUE_API AEntity : public ACharacter, public ISaveInterface, public IGameplayTagAssetInterface, public IAbilitySystemInterface
{
	
	GENERATED_BODY()

public:
	AEntity();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data")
	TObjectPtr<UEntityData> EntityData;

	virtual FName GetUniqueSaveID_Implementation() override { return UniqueSaveID; }
	
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	virtual void OnSaveGame_Implementation(FEntitySaveData& OutData) override;

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	virtual void OnLoadGame_Implementation(const FEntitySaveData& InData) override;
	
	UFUNCTION(BlueprintPure, Category = "Components")
	FORCEINLINE UInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	UFUNCTION(BlueprintPure, Category = "Components")
	FORCEINLINE UTargetingComponent* GetTargetingComponent() const { return TargetingComponent; }

	UFUNCTION(BlueprintPure, Category = "Components")
	FORCEINLINE USpringArmComponent* GetSpringArmComponent() const { return SpringArmComponent; }

	UFUNCTION(BlueprintPure, Category = "Components")
	FORCEINLINE UCameraComponent* GetCameraComponent() const { return CameraComponent; }

	UFUNCTION(BlueprintPure, Category = "Components")
	FORCEINLINE UMotionWarpingComponent* GetMotionWarpingComponent() const { return MotionWarpingComponent; }

	UFUNCTION(BlueprintPure, Category = "Components")
	FORCEINLINE UMantleComponent* GetMantleComponent() const { return MantleComponent; }
	
	UPROPERTY(BlueprintAssignable)
	FOnEntityCanBeFinished OnCanBeFinished;

	UPROPERTY(BlueprintAssignable)
	FOnEntityShieldTakeDamage OnShieldTakeDamage;
	
	UFUNCTION(BlueprintPure)
	TScriptInterface<IPickable> GetCurrentItem() const; 

	UFUNCTION(BlueprintPure)
	AActor* GetLastDamageCauser() const { return LastDamageCauser; }

	virtual ACharacter* GetCharacter() { return this; }
	virtual UCharacterMovementComponent* GetCharacterMovementComponent() { return GetCharacterMovement(); }
	
	UFUNCTION(BlueprintCallable)
	virtual FName GetDirectionalReact(const FVector& ImpactPoint); 

	UFUNCTION(BlueprintCallable, Category = "Combat | Weapon")
	virtual void SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled);

	UPROPERTY(BlueprintAssignable)
	FOnEntityDead OnDead;

	/*UPROPERTY(BlueprintAssignable)
	FOnEntityDamaged OnDamaged;*/

	UFUNCTION(BlueprintCallable, Category = "Entity Flag")
	bool IsAlive() const;

	UFUNCTION(BlueprintCallable, Category = "Entity Flag")
	bool IsShielded() const;

	UFUNCTION(BlueprintCallable, Category = "Entity Flag")
	bool HasEnergy() const;

	UFUNCTION(BlueprintCallable, Category = "Entity Flag")
	bool IsStunned() const;

	//TODO: Check
	virtual void Landed(const FHitResult& Hit) override;

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

	UFUNCTION(BlueprintCallable)
	virtual void PerformDeath();

	UFUNCTION(BlueprintCallable)
	bool GetHitSectionForTag(FGameplayTag IncomingTag, FHitReactDefinition& OutDefinition) const;

	UFUNCTION(BlueprintCallable)
	FName GetDeathSectionForTag(FGameplayTag IncomingTag) const;

	UFUNCTION()
	void OnBodyPartOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION(BlueprintCallable, Category = "Entity|Unarmed")
	void ActivateBodyHitbox(FName ComponentTag, FGameplayTag DamageType);

	UFUNCTION(BlueprintCallable, Category = "Entity|Unarmed")
	void DeactivateBodyHitbox(FName ComponentTag) const;
	
protected:
	virtual void InitializeAttributeSet();
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TSubclassOf<UGameplayEffect> DefaultAttributeEffect;
	
	virtual void BeginPlay() override;
	
	virtual void OnConstruction(const FTransform &Transform) override;
	
	void InitializeComponentsData() const;

	void GiveDefaultAbilities();
	
	UPROPERTY()
	AActor* LastDamageCauser;

	UPROPERTY(Transient)
	APlayerController* PlayerControllerRef = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input | Mapping")
	TObjectPtr<UInputMappingContext> CharacterContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	ACameraActor* FollowCamera;
	
	UPROPERTY()
	TArray<AActor*> IgnoreActors;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	UAbilitySystemComponent* AbilitySystemComponent;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintGetter = GetInventoryComponent)
	UInventoryComponent* InventoryComponent;

	UPROPERTY(VisibleDefaultsOnly,BlueprintGetter = GetTargetingComponent)
	UTargetingComponent* TargetingComponent;
	
	UPROPERTY(VisibleDefaultsOnly,BlueprintGetter = GetSpringArmComponent)
	USpringArmComponent* SpringArmComponent;

	UPROPERTY(VisibleDefaultsOnly,BlueprintGetter = GetCameraComponent)
	UCameraComponent* CameraComponent;

	UPROPERTY(VisibleDefaultsOnly,BlueprintGetter = GetMotionWarpingComponent)
	UMotionWarpingComponent* MotionWarpingComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintGetter = GetMantleComponent)
	UMantleComponent* MantleComponent;
	
	UPROPERTY(VisibleDefaultsOnly)
	UVelmaraAttributeSet* AttributeSet;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName UniqueSaveID;
};