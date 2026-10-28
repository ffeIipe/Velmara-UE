#pragma once

#include "CoreMinimal.h"
#include "GAS/VelmaraGameplayAbility.h"
#include "SkeletonMageBlinkAbility.generated.h"

class UNiagaraSystem;
class USoundBase;

/*
 * Escape del SkeletonMage: si el objetivo está a menos de TriggerRange, el mago se deshace (oculto y sin colisión) y a los
 * VanishDuration reaparece en un punto del navmesh entre Min/MaxDistanceFromTarget del objetivo, mirándolo.
 * Solo se puede activar con el objetivo cerca, así que el BT puede intentarla siempre. Tag: Ability.Blink.
 */
UCLASS()
class TESISUE_API USkeletonMageBlinkAbility : public UVelmaraGameplayAbility
{
	GENERATED_BODY()

public:
	USkeletonMageBlinkAbility();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Blink", meta = (ClampMin = "0"))
	float TriggerRange = 450.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Blink", meta = (ClampMin = "0"))
	float MinDistanceFromTarget = 900.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Blink", meta = (ClampMin = "0"))
	float MaxDistanceFromTarget = 1400.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Blink", meta = (ClampMin = "0"))
	float VanishDuration = 0.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Blink|Feedback")
	TObjectPtr<UNiagaraSystem> VanishEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Blink|Feedback")
	TObjectPtr<UNiagaraSystem> AppearEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Blink|Feedback")
	TObjectPtr<USoundBase> BlinkSound;

private:
	UFUNCTION()
	void Reappear();

	void SetVanished(bool bVanish);

	AActor* FindTarget(const AActor* Avatar) const;

	bool FindDestination(const AActor* Avatar, const AActor* Target, FVector& OutLocation) const;

	FVector Destination = FVector::ZeroVector;

	TWeakObjectPtr<AActor> BlinkTarget;

	bool bVanished = false;
};
