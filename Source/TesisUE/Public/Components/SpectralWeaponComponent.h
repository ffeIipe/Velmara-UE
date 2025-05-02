// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SpectralWeaponComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TESISUE_API USpectralWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	USpectralWeaponComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties")
    int32 MaxAmmo = 3;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties")
    float ReloadTime = .5f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Properties")
    int32 CurrentAmmo;

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    virtual void PrimaryFire();

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    virtual void SecondaryFire();

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void Reload();

    void FinishReload();

    void AttachToOwner(USceneComponent* InParent, FName SocketName);

    void EnableSpectralWeapon(bool Enable);

protected:
    virtual void Fire(bool bIsPrimary);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties")
    class USoundCue* FireSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties")
    class UParticleSystem* MuzzleFlash;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties")
    class UStaticMesh* SpectralWeaponMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties")
    class UStaticMeshComponent* SpectralWeaponMeshComponent;

    virtual void BeginPlay() override;

private:
    bool bIsReloading = false;
    
    FTimerHandle TimerHandle_Reload;
    
    FTimerHandle TimerHandle_BetweenPrimaryShots;

    bool bIsFireEnable = true;

    void EnableFire();

    void SetTimer(FTimerHandle TimerHandle, float Time, void (USpectralWeaponComponent::* InTimerMethod)());
};
