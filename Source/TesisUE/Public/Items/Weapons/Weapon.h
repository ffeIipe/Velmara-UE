// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/MementoComponent.h"
#include "Interfaces/Weapon/WeaponInterface.h"
#include "Items/Item.h"
#include "Weapon.generated.h"

class IAnimatorProvider;
class ICharacterStateProvider;
class IControllerProvider;
/**
 * 
 */
UCLASS(Abstract)
class TESISUE_API AWeapon : public AItem, public IWeaponInterface
{
	GENERATED_BODY()

public:
	virtual void UsePrimaryAttack_Implementation() override {}
	
	virtual void UseSecondaryAttack_Implementation() override {}

	virtual void UseAbilityAttack_Implementation() override {}
	
	virtual void SetDamageType_Implementation(TSubclassOf<UMeleeDamage> DamageType) override {}

	virtual void ResetWeapon_Implementation() override {}
	
	virtual void Pick(AActor* NewOwner) override;

	virtual void Unequip() {}

	virtual void EnableVisuals(const bool bEnable) override { Super::EnableVisuals(bEnable); }

	virtual void SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled) override {}

	virtual void ClearIgnoreActors() override {}

	virtual FName& GetUniqueSaveID() override { return UniqueSaveID; }

	void SetUniqueSaveID(const FName Name) { UniqueSaveID = Name; }
	
	void EnableWeaponState(bool bEnable) const;

protected:
	virtual void BeginPlay() override;
	
	UFUNCTION(BlueprintCallable)
	virtual void AttachMeshToSocket(USceneComponent* InParent) {}

	UPROPERTY()
	TScriptInterface<IControllerProvider> ControllerProvider;
	TScriptInterface<ICharacterStateProvider> CharacterStateProvider;
	TScriptInterface<IAnimatorProvider> AnimatorProvider;
};