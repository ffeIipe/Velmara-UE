#pragma once

UENUM(BlueprintType)
enum class ECharacterWeaponStates : uint8
{
	ECWS_Unequipped UMETA(DisplayName = "Unequipped"),
	ECWS_EquippedWeapon UMETA(DisplayName = "EquippedWeapon"),
	ECWS_EquippingWeapon UMETA(DisplayName = "EquippingWeapon")
};

UENUM(BlueprintType)
enum class ECharacterActionsStates : uint8
{
	ECAS_Nothing UMETA(DisplayName = "Nothing"),
	ECAS_Attack UMETA(DisplayName = "Attack"),
	ECAS_Dodge UMETA(DisplayName = "Dodge"),
	ECAS_Dead UMETA(DisplayName = "Death"),
	ECAS_Block UMETA(DisplayName = "Block"),
	ECAS_Finish UMETA(DisplayName = "Finish"),
	ECAS_Stun UMETA(DisplayName = "Stun")
};

UENUM(BlueprintType)
enum class ECharacterModeStates : uint8
{
	ECMS_Human UMETA(DisplayName = "Human"),
	ECMS_Spectral UMETA(DisplayName = "Spectral"),
	ECMS_Possessing UMETA(DisplayName = "Possessing")
};

UENUM(BlueprintType)
enum class EWeaponCommandType : uint8
{
	EWCT_None UMETA(DisplayName = "None"),
	EWCT_PrimaryAttack UMETA(DisplayName = "PrimaryAttack"),
	EWCT_SecondaryAttack UMETA(DisplayName = "SecondaryAttack"),
	EWCT_JumpAttack UMETA(DisplayName = "JumpAttack"),
	EWCT_LaunchAttack UMETA(DisplayName = "LaunchAttack"),
	EWCT_CrasherAttack UMETA(DisplayName = "CrasherAttack")
};