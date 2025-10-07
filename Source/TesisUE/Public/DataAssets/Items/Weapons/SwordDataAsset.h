// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SwordDataAsset.generated.h"

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
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment Socket")
	FName CustomInSocketName;
};

USTRUCT(BlueprintType)
struct FSwordProperties
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Stats")
	float BaseDamage = 10.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float CriticalChance = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float CriticalDamageMultiplier = 2.0f;
};

UCLASS(BlueprintType)
class TESISUE_API USwordDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Attacks")
	TSubclassOf<UCommand> LightComboCommandClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Attacks")
	TSubclassOf<UCommand> LaunchCommandClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Attacks")
	TSubclassOf<UCommand> JumpComboCommandClass;
    
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Attacks")
	TSubclassOf<UCommand> HeavyComboCommandClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Attacks")
	TSubclassOf<UCommand> HeavyJumpComboCommandClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Attacks")
	TSubclassOf<UCommand> AbilityCommandClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties")
	FSwordProperties Stats;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	FSwordEffects Effects;
};
