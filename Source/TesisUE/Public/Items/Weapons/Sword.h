// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "DataAssets/Items/Weapons/SwordData.h"
#include "Items/Item.h"
#include "Sword.generated.h"

class UComboChain;
class USwordData;
class UCommand;
class UBoxComponent;

UCLASS()
class TESISUE_API ASword : public AWeapon
{
    GENERATED_BODY()

public:
    ASword();
    
    virtual void BeginPlay() override;
    
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWallHitSignature, const FHitResult&, HitResult);

    UPROPERTY(BlueprintAssignable, Category = "Collision")
    FOnWallHitSignature OnWallHit;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
    USwordData* SwordData;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
    UComboChain* ComboData;

    UPROPERTY()
    TArray<AActor*> IgnoreActors;

    UBoxComponent* GetWeaponDamageBox() const { return WeaponDamageBox; }
    
    virtual void Pick(AActor* NewOwner) override;
    
    virtual void Unequip() override;
    
    virtual UPrimitiveComponent* GetCollisionComponent() override;

    virtual void AttachMeshToSocket(USceneComponent* InParent) override;
    
protected:
    UFUNCTION(BlueprintCallable)
    void OnBoxOverlap(
       UPrimitiveComponent* OverlappedComponent,
       AActor* OtherActor,
       UPrimitiveComponent* OtherComp,
       int32 OtherBodyIndex,
       bool bFromSweep,
       const FHitResult& SweepResult);

private:
    UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
    UBoxComponent* WeaponDamageBox;

    UPROPERTY(VisibleAnywhere)
    USceneComponent* BoxTraceStart;

    UPROPERTY(VisibleAnywhere)
    USceneComponent* BoxTraceEnd;

    virtual void ClearIgnoreActors() override;
    virtual void SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled) override;
};