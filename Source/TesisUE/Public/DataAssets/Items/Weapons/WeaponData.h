#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "Features/WeaponUpgradeSystem/Data/WeaponUpgradeTypes.h"
#include "WeaponData.generated.h"

class UGameplayAbility;
class UTexture2D;

UCLASS()
class TESISUE_API UWeaponData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TArray<TSubclassOf<UGameplayAbility>> AbilitiesToGrant;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	FGameplayTag WeaponTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	FGameplayTag HolsterTag;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	float BaseDamage = -10.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	float CriticalChance = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	float CriticalDamageMultiplier = 1.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mesh")
	FName CustomInSocketName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mesh")
	TSubclassOf<UAnimInstance> AnimLayer = nullptr;

	/* MEJORAS (Altar de Sangre) */

	// Nombre del arma en el menú de mejoras.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrades")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrades")
	TObjectPtr<UTexture2D> Icon = nullptr;

	// Combos que se desbloquean con Sangre. Cada UnlockTag debe estar como RequiredUnlockTag en el nodo que abre la rama.
	// Si se deja vacío, se generan entradas a partir de los RequiredUnlockTag del árbol de combos.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrades", meta = (TitleProperty = "DisplayName"))
	TArray<FWeaponComboUpgrade> ComboUpgrades;
};