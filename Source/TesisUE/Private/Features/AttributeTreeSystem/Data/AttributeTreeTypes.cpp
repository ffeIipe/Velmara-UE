#include "Features/AttributeTreeSystem/Data/AttributeTreeTypes.h"

namespace AttributeTreeTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Stat_MaxHealth, "Progression.Stats.MaxHealth", "Rama de vida máxima del árbol de atributos");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Stat_Armor, "Progression.Stats.Armor", "Rama de armadura del árbol de atributos");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Stat_BaseDamage, "Progression.Stats.BaseDamage", "Rama de daño base del árbol de atributos");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Stat_AttackSpeed, "Progression.Stats.AttackSpeed", "Rama de velocidad de ataque del árbol de atributos");
}

float FAttributeUpgradeDefinition::GetTotalBonusAtLevel(const int32 Level) const
{
	if (Levels.IsEmpty()) return 0.f;

	return Levels[FMath::Clamp(Level, 1, Levels.Num()) - 1].TotalBonus;
}

int32 FAttributeUpgradeDefinition::GetCostForLevel(const int32 Level) const
{
	if (!Levels.IsValidIndex(Level - 1)) return 0;

	return Levels[Level - 1].Cost;
}

namespace AttributeTreeUtils
{
	FString ToRoman(int32 Number)
	{
		if (Number <= 0) return TEXT("-");

		static const int32 Values[] = {1000, 900, 500, 400, 100, 90, 50, 40, 10, 9, 5, 4, 1};
		static const TCHAR* Symbols[] = {TEXT("M"), TEXT("CM"), TEXT("D"), TEXT("CD"), TEXT("C"), TEXT("XC"),
		                                 TEXT("L"), TEXT("XL"), TEXT("X"), TEXT("IX"), TEXT("V"), TEXT("IV"), TEXT("I")};

		FString Result;
		for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(Values)); ++Index)
		{
			while (Number >= Values[Index])
			{
				Result += Symbols[Index];
				Number -= Values[Index];
			}
		}
		return Result;
	}

	static FString FormatNumber(const float Value)
	{
		return FMath::IsNearlyEqual(Value, FMath::RoundToFloat(Value))
			? FString::Printf(TEXT("%d"), FMath::RoundToInt(Value))
			: FString::Printf(TEXT("%.1f"), Value);
	}

	FText FormatBonus(const float Value, const EAttributeUpgradeDisplay Format)
	{
		const float Shown = Format == EAttributeUpgradeDisplay::Percent ? Value * 100.f : Value;
		const TCHAR* Sign = Shown < 0.f ? TEXT("") : TEXT("+");
		const TCHAR* Suffix = Format == EAttributeUpgradeDisplay::Percent ? TEXT("%") : TEXT("");

		return FText::FromString(FString::Printf(TEXT("%s%s%s"), Sign, *FormatNumber(Shown), Suffix));
	}

	FText FormatValue(const float Value, const EAttributeUpgradeDisplay Format)
	{
		if (Format == EAttributeUpgradeDisplay::Percent)
		{
			return FText::FromString(FString::Printf(TEXT("%s%%"), *FormatNumber(Value * 100.f)));
		}
		return FText::FromString(FormatNumber(Value));
	}
}
