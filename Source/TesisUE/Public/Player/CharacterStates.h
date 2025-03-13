#pragma once

UENUM(BlueprintType)
enum class ECharacterStates : uint8
{
	ECS_Nothing UMETA(DisplayName = "Nothing"),
	ECS_Attack UMETA(DisplayName = "Attack"),
	ECS_Dodge UMETA(DisplayName = "Dodge")
};