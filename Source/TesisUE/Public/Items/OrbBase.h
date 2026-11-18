#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Interfaces/OrbPoolingInterface.h"
#include "OrbBase.generated.h"

class UNiagaraComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UGameplayEffect;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOrbArrivedSignature, AActor*, Target);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnOrbArrivedNative, AActor*);

/*
 * Orbe base: actor liviano (sin colisión) que viaja desde donde nace hasta un actor objetivo y, al llegar,
 * aplica un GameplayEffect (opcional) al objetivo, avisa por delegado y vuelve al pool (UOrbsPoolManager).
 *
 * El vuelo tiene dos fases pensadas para "game juice":
 *  1. Burst: sale despedido hacia afuera/arriba y describe un arco lento (fricción + giro).
 *  2. Seek: pasado el HoverTime acelera de golpe y curva hacia el objetivo.
 *
 * Se lanza con LaunchToTarget. Sirve para el feedback del tótem (alma enemigo -> tótem) y para vida/energía (orbe -> jugador).
 * Si el objetivo desaparece en pleno vuelo, el orbe vuelve al pool sin avisar de llegada.
 */
UCLASS()
class TESISUE_API AOrbBase : public AActor, public IOrbPoolingInterface
{
	GENERATED_BODY()

public:
	AOrbBase();

	// Lanza el orbe hacia Target. Llamar tras SpawnOrbFromPool.
	UFUNCTION(BlueprintCallable, Category = "Orb")
	void LaunchToTarget(AActor* Target);

	virtual void ActivateOrb_Implementation(const FVector& Location, const FRotator& Rotation) override;
	virtual void DeactivateOrb_Implementation() override;

	virtual void Tick(float DeltaSeconds) override;
	// shortcut: un hitch largo (clamp a 1/30 s) estira el tiempo del vuelo, no es exacto en tiempo real.

	// Para código C++ (por ejemplo el tótem). Se limpia al desactivar el orbe.
	FOnOrbArrivedNative OnArrivedNative;

	UPROPERTY(BlueprintAssignable, Category = "Orb")
	FOnOrbArrivedSignature OnArrived;

protected:
	// Efecto aplicado al objetivo al llegar. Vacío = solo feedback.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orb | Effect")
	TSubclassOf<UGameplayEffect> EffectClass;

	// Tag SetByCaller con el que se pasa Magnitude al efecto (ej. Data.Attributes.Health).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orb | Effect", meta = (EditCondition = "EffectClass != nullptr"))
	FGameplayTag MagnitudeTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orb | Effect", meta = (EditCondition = "EffectClass != nullptr"))
	float Magnitude = 5.f;

	/* FASE 1: salida. El orbe es despedido hacia afuera/arriba y describe un arco lento (frena por fricción mientras la dirección gira). */

	// Velocidad inicial horizontal con la que sale despedido (dirección al azar).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orb | Burst", meta = (Units = "cm/s"))
	float BurstSpeed = 450.f;

	// Componente vertical extra de la salida, para que el orbe suba antes del arco.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orb | Burst", meta = (Units = "cm/s"))
	float BurstUpSpeed = 250.f;

	// Fricción: más alto = frena antes y el arco queda más corto.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orb | Burst")
	float BurstDrag = 2.f;

	// Cuánto gira la dirección durante el arco (el sentido se sortea por orbe).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orb | Burst", meta = (Units = "deg/s"))
	float ArcTurnRate = 200.f;

	// Duración de la fase lenta; se sortea entre estos dos valores para que los orbes no lleguen todos juntos.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orb | Burst", meta = (Units = "s"))
	float HoverTimeMin = 0.9f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orb | Burst", meta = (Units = "s"))
	float HoverTimeMax = 1.4f;

	/* FASE 2: succión. Acelera de golpe hacia el objetivo, curvando suave. */

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orb | Seek", meta = (Units = "cm/s^2"))
	float SeekAcceleration = 7000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orb | Seek", meta = (Units = "cm/s"))
	float MaxSpeed = 3500.f;

	// Qué tan rápido gira hacia el objetivo (por segundo). Más bajo = curva más abierta.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orb | Seek")
	float SteerRate = 7.f;

	// Distancia a la que se considera que llegó.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orb | Seek", meta = (Units = "cm"))
	float ArriveDistance = 15.f;

	// Punto de impacto relativo al actor objetivo. Cero = centro de la cápsula (centro del cuerpo en un Character).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orb | Seek")
	FVector TargetOffset = FVector::ZeroVector;

	// Seguridad: si no llega en este tiempo, vuelve al pool.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orb | Seek", meta = (Units = "s"))
	float MaxLifetime = 8.f;

	// Evento para VFX/SFX extra al llegar (se llama antes de volver al pool).
	UFUNCTION(BlueprintImplementableEvent, Category = "Orb")
	void ReceiveArrived(AActor* Target);

	// Cuerpo visual del orbe (esfera + material) y su luz; se configuran en cada BP hijo.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Orb")
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Orb")
	TObjectPtr<UPointLightComponent> Light;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Orb")
	TObjectPtr<UNiagaraComponent> Niagara;

private:
	void Arrive();
	void ReturnToPool();
	void ApplyEffect(AActor* Target) const;

	TWeakObjectPtr<AActor> TargetActor;
	FVector Velocity = FVector::ZeroVector;
	float Age = 0.f;
	float HoverTime = 1.f;
	float ArcSign = 1.f;
};
