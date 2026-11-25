#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "AbilitySystemInterface.h"
#include "GameplayTagAssetInterface.h"
#include "GenericTeamAgentInterface.h"
#include "Features/SaveSystem/Interfaces/SaveInterface.h"
#include "Interfaces/Damageable.h"

#include "Entity.generated.h"

struct FGameplayAbilitySpecHandle;
class IPickable;
class UInventoryComponent;
class UTargetingComponent;
class UCameraComponent;
class UMotionWarpingComponent;
struct FHitReactDefinition;
struct FWeaponKnockback;
class UWeaponData;
class UCurveFloat;
struct FInputActionValue;
class UVelmaraAttributeSet;
class UInputMappingContext;
class UGameplayEffect;
class USpringArmComponent;
class UEntityData;

class AOrbBase;

USTRUCT(BlueprintType)
struct FEntityOrbDrop
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<AOrbBase> OrbClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1"))
	int32 Count = 1;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEntityDead, AEntity*, EntityDead);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEntityDamaged, AEntity*, LastDamageCauser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEntityCanBeFinished);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEntityShieldTakeDamage);

UCLASS()
class TESISUE_API AEntity : public ACharacter, public ISaveInterface, public IGameplayTagAssetInterface,
                            public IAbilitySystemInterface, public IDamageable
{
	GENERATED_BODY()

public:
	AEntity();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data")
	TObjectPtr<UEntityData> EntityData;

	FTimerHandle PossessionRecoveryTimer;

	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;

	UFUNCTION(BlueprintCallable, Category = "Pool")
	void ResetForPoolSpawn();

	UFUNCTION(BlueprintCallable)
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	virtual void OnSaveGame_Implementation(FEntitySaveData& OutData) override;

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	virtual void OnLoadGame_Implementation(const FEntitySaveData& InData) override;

	virtual void OnPostGameLoaded_Implementation() override;

	virtual void ReceiveDamage_Implementation(FGameplayEventData DamagePayload) override;

	virtual void MortalDamage_Implementation(FGameplayEventData DeathPayload) override;
	
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
	virtual FGameplayTag GetDirectionalReact(const FVector& ImpactPoint); 

	UFUNCTION(BlueprintCallable, Category = "Combat | Weapon")
	virtual void SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled);

	virtual void PossessedBy(AController* NewController) override;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnEntityDead OnDead;

	void NotifyDeathToWorld();

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

	UFUNCTION()
	void OnBodyPartOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION(BlueprintCallable, Category = "Entity|Unarmed")
	void ActivateBodyHitbox(FName ComponentTag, FGameplayTag DamageType);

	UFUNCTION(BlueprintCallable, Category = "Entity|Unarmed")
	void DeactivateBodyHitbox(FName ComponentTag) const;

	UFUNCTION(BlueprintCallable, Category = "Entity|Effects")
	void HitFlash(float Duration, float Amount);

	void DeactivateHitFlash();

	UPROPERTY()
	FTimerHandle HitFlashTimerHandle;

	float HitFlashPeakAmount = 0.f;

	float HitFlashDuration = 0.1f;

	float HitFlashElapsed = 0.f;

	void UpdateHitFlash();

	void SetHitFlashAmount(float Amount);

	void ApplyKnockback(const FVector& AttackerLocation, const FWeaponKnockback& Knockback);

	UFUNCTION(BlueprintCallable, Category = "Combat|Juggle")
	void StartJuggle(float HoverDuration = -1.f);

	void StartAttackHover();

	UFUNCTION(BlueprintCallable, Category = "Combat|Juggle")
	void EndJuggle();

	UFUNCTION(BlueprintPure, Category = "Combat|Juggle")
	bool IsJuggled() const { return bJuggled; }

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Juggle", meta = (ClampMin = "0.1", Units = "s"))
	float DefaultJuggleDuration = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Juggle", meta = (ClampMin = "0.05", Units = "s"))
	float AirHitStunDuration = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Juggle", meta = (ClampMin = "0.1", Units = "s"))
	float AirComboJuggleDuration = 1.2f;

	void OnDamageReceived();

	FTimerHandle JuggleTimerHandle;

	bool bJuggled = false;

	bool bJuggleEndsWithAbility = false;

	TWeakObjectPtr<const UGameplayAbility> JuggleSustainAbility;

	float JuggleRemaining = 0.f;

	float JuggleSavedGravityScale = 1.f;

	void UpdateJuggle();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat")
	bool bKnockbackResistant = false;

	UPROPERTY(EditDefaultsOnly, Category = "Overlay")
	TObjectPtr<UWeaponData> InnateWeaponOverlay;

	FTimerHandle KnockbackTimerHandle;

	FVector KnockbackDirection = FVector::ZeroVector;

	float KnockbackDistance = 0.f;

	float KnockbackDuration = 0.15f;

	float KnockbackElapsed = 0.f;

	UPROPERTY()
	TObjectPtr<UCurveFloat> KnockbackCurve;

	void UpdateKnockback();

	void StopKnockback();

	float EvaluateKnockbackProgress(float Alpha) const;
	
	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> DissolveMaterials;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	FGenericTeamId TeamId; 

	UFUNCTION(BlueprintCallable, Category = "Team")
	bool IsHostile(const AEntity* OtherEntity) const;

	UFUNCTION(BlueprintCallable, Category = "Team")
	void SetTeam(ETeamAttitude::Type NewTeam);

	UFUNCTION(BlueprintCallable, Category = "Team")
	FGenericTeamId GetTeamId() const {return TeamId;}

	UFUNCTION(BlueprintCallable, Category = "Team")
	void SetTeamId(int NewTeamIndex);
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orbs")
	TArray<FEntityOrbDrop> DeathOrbDrops;

	void SpawnDeathOrbs();

	UFUNCTION(BlueprintCallable)
	virtual void InitializeAttributeSet();
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TSubclassOf<UGameplayEffect> DefaultAttributeEffect;

	UPROPERTY(BlueprintReadOnly, Category = "Abilities")
	TArray<FGameplayAbilitySpecHandle> GrantedAbilityHandles;
	
	virtual void BeginPlay() override;
	
	virtual void OnConstruction(const FTransform &Transform) override;
	
	void InitializeComponentsData() const;

	UFUNCTION(BlueprintCallable)
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

	UPROPERTY(VisibleDefaultsOnly)
	UVelmaraAttributeSet* AttributeSet;
};