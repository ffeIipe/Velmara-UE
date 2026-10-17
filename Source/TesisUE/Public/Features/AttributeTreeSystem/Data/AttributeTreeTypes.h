#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "AttributeTreeTypes.generated.h"

class UTexture2D;

/*
 * Tags que identifican cada rama del árbol. Cuelgan de "Progression.Stats" para coincidir con el BaseStatTag
 * del ProgressionComponent.
 */
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
	// Se muestra tal cual: +20
	Flat,
	// Se muestra como porcentaje: 0.1 -> +10%
	Percent
};

USTRUCT(BlueprintType)
struct FAttributeUpgradeLevel
{
	GENERATED_BODY()

	// Costo en moneda para llegar a este nivel. En el Nivel I se ignora (es el nivel inicial).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute Tree", meta = (ClampMin = 0))
	int32 Cost = 0;

	// Bonus TOTAL (acumulado, no incremental) que se suma al atributo estando en este nivel.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute Tree")
	float TotalBonus = 0.f;
};

USTRUCT(BlueprintType)
struct FAttributeUpgradeDefinition
{
	GENERATED_BODY()

	// Identificador de la rama. Es la clave con la que se guardan los niveles.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute Tree", meta = (Categories = "Progression.Stats"))
	FGameplayTag UpgradeTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute Tree")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute Tree", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute Tree")
	TObjectPtr<UTexture2D> Icon = nullptr;

	// Atributo de GAS que mejora esta rama.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute Tree")
	FGameplayAttribute Attribute;

	// Opcional: atributo "actual" asociado a un máximo (ej. Health para MaxHealth). Al subir de nivel recibe la diferencia.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute Tree")
	FGameplayAttribute LinkedCurrentAttribute;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute Tree")
	EAttributeUpgradeDisplay DisplayFormat = EAttributeUpgradeDisplay::Flat;

	// Índice 0 = Nivel I (inicial), índice 4 = Nivel V. La cantidad de entradas define el nivel máximo.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute Tree", meta = (TitleProperty = "Costo: {Cost}  |  Bonus: {TotalBonus}"))
	TArray<FAttributeUpgradeLevel> Levels;

	int32 GetMaxLevel() const { return FMath::Max(Levels.Num(), 1); }

	// Level es 1-based (1 = Nivel I).
	float GetTotalBonusAtLevel(int32 Level) const;

	int32 GetCostForLevel(int32 Level) const;
};

namespace AttributeTreeUtils
{
	// 1 -> "I", 4 -> "IV"...
	TESISUE_API FString ToRoman(int32 Number);

	TESISUE_API FText FormatBonus(float Value, EAttributeUpgradeDisplay Format);

	TESISUE_API FText FormatValue(float Value, EAttributeUpgradeDisplay Format);
}
