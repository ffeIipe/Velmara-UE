// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "Interfaces/Weapon/MeleeWeapon.h"
#include "Interfaces/Weapon/ResetMelee.h"
#include "Items/Item.h"
#include "Sword.generated.h"

class USwordDataAsset;
class UCommand;
class UMeleeDamage;
class UBoxComponent;

UCLASS()
class TESISUE_API ASword : public AWeapon, public IMeleeWeapon, public IResetMelee
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
    TSubclassOf<UMeleeDamage> DamageTypeClass;

    virtual void Unequip() override;
    
    virtual UPrimitiveComponent* GetCollisionComponent() override;
    
    virtual void ResetMelee() override;

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

    virtual void UsePrimaryAttack(bool bIsInAir) override;
    virtual void UseSecondaryAttack(bool bIsInAir) override;
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
    
    bool PerformBaseAttack(UCommand* CommToPlay) const;
    
    virtual void ClearIgnoreActors() override { IgnoreActors.Empty(); }
    virtual void SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled) override;

    UPROPERTY()
    UCommand* LightCommandInstance;
    UPROPERTY()
    UCommand* JumpCommandInstance;
    UPROPERTY()
    UCommand* HeavyCommandInstance;
    UPROPERTY()
    UCommand* HeavyJumpCommandInstance;
};