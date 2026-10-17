#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "CurrencyProvider.generated.h"

/*
 * Contrato que debe cumplir cualquier sistema de moneda para que otros sistemas (árbol de atributos, altar de sangre,
 * tienda, etc.) puedan consultar y gastar moneda sin conocer su implementación concreta.
 *
 * Cada moneda se identifica con un tag "Currency.*" (ver CurrencyTags). Un proveedor puede manejar uno o varios tipos.
 *
 * Puede implementarse en C++ o en Blueprint, en el Pawn, en alguno de sus componentes, en el PlayerController,
 * en el PlayerState o en el GameInstance: los sistemas de mejora lo buscan automáticamente en ese orden.
 */
UINTERFACE(Blueprintable)
class UCurrencyProvider : public UInterface
{
	GENERATED_BODY()
};

class TESISUE_API ICurrencyProvider
{
	GENERATED_BODY()

public:
	// Indica si este proveedor maneja el tipo de moneda pedido.
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Currency")
	bool ProvidesCurrency(FGameplayTag CurrencyType) const;

	// Cantidad disponible actualmente del tipo de moneda pedido.
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Currency")
	int32 GetCurrencyAmount(FGameplayTag CurrencyType) const;

	// Intenta descontar Amount del tipo pedido. Devuelve false (sin modificar nada) si no alcanza.
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Currency")
	bool TrySpendCurrency(FGameplayTag CurrencyType, int32 Amount);
};
