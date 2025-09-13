// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EffectsData.generated.h"

/**
 * 
 */

class UNiagaraSystem;

USTRUCT(BlueprintType)
struct FEntityEffects
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "SFX")
	USoundBase* ErrorSFX;

	UPROPERTY(EditDefaultsOnly, Category = "SFX")
	USoundBase* ReceiveDamageSFX;

	UPROPERTY(EditDefaultsOnly, Category = "SFX")
	USoundBase* HitSound;

	UPROPERTY(EditDefaultsOnly, Category = "SFX")
	USoundBase* ShieldImpactSFX;

	UPROPERTY(EditDefaultsOnly, Category = "VFX")
	UNiagaraSystem* ReceiveDamageFX;
};

UCLASS()
class TESISUE_API UEffectsData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	FEntityEffects EntityEffects;
};
