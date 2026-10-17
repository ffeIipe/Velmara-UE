#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SunlightExposureSubsystem.generated.h"

/*
 * Junta la exposición a la luz de varias fuentes (huecos del techo) y la traduce a los eventos del sistema de sol existente:
 * Event.Damage.Sun.Activate al entrar en la primera fuente y Event.Damage.Sun.Deactivate al salir de la última.
 * Así GA_SunBurning sigue decidiendo el daño, el cue y las inmunidades (ej. State.Possessing).
 */
UCLASS()
class TESISUE_API USunlightExposureSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// Llamalo en cada chequeo de la fuente; solo envía eventos cuando cambia el estado (o para refrescar el Activate).
	UFUNCTION(BlueprintCallable, Category = "Sunlight")
	void SetExposed(AActor* Target, UObject* Source, bool bExposed);

	// Quita la fuente de todos los objetivos (al cerrarse o volver al pool).
	UFUNCTION(BlueprintCallable, Category = "Sunlight")
	void ClearSource(UObject* Source);

	UFUNCTION(BlueprintPure, Category = "Sunlight")
	bool IsExposed(const AActor* Target) const;

	// Reenvía Activate mientras siga expuesto, por si otro sistema (ej. BP_SunBurning bajo techo) mandó Deactivate.
	static constexpr double ResendActivateInterval = 1.0;

private:
	struct FExposureEntry
	{
		TArray<TWeakObjectPtr<UObject>> Sources;
		double LastActivateTime = 0.0;
	};

	void SendSunEvent(AActor* Target, bool bActivate) const;

	TMap<TWeakObjectPtr<AActor>, FExposureEntry> Exposures;
};
