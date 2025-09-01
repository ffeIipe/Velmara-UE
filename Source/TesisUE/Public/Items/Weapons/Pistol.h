// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "Pistol.generated.h"

class ICharacterStateProvider;
class IControllerProvider;
class UNiagaraSystem;

UCLASS()
class TESISUE_API APistol : public AWeapon
{
	GENERATED_BODY()
	
protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties | VFX")
	UNiagaraSystem* MuzzleFlash;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties | VFX")
	UNiagaraSystem* SparksEffect;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties | SFX");
	USoundBase* OutOfBlood;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties | Montages");
	UAnimMontage* ReloadMontage;

	UPROPERTY()
	AController* OwnerController;
	TScriptInterface<IControllerProvider> ControllerProvider;
	
	TScriptInterface<ICharacterStateProvider> CharacterStateProvider;

	void AttachMeshToSocket(USceneComponent* InParent, FName InSocketName) const;
	virtual void Equip(USceneComponent* InParent, FName InSocketName, AActor* NewOwner, APawn* NewInstigator) override;

private:
	//--- Basic Functions ---
	void Fire();

	void EnableFire();

	void Reload();
	
	void FinishReload();
	void SetTimer(FTimerHandle TimerHandle, float Time, void (APistol::*InTimerMethod)());

	// --- Stats ---
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	float Damage;

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
	
	FTimerHandle TimerHandle_Reload;
	FTimerHandle TimerHandle_BetweenShots;
};
