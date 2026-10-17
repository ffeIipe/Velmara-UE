#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TotemSubsystem.generated.h"

class UTotemChargeComponent;

/*
 * Registro de los tótems recargables del nivel. Recibe las muertes de las entidades y se las pasa a cada tótem,
 * que decide si la muerte le suma carga (distancia, equipo, si ya está cargado, etc.).
 */
UCLASS()
class TESISUE_API UTotemSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void RegisterTotem(UTotemChargeComponent* Totem);

	void UnregisterTotem(UTotemChargeComponent* Totem);

	// AEntity lo llama automáticamente al recibir el tag State.Dead. Llamalo desde Blueprint solo para actores que no son AEntity.
	UFUNCTION(BlueprintCallable, Category = "Totem")
	void NotifyEntityDied(AActor* DeadActor);

private:
	TArray<TWeakObjectPtr<UTotemChargeComponent>> Totems;
};
