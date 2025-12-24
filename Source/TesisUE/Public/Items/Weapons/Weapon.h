#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "DamageTypes/BaseDamageType.h"
#include "Interfaces/Weapon/WeaponInterface.h"
#include "Items/Item.h"
#include "Weapon.generated.h"


class UWeaponData;

UCLASS(Abstract)
class TESISUE_API AWeapon : public AItem, public IWeaponInterface
{
	GENERATED_BODY()

public:
	
	virtual void Pick(AActor* NewOwner) override;
	virtual void Unequip() {}

	virtual void EnableVisuals() override { Super::EnableVisuals(); }
	virtual void DisableVisuals() override { Super::DisableVisuals(); }

	virtual void SetDamageType_Implementation(TSubclassOf<UBaseDamageType> DamageType) override;
	virtual void SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled) override {}
	virtual void ClearIgnoreActors() override {}

protected:
	UPROPERTY(EditAnywhere)
	UWeaponData* WeaponData;
	
	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> GrantedAbilityHandles;
	
	UPROPERTY(BlueprintReadWrite)
	TSubclassOf<UBaseDamageType> DamageTypeClass = UBaseDamageType::StaticClass();
	
	UPROPERTY(EditDefaultsOnly, Category= "GAS")
	TSubclassOf<UGameplayEffect> DamageEffectClass;
	
	UFUNCTION(BlueprintCallable)
	virtual void AttachMeshToSocket(USceneComponent* InParent) {}

    void OnHit(AActor* Actor, float Damage);
	
};