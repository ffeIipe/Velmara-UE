#pragma once

#include "CoreMinimal.h"
#include "Features/BossSystem/Data/BossTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BossSystemLibrary.generated.h"

UCLASS()
class TESISUE_API UBossSystemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Vivo, con ASC y hostil al boss (o controlado por un jugador si bOnlyPlayerControlled).
	UFUNCTION(BlueprintPure, Category = "Boss")
	static bool IsValidBossTarget(const AActor* Boss, const AActor* Target, bool bOnlyPlayerControlled = true);

	// True si el objetivo tiene alguno de los tags de inmunidad (ej. State.Dodging).
	UFUNCTION(BlueprintPure, Category = "Boss")
	static bool IsImmuneToBossHit(const AActor* Target, const FGameplayTagContainer& ImmunityTags);

	// Aplica Params.Damage * DamageMultiplier con el GE configurado. Sin ASC en el instigador, el daño se lo aplica el propio objetivo (como el sol).
	UFUNCTION(BlueprintCallable, Category = "Boss")
	static bool ApplyBossDamage(AActor* Instigator, AActor* DamageCauser, AActor* Target, const FBossDamageParams& Params, float DamageMultiplier, const FHitResult& Hit);
};
