#include "Components/ProgressionComponent.h"

#include "Features/CurrencySystem/Data/CurrencyTags.h"

UProgressionComponent::UProgressionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	AvailableCurrency = 0;
	ProvidedCurrencyType = CurrencyTags::Blood;
}

void UProgressionComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

bool UProgressionComponent::TryBuyUpgrade(FGameplayTag TagToGrant, int32 Cost)
{
	if (!TagToGrant.IsValid() || AvailableCurrency < Cost)
	{
		return false;
	}

	AvailableCurrency -= Cost;
	OnCurrencyChanged.Broadcast(AvailableCurrency);

	if (TagToGrant.MatchesTag(BaseComboTag))
	{
		if (!UnlockedCombos.HasTagExact(TagToGrant))
		{
			UnlockedCombos.AddTag(TagToGrant);
			OnComboUnlocked.Broadcast(TagToGrant);
		}
	}
	else if (TagToGrant.MatchesTag(BaseStatTag))
	{
		int32 CurrentLevel = StatUpgrades.Contains(TagToGrant) ? StatUpgrades[TagToGrant] : 0;
		int32 NewLevel = CurrentLevel + 1;
		
		StatUpgrades.Add(TagToGrant, NewLevel);
		OnStatUpgraded.Broadcast(TagToGrant, NewLevel);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("TryBuyUpgrade: El Tag %s no desciende de Combo ni de Stat."), *TagToGrant.ToString());
		AvailableCurrency += Cost; 
		OnCurrencyChanged.Broadcast(AvailableCurrency);
		return false;
	}

	return true;
}

void UProgressionComponent::AddCurrency(int32 Amount)
{
	AvailableCurrency += Amount;
	
	if (AvailableCurrency < 0)
	{
		AvailableCurrency = 0;
	}

	OnCurrencyChanged.Broadcast(AvailableCurrency);
}

bool UProgressionComponent::ProvidesCurrency_Implementation(const FGameplayTag CurrencyType) const
{
	return CurrencyType.IsValid() && CurrencyType == ProvidedCurrencyType;
}

int32 UProgressionComponent::GetCurrencyAmount_Implementation(const FGameplayTag CurrencyType) const
{
	return ProvidesCurrency_Implementation(CurrencyType) ? AvailableCurrency : 0;
}

bool UProgressionComponent::TrySpendCurrency_Implementation(const FGameplayTag CurrencyType, const int32 Amount)
{
	if (!ProvidesCurrency_Implementation(CurrencyType) || Amount < 0 || AvailableCurrency < Amount)
	{
		return false;
	}

	AvailableCurrency -= Amount;
	OnCurrencyChanged.Broadcast(AvailableCurrency);

	return true;
}

bool UProgressionComponent::HasComboUnlocked(FGameplayTag ComboTag) const
{
	return UnlockedCombos.HasTagExact(ComboTag);
}

int32 UProgressionComponent::GetStatLevel(FGameplayTag StatTag) const
{
	if (const int32* FoundLevel = StatUpgrades.Find(StatTag))
	{
		return *FoundLevel;
	}
	
	return 0;
}