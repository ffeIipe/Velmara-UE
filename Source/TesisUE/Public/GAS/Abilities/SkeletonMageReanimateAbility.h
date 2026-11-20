#pragma once

#include "CoreMinimal.h"
#include "GAS/VelmaraGameplayAbility.h"
#include "SkeletonMageReanimateAbility.generated.h"

class UAnimMontage;
class UNiagaraSystem;
class USoundBase;

UCLASS()
class TESISUE_API USkeletonMageReanimateAbility : public UVelmaraGameplayAbility
{
	GENERATED_BODY()

public:
	USkeletonMageReanimateAbility();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reanimate|Cast")
	TObjectPtr<UAnimMontage> CastMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reanimate|Cast", meta = (ClampMin = "0"))
	float CastEventDelay = 0.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reanimate|Graves", meta = (ClampMin = "0"))
	float SearchRadius = 1800.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reanimate|Graves", meta = (ClampMin = "0", ClampMax = "120"))
	float MaxGraveAge = 30.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reanimate|Graves", meta = (ClampMin = "1"))
	int32 MaxRevivesPerCast = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reanimate|Feedback", meta = (ClampMin = "0"))
	float RiseDelay = 1.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reanimate|Feedback")
	TObjectPtr<UNiagaraSystem> GraveEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reanimate|Feedback")
	TObjectPtr<USoundBase> RiseSound;

private:
	UFUNCTION()
	void ReanimateGraves();

	UFUNCTION()
	void HandleMontageEnded();
};
