// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DataAssets/Items/Weapons/SwordData.h"
#include "Features/InventorySystem/Items/Weapon.h"
#include "Sword.generated.h"

class UComboChain;
class USwordData;
class UCommand;
class UBoxComponent;
class UComboNode;
class UWeaponUpgradeComponent;

UCLASS()
class TESISUE_API ASword : public AWeapon
{
    GENERATED_BODY()

public:
    ASword();
    
    virtual void BeginPlay() override;
    
    // En runtime, si el dueño tiene UWeaponUpgradeComponent, apunta a una copia sin las ramas de combo bloqueadas.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
    USwordData* SwordData;

    // Data Asset original (sin filtrar por los combos desbloqueados).
    UFUNCTION(BlueprintPure, Category = "Combat")
    USwordData* GetBaseSwordData() const { return BaseSwordData ? BaseSwordData.Get() : SwordData; }

    // Regenera SwordData a partir del original quitando las ramas que el dueño todavía no desbloqueó.
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void RefreshComboUnlocks();

    UPROPERTY()
    TArray<AActor*> IgnoreActors;

    virtual void Equip() override;

    virtual void Holster() override;
    
    virtual void AttachMeshToSocket(USceneComponent* InParent, FName InSocketName = NAME_None) override;
    
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

    UFUNCTION()
    void HandleCombosChanged();

    UComboNode* PruneLockedBranches(UComboNode* Node, const UWeaponUpgradeComponent& Upgrades, const USwordData& FilteredData, TSet<UComboNode*>& Visited) const;

    UPROPERTY(Transient)
    TObjectPtr<USwordData> BaseSwordData;

    UPROPERTY(Transient)
    TWeakObjectPtr<UWeaponUpgradeComponent> BoundUpgradeComponent;
};