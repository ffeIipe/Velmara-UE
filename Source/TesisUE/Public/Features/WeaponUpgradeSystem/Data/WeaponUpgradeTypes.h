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
	// Se puede comprar (requisitos cumplidos).
	Available,
	// Falta desbloquear un combo previo.
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

/*
 * Combo desbloqueable de un arma. Se bloquea en el árbol de combos poniendo UnlockTag como RequiredUnlockTag
 * en el UComboNode que inicia la rama (en el Data Asset del arma).
 */
USTRUCT(BlueprintType)
struct FWeaponComboUpgrade
{
	GENERATED_BODY()

	// Debe coincidir con el RequiredUnlockTag del UComboNode que abre la rama.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Upgrade", meta = (Categories = "Progression.Combo"))
	FGameplayTag UnlockTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Upgrade")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Upgrade", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Upgrade")
	TObjectPtr<UTexture2D> Icon = nullptr;

	// Costo en Sangre (o en la moneda configurada en el UWeaponUpgradeComponent).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Upgrade", meta = (ClampMin = 0))
	int32 Cost = 150;

	// Combos que hay que tener desbloqueados antes. Además, si la rama cuelga de otra rama bloqueada, esa se exige sola.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Upgrade", meta = (Categories = "Progression.Combo"))
	FGameplayTagContainer RequiredUnlocks;
};

// Dónde está un combo dentro del árbol de UComboNode del arma y cómo se ejecuta.
USTRUCT(BlueprintType)
struct FComboInputPath
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Weapon Upgrade")
	bool bFound = false;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon Upgrade")
	EComboRoot Root = EComboRoot::None;

	// Secuencia de inputs para llegar al primer golpe del combo (incluye el input que lo dispara).
	UPROPERTY(BlueprintReadOnly, Category = "Weapon Upgrade")
	TArray<EComboInput> Inputs;

	// Cantidad de golpes de la rama (nodos que dependen de este desbloqueo).
	UPROPERTY(BlueprintReadOnly, Category = "Weapon Upgrade")
	int32 HitCount = 0;

	// Desbloqueo de la rama bloqueada de la que cuelga este combo (requisito implícito), si la hay.
	UPROPERTY(BlueprintReadOnly, Category = "Weapon Upgrade")
	FGameplayTag ParentUnlockTag;
};

namespace WeaponUpgradeUtils
{
	// Busca el nodo cuyo RequiredUnlockTag es UnlockTag en el árbol de combos del arma (solo USwordData por ahora).
	TESISUE_API FComboInputPath FindComboPath(const UWeaponData* Weapon, const FGameplayTag& UnlockTag);

	// Todos los RequiredUnlockTag usados en el árbol de combos del arma, en orden de aparición.
	TESISUE_API void CollectUnlockTags(const UWeaponData* Weapon, TArray<FGameplayTag>& OutTags);

	// "Suelo: Pulsar · Pulsar · Mantener"
	TESISUE_API FText DescribeComboInput(const FComboInputPath& Path);

	// Nombre del arma para la UI (DisplayName del Data Asset o su nombre sin el prefijo DA_).
	TESISUE_API FText GetWeaponDisplayName(const UWeaponData* Weapon);
}
