// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "Interfaces/Weapon/RangedWeapon.h"
#include "Pistol.generated.h"

class UNiagaraSystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFire);

UCLASS()
class TESISUE_API APistol : public AWeapon, public IRangedWeapon
{
	GENERATED_BODY()
	
protected:
	APistol();
	virtual void BeginPlay() override;

	virtual void UsePrimaryAttack() override;
	virtual void UseSecondaryAttack() override;

	virtual void PrimaryShoot() override;

	virtual void SecondaryShoot() override;

	FOnFire OnFire;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties | VFX")
	UNiagaraSystem* MuzzleFlash;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties | VFX")
	UNiagaraSystem* SparksEffect;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties | SFX");
	USoundBase* ShootSound;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties | SFX");
	USoundBase* OutOfBlood;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties | Montages");
	UAnimMontage* PrimaryFireMontage;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties | Montages");
	UAnimMontage* SecondaryFireMontage;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties | Montages");
	UAnimMontage* ReloadMontage;
	
private:
	//--- Basic Functions ---
	void Fire();

	void EnableFire();

	void Reload();
	
	void FinishReload();

	void SetTimer(FTimerHandle TimerHandle, float Time, void (APistol::*InTimerMethod)());

	UFUNCTION()
	void PlayEffects();

	// --- Stats ---
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	int32 MaxAmmo;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	int32 CurrentAmmo;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	bool bIsFireEnabled = false;
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	float FireEnableTime;
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	float ReloadTime;
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	bool bIsReloading = false;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	float EnergyToDecrease = 10.f;
	
	FTimerHandle TimerHandle_Reload;
	FTimerHandle TimerHandle_BetweenShots;
};
