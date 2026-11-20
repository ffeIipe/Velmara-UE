#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "Features/AttributeTreeSystem/Data/AttributeTreeTypes.h"
#include "Features/UpgradeSystem/Core/UpgradeComponentBase.h"
#include "AttributeTreeComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAttributeUpgradedSignature, FGameplayTag, UpgradeTag, int32, NewLevel);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TESISUE_API UAttributeTreeComponent : public UUpgradeComponentBase
{
	GENERATED_BODY()

public:
	UAttributeTreeComponent();

	UFUNCTION(BlueprintCallable, Category = "Attribute Tree")
	EAttributeUpgradeResult TryUpgrade(FGameplayTag UpgradeTag);

	UFUNCTION(BlueprintCallable, Category = "Attribute Tree")
	void SetUpgradeLevel(FGameplayTag UpgradeTag, int32 NewLevel);

	UFUNCTION(BlueprintCallable, Category = "Attribute Tree")
	void ReapplyAllUpgrades();

	UFUNCTION(BlueprintPure, Category = "Attribute Tree | Save")
	const TMap<FGameplayTag, int32>& GetUpgradeLevels() const { return UpgradeLevels; }

	UFUNCTION(BlueprintCallable, Category = "Attribute Tree | Save")
	void RestoreUpgradeLevels(const TMap<FGameplayTag, int32>& SavedLevels);

	UFUNCTION(BlueprintPure, Category = "Attribute Tree | Queries")
	int32 GetUpgradeLevel(FGameplayTag UpgradeTag) const;

	UFUNCTION(BlueprintPure, Category = "Attribute Tree | Queries")
	int32 GetMaxLevel(FGameplayTag UpgradeTag) const;

	UFUNCTION(BlueprintPure, Category = "Attribute Tree | Queries")
	bool IsMaxLevel(FGameplayTag UpgradeTag) const;

	UFUNCTION(BlueprintPure, Category = "Attribute Tree | Queries")
	int32 GetNextLevelCost(FGameplayTag UpgradeTag) const;

	UFUNCTION(BlueprintPure, Category = "Attribute Tree | Queries")
	bool CanAffordUpgrade(FGameplayTag UpgradeTag) const;

	UFUNCTION(BlueprintPure, Category = "Attribute Tree | Queries")
	bool GetUpgradeDefinition(FGameplayTag UpgradeTag, FAttributeUpgradeDefinition& OutDefinition) const;

	UFUNCTION(BlueprintPure, Category = "Attribute Tree | Queries")
	const TArray<FAttributeUpgradeDefinition>& GetUpgrades() const { return Upgrades; }

	UFUNCTION(BlueprintPure, Category = "Attribute Tree | Queries")
	float GetCurrentAttributeValue(FGameplayTag UpgradeTag) const;

	UPROPERTY(BlueprintAssignable, Category = "Attribute Tree | Events")
	FOnAttributeUpgradedSignature OnAttributeUpgraded;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute Tree | Config", meta = (TitleProperty = "DisplayName"))
	TArray<FAttributeUpgradeDefinition> Upgrades;

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

	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UGameplayEffect>> UpgradeEffects;

	TMap<FGameplayTag, FActiveGameplayEffectHandle> ActiveUpgradeEffectHandles;

	TMap<FGameplayTag, float> AppliedBonuses;
};
