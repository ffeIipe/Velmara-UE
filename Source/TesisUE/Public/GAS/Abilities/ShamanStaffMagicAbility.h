#pragma once

#include "CoreMinimal.h"
#include "Features/BossSystem/Data/BossTypes.h"
#include "GAS/VelmaraGameplayAbility.h"
#include "ShamanStaffMagicAbility.generated.h"

class ABossProjectile;
class UAnimMontage;

/*
 * Ataque a distancia del ShamanOrc: reproduce el montage de casteo y, pasado FireDelay, dispara un proyectil mágico
 * desde la punta del bastón hacia el TargetActor del blackboard (o el jugador). Solo daña pawns controlados por el jugador
 * y se esquiva con el dodge. Se otorga desde DefaultAbilities del EntityData del orco.
 */
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

	// Segundos desde el inicio del casteo hasta que sale el proyectil (se escala con AttackSpeed).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shaman|Cast", meta = (ClampMin = "0"))
	float FireDelay = 0.5f;

	// Se busca en todos los componentes del orco (su mesh o la del bastón). Si no existe, sale frente al orco.
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
