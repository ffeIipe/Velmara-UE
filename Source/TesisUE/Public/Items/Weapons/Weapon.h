#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "Interfaces/Weapon/WeaponInterface.h"
#include "Items/Item.h"
#include "Player/CharacterWeaponStates.h"
#include "Weapon.generated.h"

class IAnimatorProvider;
class ICharacterStateProvider;
class IControllerProvider;

UCLASS(Abstract)
class TESISUE_API AWeapon : public AItem, public IWeaponInterface
{
	GENERATED_BODY()

public:
	virtual void UseWeapon_Implementation(const EWeaponCommandType& CommandType) override {}
	virtual void SaveWeaponAttack_Implementation(const EWeaponCommandType& CommandType) override {}
	virtual EWeaponCommandType GetCommandTypeSaved_Implementation() override { return CommandTypeSaved; }
	
	virtual void SetDamageType_Implementation(TSubclassOf<UMeleeDamage> DamageType) override {}
	virtual void ResetWeapon_Implementation() override {}
	
	virtual void Pick(AActor* NewOwner) override;
	virtual void Unequip() {}

	virtual void EnableVisuals() override { Super::EnableVisuals(); }
	virtual void DisableVisuals() override { Super::DisableVisuals(); }

	virtual void SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled) override {}
	virtual void ClearIgnoreActors() override {}

	void EnableWeaponState(bool bEnable) const; //TODO: change this to enable and disable.
	
protected:
	UPROPERTY(EditDefaultsOnly, Category= "GAS")
	TSubclassOf<UGameplayEffect> DamageEffectClass;
	
	UFUNCTION(BlueprintCallable)
	virtual void AttachMeshToSocket(USceneComponent* InParent) {}

	UPROPERTY()
	TScriptInterface<IControllerProvider> ControllerProvider;
	TScriptInterface<ICharacterStateProvider> CharacterStateProvider;
	TScriptInterface<IAnimatorProvider> AnimatorProvider;
};