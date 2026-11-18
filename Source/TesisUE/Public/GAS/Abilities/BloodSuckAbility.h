#pragma once

#include "CoreMinimal.h"
#include "GAS/VelmaraGameplayAbility.h"
#include "BloodSuckAbility.generated.h"

class UAnimMontage;
class UGameplayEffect;
class AEntity;

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

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BloodSuck")
	TObjectPtr<UAnimMontage> SuckMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BloodSuck")
	TSubclassOf<UGameplayEffect> HealEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BloodSuck")
	float GrabRange = 600.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BloodSuck")
	float BiteDistance = 120.f;

	// Hueso del enemigo al que se pega el mesh de Velmara (conserva la pose a la espalda que ya tiene al engancharse).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BloodSuck")
	FName AttachBone = TEXT("spine_03");

	// Distancia a la que Velmara queda pegada a la espalda del enemigo.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BloodSuck")
	float BackDistance = 60.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BloodSuck")
	TObjectPtr<UAnimMontage> VictimReactMontage;

private:
	UPROPERTY()
	TWeakObjectPtr<AEntity> Victim;

	bool bIgnoringMoveInput = false;
	bool bAttached = false;
	FTransform SavedMeshTransform;

	AEntity* FindVictim(const AEntity* Self) const;
	void SetVictimFrozen(bool bFrozen) const;

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageInterrupted();
};
