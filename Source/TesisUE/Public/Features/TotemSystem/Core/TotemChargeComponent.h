#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "TotemChargeComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTotemEnemyAbsorbedSignature, AActor*, DeadEnemy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTotemChargeChangedSignature, int32, CurrentKills, int32, KillsToCharge);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTotemChargeStateSignature);

/*
 * Feature de recarga para tótems: el tótem solo puede ser poseído cuando está cargado.
 *
 * Cada enemigo que muere dentro de ChargeRadius suma una carga. Al llegar a KillsToCharge el tótem queda cargado
 * y vuelve a ser poseíble; al poseerlo se consume la carga (si bConsumeChargeOnPossession está activo).
 *
 * Tótem fijo   -> sin este componente, o con bRequiresCharge desactivado: siempre se puede poseer.
 * Tótem recargable -> con este componente y bRequiresCharge activo.
 *
 * Mientras está descargado, el tótem ignora el canal de traza de posesión, así GA_Possess no lo detecta ni lo resalta.
 * Las muertes llegan a través de UTotemSubsystem, que AEntity notifica cuando recibe el tag State.Dead.
 */
UCLASS(ClassGroup=(Totem), meta=(BlueprintSpawnableComponent))
class TESISUE_API UTotemChargeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTotemChargeComponent();

	/* CONSULTAS */

	// Devuelve true si el actor no usa recarga (tótem fijo / cualquier otra entidad) o si su tótem está cargado.
	UFUNCTION(BlueprintPure, Category = "Totem | Charge", meta = (DefaultToSelf = "Actor"))
	static bool CanActorBePossessed(const AActor* Actor);

	UFUNCTION(BlueprintPure, Category = "Totem | Charge")
	bool RequiresCharge() const { return bRequiresCharge; }

	// Un tótem que no requiere carga se considera siempre cargado.
	UFUNCTION(BlueprintPure, Category = "Totem | Charge")
	bool IsCharged() const;

	UFUNCTION(BlueprintPure, Category = "Totem | Charge")
	int32 GetCurrentKills() const { return CurrentKills; }

	UFUNCTION(BlueprintPure, Category = "Totem | Charge")
	int32 GetKillsToCharge() const { return KillsToCharge; }

	// 0..1, pensado para barras de carga o parámetros de material.
	UFUNCTION(BlueprintPure, Category = "Totem | Charge")
	float GetChargePercent() const;

	UFUNCTION(BlueprintPure, Category = "Totem | Charge")
	float GetChargeRadius() const { return ChargeRadius; }

	/* MODIFICADORES */

	// Llamado por UTotemSubsystem cuando muere una entidad. Devuelve true si la muerte sumó carga.
	UFUNCTION(BlueprintCallable, Category = "Totem | Charge")
	bool TryAbsorbDeath(AActor* DeadActor);

	// Suma cargas manualmente (por ejemplo, un enemigo élite que vale doble).
	UFUNCTION(BlueprintCallable, Category = "Totem | Charge")
	void AddCharge(int32 Kills = 1);

	UFUNCTION(BlueprintCallable, Category = "Totem | Charge")
	void FillCharge();

	// Vacía la carga. Se llama sola al poseer el tótem si bConsumeChargeOnPossession está activo.
	UFUNCTION(BlueprintCallable, Category = "Totem | Charge")
	void ConsumeCharge();

	// Convierte el tótem en fijo (false) o recargable (true) en runtime.
	UFUNCTION(BlueprintCallable, Category = "Totem | Charge")
	void SetRequiresCharge(bool bNewRequiresCharge);

	/* EVENTOS */

	// Un enemigo murió cerca y sumó carga. Útil para VFX (por ejemplo, un alma viajando del enemigo al tótem).
	UPROPERTY(BlueprintAssignable, Category = "Totem | Events")
	FOnTotemEnemyAbsorbedSignature OnEnemyAbsorbed;

	UPROPERTY(BlueprintAssignable, Category = "Totem | Events")
	FOnTotemChargeChangedSignature OnChargeChanged;

	UPROPERTY(BlueprintAssignable, Category = "Totem | Events")
	FOnTotemChargeStateSignature OnFullyCharged;

	UPROPERTY(BlueprintAssignable, Category = "Totem | Events")
	FOnTotemChargeStateSignature OnChargeConsumed;

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/* CONFIGURACIÓN */

	// Desactivalo en una instancia para que se comporte como un tótem fijo.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Totem | Charge")
	bool bRequiresCharge = true;

	// Enemigos que tienen que morir cerca para cargarlo por completo.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Totem | Charge", meta = (ClampMin = "1", EditCondition = "bRequiresCharge"))
	int32 KillsToCharge = 3;

	// Distancia máxima (desde el tótem) a la que una muerte cuenta como carga.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Totem | Charge", meta = (ClampMin = "0", Units = "cm", EditCondition = "bRequiresCharge"))
	float ChargeRadius = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Totem | Charge", meta = (EditCondition = "bRequiresCharge"))
	bool bStartCharged = false;

	// Si está activo, poseer el tótem gasta la carga y hay que volver a recargarlo.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Totem | Charge", meta = (EditCondition = "bRequiresCharge"))
	bool bConsumeChargeOnPossession = true;

	// Solo cuentan las muertes de entidades de un equipo distinto al del jugador.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Totem | Charge", meta = (EditCondition = "bRequiresCharge"))
	bool bOnlyCountEnemies = true;

	/* POSESIÓN */

	// Mientras está descargado, el tótem ignora este canal para que la traza de GA_Possess no lo encuentre.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Totem | Possession")
	bool bIgnorePossessionTraceWhileUncharged = true;

	// Canal "Possession" (ECC_GameTraceChannel6 en DefaultEngine.ini).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Totem | Possession", meta = (EditCondition = "bIgnorePossessionTraceWhileUncharged"))
	TEnumAsByte<ECollisionChannel> PossessionTraceChannel = ECC_GameTraceChannel6;

	/* VISUALES */

	// Parámetro escalar (0..1) que se actualiza en los materiales de las mallas del tótem. None para desactivarlo.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Totem | Visuals")
	FName ChargeMaterialParameter = FName("ChargePercent");

private:
	UFUNCTION()
	void HandleOwnerControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

	void SetCurrentKills(int32 NewKills);

	// Refresca colisión y materiales, y dispara los eventos según si el tótem cambió de estado.
	void NotifyChargeChanged(bool bWasCharged);

	bool IsOwnerPlayerControlled() const;

	bool IsEnemyOfPlayer(const AActor* DeadActor) const;

	void RefreshPossessionTraceResponse();

	void RefreshChargeMaterials() const;

	UPROPERTY(VisibleInstanceOnly, Category = "Totem | Charge")
	int32 CurrentKills = 0;

	// Respuestas originales al canal de posesión, para restaurarlas cuando el tótem se carga.
	TMap<TWeakObjectPtr<UPrimitiveComponent>, TEnumAsByte<ECollisionResponse>> CachedTraceResponses;
};
