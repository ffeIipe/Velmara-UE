#pragma once

#include "CoreMinimal.h"
#include "Chaos/ChaosNotifyHandlerInterface.h"
#include "GameFramework/Actor.h"
#include "BossCeilingBreakPoint.generated.h"

class UArrowComponent;
class UGeometryCollectionComponent;

/*
 * Punto del techo que el boss puede romper. Colocalo pegado al techo (la flecha apunta hacia donde cae el derrumbe).
 * Si el boss no tiene suficientes puntos para una etapa, completa con huecos aleatorios dentro de la arena.
 * La roca es un Geometry Collection fracturado: cae con física, se parte contra el suelo y a los pocos segundos queda estática.
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling")
	TArray<TObjectPtr<AActor>> CeilingPiecesToHide;

	// Segundos desde que la roca se parte contra el suelo hasta que sus pedazos quedan estáticos (sin física).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling", meta = (ClampMin = "0.1"))
	float SettleTime = 4.f;

	UFUNCTION(BlueprintCallable, Category = "Boss|Ceiling")
	void Break();

	// Vuelve a mostrar las piezas ocultas del techo. La roca ya caída queda donde está.
	UFUNCTION(BlueprintCallable, Category = "Boss|Ceiling")
	void ResetPoint();

	UFUNCTION(BlueprintPure, Category = "Boss|Ceiling")
	bool IsBroken() const { return bBroken; }

	UFUNCTION(BlueprintPure, Category = "Boss|Ceiling")
	bool CanBreakAtStage(int32 Stage) const { return !bBroken && (StageIndex < 0 || StageIndex == Stage); }

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Ceiling")
	void OnCeilingBroken();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UArrowComponent> Arrow;

	// Roca fracturada (asigná el Geometry Collection en el Blueprint hijo). Sin asset no hace nada.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UGeometryCollectionComponent> Rock;

private:
	UFUNCTION()
	void HandleRockCollision(const FChaosPhysicsCollisionInfo& CollisionInfo);

	void Shatter();
	void Settle();

	bool bBroken = false;
	bool bShattered = false;
	FTimerHandle SettleTimer;
};
