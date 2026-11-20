#pragma once

#include "CoreMinimal.h"
#include "Features/BossSystem/Data/BossTypes.h"
#include "GAS/VelmaraGameplayAbility.h"
#include "ShamanStaffMagicAbility.generated.h"

class ABossProjectile;
class UAnimMontage;

UCLASS()
class TESISUE_API UShamanStaffMagicAbility : public UVelmaraGameplayAbility
{
	GENERATED_BODY()

public:
	UShamanStaffMagicAbility();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shaman|Cast")
	TObjectPtr<UAnimMontage> CastMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shaman|Cast", meta = (ClampMin = "0"))
	float FireDelay = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shaman|Cast")
	FName StaffTipSocket = TEXT("StaffTip");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shaman|Projectile")
	TSubclassOf<ABossProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shaman|Projectile")
	float ProjectileSpeed = 1800.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shaman|Projectile")
	float ProjectileLifetime = 3.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shaman|Projectile")
	FBossDamageParams DamageParams;

private:
	UFUNCTION()
	void FireProjectile();

	UFUNCTION()
	void HandleMontageEnded();

	AActor* FindTarget() const;

	FVector GetStaffTipLocation(const AActor* Avatar) const;
};
