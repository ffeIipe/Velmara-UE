#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EnemyEditorLibrary.generated.h"

class UAnimMontage;

UCLASS()
class TESISUE_API UEnemyEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/*
	 * Copia los notifies de un montage de ataque existente (colisión de espada, ventana de combo, motion warping, sonido,
	 * trail...) a Montage, reubicándolos según el golpe del nuevo: colisión en [HitStart, HitEnd], ventana de combo en
	 * [HitEnd, ComboWindowEnd], warping antes del golpe y sonido justo antes. También copia los blend in/out.
	 * Solo editor (devuelve false en el juego). Guardá el asset después.
	 */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Editor")
	static bool CopyAttackNotifiesFromTemplate(UAnimMontage* Montage, UAnimMontage* Template, float HitStart, float HitEnd, float ComboWindowEnd);
};
