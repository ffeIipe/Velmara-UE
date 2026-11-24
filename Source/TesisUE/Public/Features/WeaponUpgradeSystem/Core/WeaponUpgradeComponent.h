#pragma once

#include "CoreMinimal.h"
#include "Features/UpgradeSystem/Core/UpgradeComponentBase.h"
#include "Features/WeaponUpgradeSystem/Data/WeaponUpgradeTypes.h"
#include "WeaponUpgradeComponent.generated.h"

class UComboNode;
class UWeaponData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponComboUnlockedSignature, UWeaponData*, Weapon, FGameplayTag, UnlockTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponCombosChangedSignature);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TESISUE_API UWeaponUpgradeComponent : public UUpgradeComponentBase
{
	GENERATED_BODY()

public:
	UWeaponUpgradeComponent();

	UFUNCTION(BlueprintCallable, Category = "Weapon Upgrades")
	EWeaponUpgradeResult TryUnlockCombo(UWeaponData* Weapon, FGameplayTag UnlockTag);

	UFUNCTION(BlueprintCallable, Category = "Weapon Upgrades")
	void SetComboUnlocked(FGameplayTag UnlockTag, bool bUnlocked);

	UFUNCTION(BlueprintPure, Category = "Weapon Upgrades | Queries")
	bool IsComboUnlocked(FGameplayTag UnlockTag) const;

	UFUNCTION(BlueprintPure, Category = "Weapon Upgrades | Queries")
	bool IsComboNodeUnlocked(const UComboNode* Node) const;

	UFUNCTION(BlueprintPure, Category = "Weapon Upgrades | Queries")
	TArray<UWeaponData*> GetUpgradeableWeapons() const;

	UFUNCTION(BlueprintPure, Category = "Weapon Upgrades | Queries")
	TArray<FWeaponComboUpgrade> GetComboUpgrades(const UWeaponData* Weapon) const;

	UFUNCTION(BlueprintPure, Category = "Weapon Upgrades | Queries")
	bool GetComboUpgrade(const UWeaponData* Weapon, FGameplayTag UnlockTag, FWeaponComboUpgrade& OutUpgrade) const;

	UFUNCTION(BlueprintPure, Category = "Weapon Upgrades | Queries")
	EWeaponComboState GetComboState(const UWeaponData* Weapon, FGameplayTag UnlockTag) const;

	UFUNCTION(BlueprintPure, Category = "Weapon Upgrades | Queries")
	FGameplayTagContainer GetMissingRequirements(const UWeaponData* Weapon, FGameplayTag UnlockTag) const;

	UFUNCTION(BlueprintPure, Category = "Weapon Upgrades | Queries")
	bool CanAffordCombo(const UWeaponData* Weapon, FGameplayTag UnlockTag) const;

	UFUNCTION(BlueprintPure, Category = "Weapon Upgrades | Queries")
	int32 GetUnlockedComboCount(const UWeaponData* Weapon) const;

	UFUNCTION(BlueprintPure, Category = "Weapon Upgrades | Save")
	const FGameplayTagContainer& GetUnlockedCombos() const { return UnlockedCombos; }

	UFUNCTION(BlueprintCallable, Category = "Weapon Upgrades | Save")
	void RestoreUnlockedCombos(const FGameplayTagContainer& SavedCombos);

	UPROPERTY(BlueprintAssignable, Category = "Weapon Upgrades | Events")
	FOnWeaponComboUnlockedSignature OnComboUnlocked;

	UPROPERTY(BlueprintAssignable, Category = "Weapon Upgrades | Events")
	FOnWeaponCombosChangedSignature OnCombosChanged;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Upgrades | Config")
	TArray<TObjectPtr<UWeaponData>> UpgradeableWeapons;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Upgrades | Config")
	bool bIncludeInventoryWeapons = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Upgrades | Config", meta = (ClampMin = 0))
	int32 DefaultComboCost = 150;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Upgrades | State", SaveGame)
	FGameplayTagContainer UnlockedCombos;
};
