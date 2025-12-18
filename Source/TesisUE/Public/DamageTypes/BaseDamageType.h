#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "BaseDamageType.generated.h"

class UNiagaraSystem;

UCLASS()
class TESISUE_API UBaseDamageType : public UDamageType
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TObjectPtr<UNiagaraSystem> HitFX;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TObjectPtr<USoundBase> HitSound;
};
