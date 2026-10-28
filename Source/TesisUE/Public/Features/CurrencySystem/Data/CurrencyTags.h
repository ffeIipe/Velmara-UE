#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

/*
 * Tipos de moneda del juego. Cada consumidor (árbol de atributos, altar de sangre, tienda...) pide un tipo concreto
 * y cada ICurrencyProvider declara cuáles maneja.
 */
namespace CurrencyTags
{
	// "Sangre": la otorgan los enemigos al morir. Se usa para desbloquear combos de armas.
	TESISUE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Blood);
}
