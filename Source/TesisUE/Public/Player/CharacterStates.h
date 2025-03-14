#pragma once

UENUM(BlueprintType)
enum class ECharacterStates : uint8
{
	ECS_Unequipped UMETA(DisplayName = "Unequipped"),
	ECS_EquippedSword UMETA(DisplayName = "EquippedSword")
};

UENUM(BlueprintType)
enum class ECharacterActions : uint8
{
	ECA_Nothing UMETA(DisplayName = "Nothing"),
	ECA_Attack UMETA(DisplayName = "Attack"),
	ECA_Dodge UMETA(DisplayName = "Dodge")
};