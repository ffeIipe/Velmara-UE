// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/Weapon/WeaponInterface.h"
#include "Items/Item.h"
#include "Weapon.generated.h"

class IAnimatorProvider;
class ICharacterStateProvider;
class IControllerProvider;
/**
 * 
 */
UCLASS()
class TESISUE_API AWeapon : public AItem, public IWeaponInterface
{
	GENERATED_BODY()

public:
	virtual void UsePrimaryAttack() override {}
	
	virtual void UseSecondaryAttack() override {}
	
	virtual void Pick(AActor* NewOwner) override;

	virtual void Unequip() {}

	virtual void EnableVisuals(bool bEnable) override;

	virtual void SetWeaponCollisionEnabled_Implementation(ECollisionEnabled::Type CollisionEnabled) override {}

	virtual void ClearIgnoreActors() override {}

	virtual FName& GetUniqueSaveID() override { return UniqueSaveID; }

	void EnableWeaponState(bool bEnable) const;
	
protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	FName InSocketName;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	float Damage;
	
	UFUNCTION(BlueprintCallable)
	virtual void AttachMeshToSocket(USceneComponent* InParent);

	UPROPERTY()
	TScriptInterface<IControllerProvider> ControllerProvider;
	TScriptInterface<ICharacterStateProvider> CharacterStateProvider;
	TScriptInterface<IAnimatorProvider> AnimatorProvider;
};