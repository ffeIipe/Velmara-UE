#include "Features/WeaponUpgradeSystem/Core/WeaponUpgradeComponent.h"

#include "DataAssets/Items/Weapons/WeaponData.h"
#include "Features/InventorySystem/Core/InventoryComponent.h"
#include "Features/WeaponUpgradeSystem/UI/WeaponUpgradeMenuWidget.h"
#include "GAS/ComboNode.h"
#include "Items/Weapons/Sword.h"

UWeaponUpgradeComponent::UWeaponUpgradeComponent()
{
	MenuWidgetClass = UWeaponUpgradeMenuWidget::StaticClass();
}

/* MEJORAS */

EWeaponUpgradeResult UWeaponUpgradeComponent::TryUnlockCombo(UWeaponData* Weapon, const FGameplayTag UnlockTag)
{
	FWeaponComboUpgrade Upgrade;
	if (!UnlockTag.IsValid() || !GetComboUpgrade(Weapon, UnlockTag, Upgrade)) return EWeaponUpgradeResult::InvalidUpgrade;
	if (IsComboUnlocked(UnlockTag)) return EWeaponUpgradeResult::AlreadyUnlocked;
	if (!GetMissingRequirements(Weapon, UnlockTag).IsEmpty()) return EWeaponUpgradeResult::MissingRequirements;

	switch (TryPay(Upgrade.Cost))
	{
	case EUpgradePaymentResult::NotEnoughCurrency:
		return EWeaponUpgradeResult::NotEnoughCurrency;
	case EUpgradePaymentResult::NoCurrencySystem:
		return EWeaponUpgradeResult::NoCurrencySystem;
	default:
		break;
	}

	UnlockedCombos.AddTag(UnlockTag);

	OnComboUnlocked.Broadcast(Weapon, UnlockTag);
	OnCombosChanged.Broadcast();
	NotifyCurrencyChanged();

	return EWeaponUpgradeResult::Success;
}

void UWeaponUpgradeComponent::SetComboUnlocked(const FGameplayTag UnlockTag, const bool bUnlocked)
{
	if (!UnlockTag.IsValid()) return;

	if (bUnlocked)
	{
		UnlockedCombos.AddTag(UnlockTag);
	}
	else
	{
		UnlockedCombos.RemoveTag(UnlockTag);
	}

	OnCombosChanged.Broadcast();
}

void UWeaponUpgradeComponent::RestoreUnlockedCombos(const FGameplayTagContainer& SavedCombos)
{
	UnlockedCombos = SavedCombos;
	OnCombosChanged.Broadcast();
}

/* CONSULTAS */

bool UWeaponUpgradeComponent::IsComboUnlocked(const FGameplayTag UnlockTag) const
{
	return UnlockTag.IsValid() && UnlockedCombos.HasTagExact(UnlockTag);
}

bool UWeaponUpgradeComponent::IsComboNodeUnlocked(const UComboNode* Node) const
{
	if (!Node) return false;

	return !Node->RequiredUnlockTag.IsValid() || IsComboUnlocked(Node->RequiredUnlockTag);
}

TArray<UWeaponData*> UWeaponUpgradeComponent::GetUpgradeableWeapons() const
{
	TArray<UWeaponData*> Weapons;

	for (UWeaponData* Weapon : UpgradeableWeapons)
	{
		if (Weapon)
		{
			Weapons.AddUnique(Weapon);
		}
	}

	const UInventoryComponent* Inventory = GetOwner() ? GetOwner()->FindComponentByClass<UInventoryComponent>() : nullptr;
	if (bIncludeInventoryWeapons && Inventory)
	{
		auto AddSword = [&Weapons](const UObject* Item)
		{
			if (const ASword* Sword = Cast<ASword>(Item))
			{
				if (USwordData* BaseData = Sword->GetBaseSwordData())
				{
					Weapons.AddUnique(BaseData);
				}
			}
		};

		AddSword(Inventory->GetCurrentWeapon().GetObject());

		for (const TScriptInterface<IPickable>& Item : Inventory->GetInventoryItems())
		{
			AddSword(Item.GetObject());
		}
	}

	return Weapons;
}

TArray<FWeaponComboUpgrade> UWeaponUpgradeComponent::GetComboUpgrades(const UWeaponData* Weapon) const
{
	if (!Weapon) return {};
	if (!Weapon->ComboUpgrades.IsEmpty()) return Weapon->ComboUpgrades;

	// Sin configuración: una entrada por cada RequiredUnlockTag usado en el árbol de combos.
	TArray<FGameplayTag> UnlockTags;
	WeaponUpgradeUtils::CollectUnlockTags(Weapon, UnlockTags);

	TArray<FWeaponComboUpgrade> GeneratedUpgrades;
	for (const FGameplayTag& UnlockTag : UnlockTags)
	{
		FString LeafName;
		UnlockTag.ToString().Split(TEXT("."), nullptr, &LeafName, ESearchCase::IgnoreCase, ESearchDir::FromEnd);

		FWeaponComboUpgrade& Upgrade = GeneratedUpgrades.AddDefaulted_GetRef();
		Upgrade.UnlockTag = UnlockTag;
		Upgrade.DisplayName = FText::FromString(LeafName.IsEmpty() ? UnlockTag.ToString() : LeafName);
		Upgrade.Cost = DefaultComboCost;
	}

	return GeneratedUpgrades;
}

bool UWeaponUpgradeComponent::GetComboUpgrade(const UWeaponData* Weapon, const FGameplayTag UnlockTag, FWeaponComboUpgrade& OutUpgrade) const
{
	for (const FWeaponComboUpgrade& Upgrade : GetComboUpgrades(Weapon))
	{
		if (Upgrade.UnlockTag == UnlockTag)
		{
			OutUpgrade = Upgrade;
			return true;
		}
	}
	return false;
}

EWeaponComboState UWeaponUpgradeComponent::GetComboState(const UWeaponData* Weapon, const FGameplayTag UnlockTag) const
{
	if (IsComboUnlocked(UnlockTag)) return EWeaponComboState::Unlocked;

	return GetMissingRequirements(Weapon, UnlockTag).IsEmpty()
		? EWeaponComboState::Available
		: EWeaponComboState::RequirementsMissing;
}

FGameplayTagContainer UWeaponUpgradeComponent::GetMissingRequirements(const UWeaponData* Weapon, const FGameplayTag UnlockTag) const
{
	FGameplayTagContainer Missing;

	FWeaponComboUpgrade Upgrade;
	if (!GetComboUpgrade(Weapon, UnlockTag, Upgrade)) return Missing;

	for (const FGameplayTag& RequiredTag : Upgrade.RequiredUnlocks)
	{
		if (!IsComboUnlocked(RequiredTag))
		{
			Missing.AddTag(RequiredTag);
		}
	}

	// Si la rama cuelga de otra rama bloqueada, no se puede ejecutar hasta desbloquear esa.
	const FComboInputPath Path = WeaponUpgradeUtils::FindComboPath(Weapon, UnlockTag);
	if (Path.ParentUnlockTag.IsValid() && !IsComboUnlocked(Path.ParentUnlockTag))
	{
		Missing.AddTag(Path.ParentUnlockTag);
	}

	return Missing;
}

bool UWeaponUpgradeComponent::CanAffordCombo(const UWeaponData* Weapon, const FGameplayTag UnlockTag) const
{
	FWeaponComboUpgrade Upgrade;
	return GetComboUpgrade(Weapon, UnlockTag, Upgrade) && CanPay(Upgrade.Cost);
}

int32 UWeaponUpgradeComponent::GetUnlockedComboCount(const UWeaponData* Weapon) const
{
	int32 Count = 0;
	for (const FWeaponComboUpgrade& Upgrade : GetComboUpgrades(Weapon))
	{
		Count += IsComboUnlocked(Upgrade.UnlockTag) ? 1 : 0;
	}
	return Count;
}
