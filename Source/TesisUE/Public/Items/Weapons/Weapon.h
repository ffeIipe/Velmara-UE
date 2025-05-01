// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/Item.h"
#include "Weapon.generated.h"

UCLASS()
class TESISUE_API AWeapon : public AItem
{
	GENERATED_BODY()
	
public:	
    AWeapon();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties")
    int32 MaxAmmo = 3;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Properties")
    int32 CurrentAmmo;

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    virtual void PrimaryFire();

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    virtual void SecondaryFire();

    virtual void Equip(USceneComponent* InParent, FName InSocketName, AActor* NewOwner, APawn* NewInstigator) override;
    
    virtual void Unequip() override;

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void Reload();

protected:
    virtual void Fire(bool bIsPrimary);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties")
    class USoundCue* FireSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties")
    class UParticleSystem* MuzzleFlash;

    virtual void BeginPlay() override;
};
