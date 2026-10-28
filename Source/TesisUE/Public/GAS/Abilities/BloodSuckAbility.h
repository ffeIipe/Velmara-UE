#pragma once

#include "CoreMinimal.h"
#include "GAS/VelmaraGameplayAbility.h"
#include "BloodSuckAbility.generated.h"

class UAnimMontage;
class UGameplayEffect;

/*
 * Chupar sangre: Velmara reproduce SuckMontage y, si termina sin que la interrumpan, se aplica HealEffect a sí misma
 * (vida + energía, los valores viven en el GE). El cooldown es el CooldownGameplayEffectClass de la habilidad. Tag: Ability.BloodSuck.
 */
UCLASS()
class TESISUE_API UBloodSuckAbility : public UVelmaraGameplayAbility
{
	GENERATED_BODY()

public:
	UBloodSuckAbility();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BloodSuck")
	TObjectPtr<UAnimMontage> SuckMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BloodSuck")
	TSubclassOf<UGameplayEffect> HealEffect;

private:
	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageInterrupted();
};
