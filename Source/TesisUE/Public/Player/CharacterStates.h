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
	ECA_Dodge UMETA(DisplayName = "Dodge"),
	ECA_Dead UMETA(DisplayName = "Death"),
	ECA_Block UMETA(DisplayName = "Block"),
	ECA_Finish UMETA(DisplayName = "Finish"),
	ECA_Stun UMETA(DisplayName = "Stun")
};

UENUM(BlueprintType)
enum class ECharacterForm : uint8
{
	ECF_Human UMETA(DisplayName = "Human"),
	ECF_Spectral UMETA(DisplayName = "Spectral"),
	ECF_Possessing UMETA(DisplayName = "Possessing")
};