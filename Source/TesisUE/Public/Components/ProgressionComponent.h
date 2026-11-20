#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Features/CurrencySystem/Interfaces/CurrencyProvider.h"
#include "ProgressionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCurrencyChangedSignature, int32, NewCurrencyAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnComboUnlockedSignature, FGameplayTag, UnlockedTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStatUpgradedSignature, FGameplayTag, StatTag, int32, NewLevel);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TESISUE_API UProgressionComponent : public UActorComponent, public ICurrencyProvider
{
	GENERATED_BODY()

public:	
	UProgressionComponent();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression | State", SaveGame)
	int32 AvailableCurrency;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression | State", SaveGame)
	FGameplayTagContainer UnlockedCombos;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression | State", SaveGame)
	TMap<FGameplayTag, int32> StatUpgrades;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression | Config")
	FGameplayTag BaseComboTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression | Config")
	FGameplayTag BaseStatTag;

public:

	UPROPERTY(BlueprintAssignable, Category = "Progression | Events")
	FOnCurrencyChangedSignature OnCurrencyChanged;

	UPROPERTY(BlueprintAssignable, Category = "Progression | Events")
	FOnComboUnlockedSignature OnComboUnlocked;

	UPROPERTY(BlueprintAssignable, Category = "Progression | Events")
	FOnStatUpgradedSignature OnStatUpgraded;

	UFUNCTION(BlueprintCallable, Category = "Progression")
	bool TryBuyUpgrade(FGameplayTag TagToGrant, int32 Cost);

	UFUNCTION(BlueprintCallable, Category = "Progression")
	void AddCurrency(int32 Amount);

	UFUNCTION(BlueprintPure, Category = "Progression | Queries")
	bool HasComboUnlocked(FGameplayTag ComboTag) const;

	UFUNCTION(BlueprintPure, Category = "Progression | Queries")
	int32 GetStatLevel(FGameplayTag StatTag) const;

	UFUNCTION(BlueprintPure, Category = "Progression | Queries")
	int32 GetAvailableCurrency() const { return AvailableCurrency; }

	virtual bool ProvidesCurrency_Implementation(FGameplayTag CurrencyType) const override;

	virtual int32 GetCurrencyAmount_Implementation(FGameplayTag CurrencyType) const override;

	virtual bool TrySpendCurrency_Implementation(FGameplayTag CurrencyType, int32 Amount) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression | Config", meta = (Categories = "Currency"))
	FGameplayTag ProvidedCurrencyType;
};