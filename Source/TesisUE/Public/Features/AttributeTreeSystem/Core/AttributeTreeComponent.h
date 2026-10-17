#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "Features/AttributeTreeSystem/Data/AttributeTreeTypes.h"
#include "Features/UpgradeSystem/Core/UpgradeComponentBase.h"
#include "AttributeTreeComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAttributeUpgradedSignature, FGameplayTag, UpgradeTag, int32, NewLevel);

/*
 * Árbol de atributos lineal: cada rama sube de Nivel I a Nivel V aplicando un bonus permanente a un atributo de GAS.
 *
 * La moneda y el menú los maneja UUpgradeComponentBase (moneda vía ICurrencyProvider, tipo configurable en CurrencyType).
 *
 * Agregalo al Blueprint del personaje jugable y llamá a ToggleMenu() desde un input.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TESISUE_API UAttributeTreeComponent : public UUpgradeComponentBase
{
	GENERATED_BODY()

public:
	UAttributeTreeComponent();

	/* MEJORAS */

	UFUNCTION(BlueprintCallable, Category = "Attribute Tree")
	EAttributeUpgradeResult TryUpgrade(FGameplayTag UpgradeTag);

	// Fija un nivel sin cobrar (para cargar partidas o debug). Reaplica el bonus correspondiente.
	UFUNCTION(BlueprintCallable, Category = "Attribute Tree")
	void SetUpgradeLevel(FGameplayTag UpgradeTag, int32 NewLevel);

	// Vuelve a aplicar los bonus de todas las ramas según su nivel actual (por ejemplo, tras cargar una partida).
	UFUNCTION(BlueprintCallable, Category = "Attribute Tree")
	void ReapplyAllUpgrades();

	/* GUARDADO */

	// Niveles comprados de cada rama, para escribirlos en la partida guardada.
	UFUNCTION(BlueprintPure, Category = "Attribute Tree | Save")
	const TMap<FGameplayTag, int32>& GetUpgradeLevels() const { return UpgradeLevels; }

	// Reemplaza todos los niveles por los de una partida guardada (las ramas que no figuran vuelven a Nivel I) y reaplica sus bonus.
	UFUNCTION(BlueprintCallable, Category = "Attribute Tree | Save")
	void RestoreUpgradeLevels(const TMap<FGameplayTag, int32>& SavedLevels);

	/* CONSULTAS */

	UFUNCTION(BlueprintPure, Category = "Attribute Tree | Queries")
	int32 GetUpgradeLevel(FGameplayTag UpgradeTag) const;

	UFUNCTION(BlueprintPure, Category = "Attribute Tree | Queries")
	int32 GetMaxLevel(FGameplayTag UpgradeTag) const;

	UFUNCTION(BlueprintPure, Category = "Attribute Tree | Queries")
	bool IsMaxLevel(FGameplayTag UpgradeTag) const;

	// Costo del próximo nivel, o -1 si ya está al máximo.
	UFUNCTION(BlueprintPure, Category = "Attribute Tree | Queries")
	int32 GetNextLevelCost(FGameplayTag UpgradeTag) const;

	UFUNCTION(BlueprintPure, Category = "Attribute Tree | Queries")
	bool CanAffordUpgrade(FGameplayTag UpgradeTag) const;

	UFUNCTION(BlueprintPure, Category = "Attribute Tree | Queries")
	bool GetUpgradeDefinition(FGameplayTag UpgradeTag, FAttributeUpgradeDefinition& OutDefinition) const;

	UFUNCTION(BlueprintPure, Category = "Attribute Tree | Queries")
	const TArray<FAttributeUpgradeDefinition>& GetUpgrades() const { return Upgrades; }

	// Valor actual (con bonus) del atributo que mejora la rama.
	UFUNCTION(BlueprintPure, Category = "Attribute Tree | Queries")
	float GetCurrentAttributeValue(FGameplayTag UpgradeTag) const;

	/* EVENTOS */

	UPROPERTY(BlueprintAssignable, Category = "Attribute Tree | Events")
	FOnAttributeUpgradedSignature OnAttributeUpgraded;

protected:
	virtual void BeginPlay() override;

	/* CONFIGURACIÓN */

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute Tree | Config", meta = (TitleProperty = "DisplayName"))
	TArray<FAttributeUpgradeDefinition> Upgrades;

	/* ESTADO (guardable) */

	// Nivel actual de cada rama. Las ramas que no figuran están en Nivel I.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attribute Tree | State", SaveGame)
	TMap<FGameplayTag, int32> UpgradeLevels;

private:
	const FAttributeUpgradeDefinition* FindDefinition(const FGameplayTag& UpgradeTag) const;

	UAbilitySystemComponent* GetAbilitySystem() const;

	void ApplyUpgradeEffect(const FAttributeUpgradeDefinition& Definition);

	void AddDefaultUpgrade(const FGameplayTag& Tag, const FText& Name, const FText& Description,
	                       const FGameplayAttribute& Attribute, const TArray<float>& TotalBonuses,
	                       EAttributeUpgradeDisplay DisplayFormat,
	                       const FGameplayAttribute& LinkedCurrentAttribute = FGameplayAttribute());

	// GE dinámicos (infinitos) que aplican el bonus de cada rama.
	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UGameplayEffect>> UpgradeEffects;

	TMap<FGameplayTag, FActiveGameplayEffectHandle> ActiveUpgradeEffectHandles;

	TMap<FGameplayTag, float> AppliedBonuses;
};
