#pragma once

#include "CoreMinimal.h"
#include "Features/BossSystem/Data/BossTypes.h"
#include "GAS/VelmaraGameplayAbility.h"
#include "ShamanStaffMagicAbility.generated.h"

class ABossProjectile;
class UAbilitySystemComponent;
class UAnimMontage;
class UAudioComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class USoundBase;

UCLASS()
class TESISUE_API UShamanStaffMagicAbility : public UVelmaraGameplayAbility
{
	GENERATED_BODY()

public:
	UShamanStaffMagicAbility();

	virtual bool CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

	virtual float GetCooldownTimeRemaining(const FGameplayAbilityActorInfo* ActorInfo) const override;

	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shaman|Cast")
	TObjectPtr<UAnimMontage> CastMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shaman|Cast", meta = (ClampMin = "0"))
	float FireDelay = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shaman|Cast")
	FName StaffTipSocket = TEXT("StaffTip");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shaman|Telegraph", meta = (ClampMin = "0"))
	float AIFireDelay = 0.85f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shaman|Telegraph")
	TObjectPtr<UNiagaraSystem> ChargeEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shaman|Telegraph")
	FName ChargeDurationParameter = TEXT("ChargeDuration");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shaman|Telegraph")
	TObjectPtr<UNiagaraSystem> TellEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shaman|Telegraph", meta = (ClampMin = "0"))
	float TellLeadTime = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shaman|Telegraph")
	TObjectPtr<USoundBase> ChargeSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shaman|Projectile")
	TSubclassOf<ABossProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shaman|Projectile")
	float ProjectileSpeed = 1800.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shaman|Projectile")
	float ProjectileLifetime = 3.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shaman|Projectile")
	FBossDamageParams DamageParams;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shaman|Player", meta = (ClampMin = "0"))
	float PlayerDamage = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shaman|Player", meta = (ClampMin = "0"))
	float PlayerProjectileSpeed = 1800.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shaman|Player", meta = (ClampMin = "0"))
	float PlayerCooldown = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shaman|Player", meta = (ClampMin = "0"))
	float PlayerEnergyCost = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shaman|Player")
	bool bPlayerHomingOnLock = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shaman|Player", meta = (EditCondition = "bPlayerHomingOnLock", ClampMin = "0"))
	float PlayerHomingAcceleration = 6000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shaman|Player")
	FGameplayTag LockingTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shaman|Player", meta = (ClampMin = "100"))
	float PlayerAimTraceDistance = 5000.f;

private:
	UFUNCTION()
	void FireProjectile();

	UFUNCTION()
	void HandleMontageEnded();

	UFUNCTION()
	void PlayTell();

	UFUNCTION()
	void HandleAvatarControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

	void StartTelegraph(float ChargeDuration);

	void StopTelegraph();

	void RestoreCastMontageRate() const;

	void StartPlayerCastRotation();

	void StopPlayerCastRotation();

	bool IsPlayerControlledAvatar(const FGameplayAbilityActorInfo* ActorInfo) const;

	UAbilitySystemComponent* GetPlayerEnergyASC(const FGameplayAbilityActorInfo* ActorInfo) const;

	bool IsLockingTarget(const AActor* Avatar) const;

	AActor* FindLockedTarget(const AActor* Avatar) const;

	AActor* FindAITarget(const AActor* Avatar) const;

	FVector ResolvePlayerAimPoint(const AActor* Avatar, AActor*& OutLockedTarget) const;

	USceneComponent* FindStaffTipComponent(const AActor* Avatar) const;

	FVector GetStaffTipLocation(const AActor* Avatar) const;

	FVector GetSafeSpawnLocation(const AActor* Avatar, float ProbeRadius) const;

	mutable double NextPlayerCastTime = 0.0;

	bool bActivatedByPlayer = false;

	float CastMontageRate = 1.f;

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> ChargeComponent;

	UPROPERTY()
	TObjectPtr<UAudioComponent> ChargeAudio;

	bool bOverrodeWantsToStrafe = false;

	bool bSavedWantsToStrafe = false;

	bool bOverrodeMovementRotation = false;

	bool bSavedOrientRotationToMovement = true;

	bool bSavedUseControllerDesiredRotation = false;
};
