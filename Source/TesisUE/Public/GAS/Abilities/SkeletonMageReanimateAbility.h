#pragma once

#include "CoreMinimal.h"
#include "GAS/VelmaraGameplayAbility.h"
#include "SkeletonMageReanimateAbility.generated.h"

class UAnimMontage;
class UNiagaraSystem;
class USoundBase;

/*
 * Habilidad del SkeletonMage: levanta enemigos que murieron cerca (ver UGraveSubsystem). Pasado CastEventDelay del casteo
 * aparece el efecto en cada tumba y, RiseDelay después, el enemigo sale del pool en ese lugar. Solo se puede activar si hay
 * alguna tumba lista, así que el BT puede intentarla sin chequear nada. Tag: Ability.Reanimate.
 */
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

	// Segundos desde el inicio del casteo hasta que se marcan las tumbas (se escala con AttackSpeed).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reanimate|Cast", meta = (ClampMin = "0"))
	float CastEventDelay = 0.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reanimate|Graves", meta = (ClampMin = "0"))
	float SearchRadius = 1800.f;

	// Muertes más viejas que esto ya no se pueden levantar (máximo 120).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reanimate|Graves", meta = (ClampMin = "0", ClampMax = "120"))
	float MaxGraveAge = 30.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reanimate|Graves", meta = (ClampMin = "1"))
	int32 MaxRevivesPerCast = 2;

	// Tiempo entre el efecto en la tumba y la aparición del enemigo: la ventana del jugador para reaccionar.
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
