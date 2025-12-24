// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WeaponData.h"
#include "Engine/DataAsset.h"
#include "SwordData.generated.h"

enum class EWeaponCommandType : uint8;
enum class EHitStopPreset : uint8;
enum class ECameraShakePreset : uint8;
class UNiagaraSystem;
class USoundBase;
class UCommand;

USTRUCT(BlueprintType)
struct FSwordEffects
{
	GENERATED_BODY()
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX | SFX")
	USoundBase* ShieldImpact;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX | VFX")
	UNiagaraSystem* HitEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX | VFX")
	UNiagaraSystem* SparksEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
	EHitStopPreset HitStopPreset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
	ECameraShakePreset CameraShakePreset;
};

UCLASS(BlueprintType)
class TESISUE_API USwordData : public UWeaponData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	FSwordEffects Effects;
};
