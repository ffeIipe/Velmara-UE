#pragma once

UENUM(BlueprintType)
enum class ECharacterHumanStates : uint8
{
	ECHS_Unequipped UMETA(DisplayName = "Unequipped"),
	ECHS_EquippedSword UMETA(DisplayName = "EquippedSword"),
	ECHS_EquippingSword UMETA(DisplayName = "EquippingSword")
};

UENUM(BlueprintType)
enum class ECharacterSpectralStates : uint8
{
	ECSS_Unequipped UMETA(DisplayName = "Unequipped"),
	ECSS_EquippedPistol UMETA(DisplayName = "EquippedPistol"),
	ECSS_EquippingPistol UMETA(DisplayName = "EquippingPistol")
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
enum class ECharacterMode : uint8
{
	ECM_Human UMETA(DisplayName = "Human"),
	ECM_Spectral UMETA(DisplayName = "Spectral"),
	ECM_Possessing UMETA(DisplayName = "Possessing")
};