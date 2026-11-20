#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "WeaponUpgradeTypes.generated.h"

class UComboNode;
class UTexture2D;
class UWeaponData;

UENUM(BlueprintType)
enum class EWeaponUpgradeResult : uint8
{
	Success,
	InvalidUpgrade,
	AlreadyUnlocked,
	MissingRequirements,
	NotEnoughCurrency,
	NoCurrencySystem
};

UENUM(BlueprintType)
enum class EWeaponComboState : uint8
{
	Unlocked,
	Available,
	RequirementsMissing
};

UENUM(BlueprintType)
enum class EComboRoot : uint8
{
	None,
	Ground,
	Air,
	Launch,
	Skill
};

UENUM(BlueprintType)
enum class EComboInput : uint8
{
	Tap,
	Hold
};

USTRUCT(BlueprintType)
struct FWeaponComboUpgrade
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Upgrade", meta = (Categories = "Progression.Combo"))
	FGameplayTag UnlockTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Upgrade")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Upgrade", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Upgrade")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Upgrade", meta = (ClampMin = 0))
	int32 Cost = 150;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Upgrade", meta = (Categories = "Progression.Combo"))
	FGameplayTagContainer RequiredUnlocks;
};

USTRUCT(BlueprintType)
struct FComboInputPath
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Weapon Upgrade")
	bool bFound = false;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon Upgrade")
	EComboRoot Root = EComboRoot::None;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon Upgrade")
	TArray<EComboInput> Inputs;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon Upgrade")
	int32 HitCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon Upgrade")
	FGameplayTag ParentUnlockTag;
};

namespace WeaponUpgradeUtils
{
	TESISUE_API FComboInputPath FindComboPath(const UWeaponData* Weapon, const FGameplayTag& UnlockTag);

	TESISUE_API void CollectUnlockTags(const UWeaponData* Weapon, TArray<FGameplayTag>& OutTags);

	TESISUE_API FText DescribeComboInput(const FComboInputPath& Path);

	TESISUE_API FText GetWeaponDisplayName(const UWeaponData* Weapon);
}
