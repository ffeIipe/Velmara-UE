// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PistolDataAsset.generated.h"

/**
 * 
 */

class UNiagaraSystem;
class USoundBase;
class UAnimMontage;

USTRUCT(BlueprintType)
struct FPistolEffects
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
    UNiagaraSystem* MuzzleFlash;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
    UNiagaraSystem* SparksEffect;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SFX")
    USoundBase* ShootSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SFX")
    USoundBase* OutOfBlood;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SocketName")
    FName CustomInSocketName;
};

USTRUCT(BlueprintType)
struct FPistolMontages
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montages")
    UAnimMontage* PrimaryFireMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montages")
    UAnimMontage* SecondaryFireMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montages")
    UAnimMontage* ReloadMontage;
};

USTRUCT(BlueprintType)
struct FPistolProperties
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float BaseDamage;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    int32 MaxAmmo;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float FireEnableTime;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float ReloadTime;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float EnergyToDecrease = 10.f;
};

UCLASS(BlueprintType)
class TESISUE_API UPistolDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponProperties")
    FPistolEffects Effects;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponProperties")
    FPistolMontages Montages;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponProperties")
    FPistolProperties Stats;
};