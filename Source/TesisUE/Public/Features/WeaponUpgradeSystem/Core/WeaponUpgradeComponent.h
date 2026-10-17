#pragma once

#include "CoreMinimal.h"
#include "Features/UpgradeSystem/Core/UpgradeComponentBase.h"
#include "Features/WeaponUpgradeSystem/Data/WeaponUpgradeTypes.h"
#include "WeaponUpgradeComponent.generated.h"

class UComboNode;
class UWeaponData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponComboUnlockedSignature, UWeaponData*, Weapon, FGameplayTag, UnlockTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponCombosChangedSignature);

/*
 * Mejoras de arma a lo God of War: se gasta Sangre para desbloquear ramas del árbol de combos (UComboNode) de cada arma.
 *
 * - Qué se puede desbloquear: UWeaponData::ComboUpgrades (o, si está vacío, los RequiredUnlockTag usados en el árbol).
 * - Qué queda bloqueado en combate: cada ASword del dueño usa una copia de su SwordData sin las ramas bloqueadas,
 *   así que GA_ComboBase_Generic no necesita cambios.
 *
 * Agregalo al Blueprint del personaje jugable. El menú lo abre el Altar de Sangre (ABloodAltar) o ToggleMenu().
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TESISUE_API UWeaponUpgradeComponent : public UUpgradeComponentBase
{
	GENERATED_BODY()

public:
	UWeaponUpgradeComponent();

	/* MEJORAS */

	UFUNCTION(BlueprintCallable, Category = "Weapon Upgrades")
	EWeaponUpgradeResult TryUnlockCombo(UWeaponData* Weapon, FGameplayTag UnlockTag);

	// Desbloquea o vuelve a bloquear un combo sin cobrar (debug / testing).
	UFUNCTION(BlueprintCallable, Category = "Weapon Upgrades")
	void SetComboUnlocked(FGameplayTag UnlockTag, bool bUnlocked);

	/* CONSULTAS */

	UFUNCTION(BlueprintPure, Category = "Weapon Upgrades | Queries")
	bool IsComboUnlocked(FGameplayTag UnlockTag) const;

	// true si el nodo no requiere desbloqueo o si su RequiredUnlockTag ya fue desbloqueado.
	UFUNCTION(BlueprintPure, Category = "Weapon Upgrades | Queries")
	bool IsComboNodeUnlocked(const UComboNode* Node) const;

	// Armas que muestra el menú: UpgradeableWeapons + espadas del inventario (Data Assets originales).
	UFUNCTION(BlueprintPure, Category = "Weapon Upgrades | Queries")
	TArray<UWeaponData*> GetUpgradeableWeapons() const;

	UFUNCTION(BlueprintPure, Category = "Weapon Upgrades | Queries")
	TArray<FWeaponComboUpgrade> GetComboUpgrades(const UWeaponData* Weapon) const;

	UFUNCTION(BlueprintPure, Category = "Weapon Upgrades | Queries")
	bool GetComboUpgrade(const UWeaponData* Weapon, FGameplayTag UnlockTag, FWeaponComboUpgrade& OutUpgrade) const;

	UFUNCTION(BlueprintPure, Category = "Weapon Upgrades | Queries")
	EWeaponComboState GetComboState(const UWeaponData* Weapon, FGameplayTag UnlockTag) const;

	// Desbloqueos previos que faltan: los de RequiredUnlocks y la rama bloqueada de la que cuelga en el árbol.
	UFUNCTION(BlueprintPure, Category = "Weapon Upgrades | Queries")
	FGameplayTagContainer GetMissingRequirements(const UWeaponData* Weapon, FGameplayTag UnlockTag) const;

	UFUNCTION(BlueprintPure, Category = "Weapon Upgrades | Queries")
	bool CanAffordCombo(const UWeaponData* Weapon, FGameplayTag UnlockTag) const;

	UFUNCTION(BlueprintPure, Category = "Weapon Upgrades | Queries")
	int32 GetUnlockedComboCount(const UWeaponData* Weapon) const;

	/* GUARDADO */

	UFUNCTION(BlueprintPure, Category = "Weapon Upgrades | Save")
	const FGameplayTagContainer& GetUnlockedCombos() const { return UnlockedCombos; }

	// Reemplaza los combos desbloqueados por los de una partida guardada.
	UFUNCTION(BlueprintCallable, Category = "Weapon Upgrades | Save")
	void RestoreUnlockedCombos(const FGameplayTagContainer& SavedCombos);

	/* EVENTOS */

	UPROPERTY(BlueprintAssignable, Category = "Weapon Upgrades | Events")
	FOnWeaponComboUnlockedSignature OnComboUnlocked;

	// Cualquier cambio en los combos desbloqueados (compra, carga, debug). Las espadas lo usan para refrescar su árbol.
	UPROPERTY(BlueprintAssignable, Category = "Weapon Upgrades | Events")
	FOnWeaponCombosChangedSignature OnCombosChanged;

protected:
	/* CONFIGURACIÓN */

	// Armas que siempre aparecen en el menú, aunque el personaje no las tenga en el inventario.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Upgrades | Config")
	TArray<TObjectPtr<UWeaponData>> UpgradeableWeapons;

	// Agrega al menú las espadas que el personaje tiene en el inventario.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Upgrades | Config")
	bool bIncludeInventoryWeapons = true;

	// Costo de las entradas generadas automáticamente (armas sin ComboUpgrades configurados).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Upgrades | Config", meta = (ClampMin = 0))
	int32 DefaultComboCost = 150;

	/* ESTADO (guardable) */

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Upgrades | State", SaveGame)
	FGameplayTagContainer UnlockedCombos;
};
