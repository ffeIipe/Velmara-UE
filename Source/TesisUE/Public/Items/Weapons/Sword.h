// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "DamageTypes/MeleeDamage.h"
#include "Interfaces/Weapon/MeleeWeapon.h"
#include "Items/Item.h"
#include "Player/CharacterWeaponStates.h"
#include "Sword.generated.h"

class USwordDataAsset;
class UCommand;
class UMeleeDamage;
class UBoxComponent;

UCLASS()
class TESISUE_API ASword : public AWeapon, public IMeleeWeapon
{
    GENERATED_BODY()

public:
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWallHitSignature, const FHitResult&, HitResult);

    UPROPERTY(BlueprintAssignable, Category = "Collision")
    FOnWallHitSignature OnWallHit;

    ASword();

    virtual void BeginPlay() override;
    
    UBoxComponent* GetWeaponDamageBox() const { return WeaponDamageBox; }

    UPROPERTY()
    TArray<AActor*> IgnoreActors;

    UPROPERTY(BlueprintReadWrite)
    TSubclassOf<UMeleeDamage> DamageTypeClass = UMeleeDamage::StaticClass();

    virtual void Unequip() override;
    
    virtual UPrimitiveComponent* GetCollisionComponent() override;

    virtual void AttachMeshToSocket(USceneComponent* InParent) override;
    void ImpactEffects(const FHitResult& Hit, bool bIsHittable) const;

protected:
    UFUNCTION(BlueprintCallable)
    void OnBoxOverlap(
       UPrimitiveComponent* OverlappedComponent,
       AActor* OtherActor,
       UPrimitiveComponent* OtherComp,
       int32 OtherBodyIndex,
       bool bFromSweep,
       const FHitResult& SweepResult);


    virtual void UseWeapon_Implementation(const EWeaponCommandType& CommandType) override;
    virtual void SaveWeaponAttack_Implementation(const EWeaponCommandType& CommandType) override;
    
    virtual void SetDamageType_Implementation(TSubclassOf<UMeleeDamage> DamageType) override;
    virtual void ResetWeapon_Implementation() override;

    UPROPERTY()
    TMap<EWeaponCommandType, TObjectPtr<UCommand>> CommandsInstances;
    
private:
    UPROPERTY(EditAnywhere)
    USwordDataAsset* SwordDataAsset;
    
    UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
    UBoxComponent* WeaponDamageBox;

    UPROPERTY(VisibleAnywhere)
    USceneComponent* BoxTraceStart;

    UPROPERTY(VisibleAnywhere)
    USceneComponent* BoxTraceEnd;

    float CalculateDamage() const;

    /*void OnWallCollision(const FHitResult& Hit);*/
    
    /*bool PerformCommand(const EWeaponCommandType& CommToPlay, bool bShouldSoftLock) const;*/
    
    virtual void ClearIgnoreActors() override;
    virtual void SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled) override;
};