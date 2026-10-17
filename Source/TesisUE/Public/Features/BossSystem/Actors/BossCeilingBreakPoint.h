#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BossCeilingBreakPoint.generated.h"

class UArrowComponent;

/*
 * Punto del techo que el boss puede romper. Colocalo pegado al techo (la flecha apunta hacia donde cae la luz).
 * Si el boss no tiene suficientes puntos para una etapa, completa con huecos aleatorios dentro de la arena.
 */
UCLASS()
class TESISUE_API ABossCeilingBreakPoint : public AActor
{
	GENERATED_BODY()

public:
	ABossCeilingBreakPoint();

	// Etapa en la que se rompe (0 = primer techo). -1 = cualquier etapa.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling")
	int32 StageIndex = -1;

	// 0 = usa el radio por defecto del boss.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling", meta = (ClampMin = "0"))
	float ShaftRadiusOverride = 0.f;

	// Piezas del techo que desaparecen al romperse (ej. un tile del techo encima de este punto).
	// Para destrucción con Chaos (Geometry Collection) usá el evento OnCeilingBroken.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling")
	TArray<TObjectPtr<AActor>> CeilingPiecesToHide;

	UFUNCTION(BlueprintCallable, Category = "Boss|Ceiling")
	void Break();

	UFUNCTION(BlueprintCallable, Category = "Boss|Ceiling")
	void ResetPoint();

	UFUNCTION(BlueprintPure, Category = "Boss|Ceiling")
	bool IsBroken() const { return bBroken; }

	UFUNCTION(BlueprintPure, Category = "Boss|Ceiling")
	bool CanBreakAtStage(int32 Stage) const { return !bBroken && (StageIndex < 0 || StageIndex == Stage); }

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Ceiling")
	void OnCeilingBroken();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UArrowComponent> Arrow;

private:
	bool bBroken = false;
};
