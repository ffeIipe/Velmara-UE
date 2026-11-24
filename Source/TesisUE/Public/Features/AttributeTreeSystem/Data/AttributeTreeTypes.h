#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "AttributeTreeTypes.generated.h"

class UTexture2D;

namespace AttributeTreeTags
{
	TESISUE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stat_MaxHealth);
	TESISUE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stat_Armor);
	TESISUE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stat_BaseDamage);
	TESISUE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stat_AttackSpeed);
}

UENUM(BlueprintType)
enum class EAttributeUpgradeResult : uint8
{
	Success,
	InvalidUpgrade,
	MaxLevelReached,
	NotEnoughCurrency,
	NoCurrencySystem,
	NoAbilitySystem
};

UENUM(BlueprintType)
enum class EAttributeUpgradeDisplay : uint8
{
	Flat,
	Percent
};

USTRUCT(BlueprintType)
struct FAttributeUpgradeLevel
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute Tree", meta = (ClampMin = 0))
	int32 Cost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute Tree")
	float TotalBonus = 0.f;
};

USTRUCT(BlueprintType)
struct FAttributeUpgradeDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute Tree", meta = (Categories = "Progression.Stats"))
	FGameplayTag UpgradeTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute Tree")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute Tree", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute Tree")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute Tree")
	FGameplayAttribute Attribute;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute Tree")
	FGameplayAttribute LinkedCurrentAttribute;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute Tree")
	EAttributeUpgradeDisplay DisplayFormat = EAttributeUpgradeDisplay::Flat;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute Tree", meta = (TitleProperty = "Costo: {Cost}  |  Bonus: {TotalBonus}"))
	TArray<FAttributeUpgradeLevel> Levels;

	int32 GetMaxLevel() const { return FMath::Max(Levels.Num(), 1); }

	float GetTotalBonusAtLevel(int32 Level) const;

	int32 GetCostForLevel(int32 Level) const;
};

namespace AttributeTreeUtils
{
	TESISUE_API FString ToRoman(int32 Number);

	TESISUE_API FText FormatBonus(float Value, EAttributeUpgradeDisplay Format);

	TESISUE_API FText FormatValue(float Value, EAttributeUpgradeDisplay Format);
}
