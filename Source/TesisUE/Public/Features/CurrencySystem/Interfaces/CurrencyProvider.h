#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "CurrencyProvider.generated.h"

UINTERFACE(Blueprintable)
class UCurrencyProvider : public UInterface
{
	GENERATED_BODY()
};

class TESISUE_API ICurrencyProvider
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Currency")
	bool ProvidesCurrency(FGameplayTag CurrencyType) const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Currency")
	int32 GetCurrencyAmount(FGameplayTag CurrencyType) const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Currency")
	bool TrySpendCurrency(FGameplayTag CurrencyType, int32 Amount);
};
