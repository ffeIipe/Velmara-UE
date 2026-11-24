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
	UFUNCTION(BlueprintPure, Category = "Boss")
	static bool IsValidBossTarget(const AActor* Boss, const AActor* Target, bool bOnlyPlayerControlled = true);

	UFUNCTION(BlueprintPure, Category = "Boss")
	static bool IsImmuneToBossHit(const AActor* Target, const FGameplayTagContainer& ImmunityTags);

	UFUNCTION(BlueprintCallable, Category = "Boss")
	static bool ApplyBossDamage(AActor* Instigator, AActor* DamageCauser, AActor* Target, const FBossDamageParams& Params, float DamageMultiplier, const FHitResult& Hit);
};
