// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SpectralWeaponComponent.generated.h"

class UAttributeComponent;
class APawn;
class ACharacter;
class APlayerController;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TESISUE_API USpectralWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	USpectralWeaponComponent();

    void InitializeSpectralWeaponComponent();

    bool bWasInitialized = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties | Primary Shot")
    float PrimaryDamage;
   
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties | Primary Shot")
    float PrimaryEnergyCost = 5.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties | Secondary Shot")
    float SecondaryDamage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties | Secondary Shot")
    float SpreadAngle = 3.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties | Secondary Shot")
    int32 Shells;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties | Secondary Shot")
    float SecondaryEnergyCost = 5.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties | Both")
    int32 MaxAmmo = 3;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties | Both")
    float ReloadTime = .5f;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Properties | Both")
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

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    UStaticMeshComponent* GetSpectralWeaponMeshComponent() const { return SpectralWeaponMeshComponent; }

protected:
    virtual void Fire(bool bIsPrimary);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties | Weapon FX")
    class USoundBase* FireSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties | Weapon FX")
    class UParticleSystem* MuzzleFlash;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties | Weapon FX")
    UAnimMontage* SpectralFireAnimation;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties | Weapon FX")
    UAnimMontage* SpectralReloadAnimation;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties | Weapon FX")
    UStaticMesh* SpectralWeaponMesh;

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "Weapon Properties | Weapon FX")
    class UStaticMeshComponent* SpectralWeaponMeshComponent;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties | Weapon FX")
    TSubclassOf<UCameraShakeBase> CameraShake;

    virtual void BeginPlay() override;

private:
    bool bIsReloading = false;
    
    FTimerHandle TimerHandle_Reload;
    
    FTimerHandle TimerHandle_BetweenPrimaryShots;

    bool bIsFireEnable = true;

    void EnableFire();

    void SetTimer(FTimerHandle TimerHandle, float Time, void (USpectralWeaponComponent::* InTimerMethod)());

    APawn* OwnerInstigator;

    ACharacter* OwnerCharacter;

    APlayerController* OwnerController;

    UAttributeComponent* OwnerAttributeComponent;
};
