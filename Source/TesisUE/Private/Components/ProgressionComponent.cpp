#include "Components/ProgressionComponent.h"

#include "Features/CurrencySystem/Data/CurrencyTags.h"

UProgressionComponent::UProgressionComponent()
{
	// Desactivamos el Tick, este componente es puramente reactivo.
	PrimaryComponentTick.bCanEverTick = false;

	AvailableCurrency = 0;
	ProvidedCurrencyType = CurrencyTags::Blood;
}

void UProgressionComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// Aquí, luego de cargar la partida (si lo haces en el BeginPlay o en un evento de inicialización), 
	// deberías disparar OnCurrencyChanged y OnStatUpgraded para que la UI inicial y GAS se sincronicen.
}

bool UProgressionComponent::TryBuyUpgrade(FGameplayTag TagToGrant, int32 Cost)
{
	// 1. Sanity Check
	if (!TagToGrant.IsValid() || AvailableCurrency < Cost)
	{
		return false;
	}

	// 2. Restar moneda
	AvailableCurrency -= Cost;
	OnCurrencyChanged.Broadcast(AvailableCurrency);

	// 3. Determinar qué estamos comprando
	if (TagToGrant.MatchesTag(BaseComboTag))
	{
		// Es un Combo. Verificamos que no lo tenga ya.
		if (!UnlockedCombos.HasTagExact(TagToGrant))
		{
			UnlockedCombos.AddTag(TagToGrant);
			OnComboUnlocked.Broadcast(TagToGrant);
		}
	}
	else if (TagToGrant.MatchesTag(BaseStatTag))
	{
		// Es una estadística. Sumamos 1 a su nivel.
		int32 CurrentLevel = StatUpgrades.Contains(TagToGrant) ? StatUpgrades[TagToGrant] : 0;
		int32 NewLevel = CurrentLevel + 1;
		
		StatUpgrades.Add(TagToGrant, NewLevel);
		OnStatUpgraded.Broadcast(TagToGrant, NewLevel);
	}
	else
	{
		// Fallback: Si no es ni combo ni stat, podríamos registrar un warning.
		UE_LOG(LogTemp, Warning, TEXT("TryBuyUpgrade: El Tag %s no desciende de Combo ni de Stat."), *TagToGrant.ToString());
		// Reembolsamos por error de configuración
		AvailableCurrency += Cost; 
		OnCurrencyChanged.Broadcast(AvailableCurrency);
		return false;
	}

	// 4. (Opcional) Aquí podrías llamar directamente a una función SaveGame o 
	// dejar que el GameMode o Character gestionen el guardado al cerrar el menú.
	
	return true;
}

void UProgressionComponent::AddCurrency(int32 Amount)
{
	// Permitimos valores negativos si alguna vez necesitas quitar dinero externamente
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