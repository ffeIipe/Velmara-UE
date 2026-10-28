#pragma once

#include "CoreMinimal.h"
#include "Features/BossSystem/Data/BossTypes.h"
#include "GameFramework/Actor.h"
#include "GenericPool/Interfaces/PoolableActor.h"
#include "BossLightShaft.generated.h"

class ABossCeilingBreakPoint;
class UCameraShakeBase;
class UDecalComponent;
class UMaterialInterface;
class UNiagaraComponent;
class UNiagaraSystem;
class USoundBase;
class USpotLightComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ELightShaftDamageMode : uint8
{
	// Usa el sistema de sol existente (GA_SunBurning): respeta sus inmunidades, cue y sonido.
	SunBurningEvent,
	// Aplica el GE de daño directamente cada tick.
	DirectDamage,
	Both
};

/*
 * Hueco de luz que deja el boss al romper el techo. Primero avisa (polvo + luz tenue), después caen los escombros
 * (daño de impacto en el área) y queda una columna de luz que quema a Velmara mientras esté adentro y sin nada que la cubra.
 */
UCLASS()
class TESISUE_API ABossLightShaft : public AActor, public IPoolableActor
{
	GENERATED_BODY()

public:
	ABossLightShaft();

	// Top = hueco en el techo, Floor = punto del piso debajo.
	UFUNCTION(BlueprintCallable, Category = "Boss|Light Shaft")
	void OpenShaft(AActor* InBoss, const FVector& InTop, const FVector& InFloor, float InRadius, const FBossDamageParams& InDamageParams,
		ABossCeilingBreakPoint* InBreakPoint = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Boss|Light Shaft")
	void CloseShaft();

	UFUNCTION(BlueprintPure, Category = "Boss|Light Shaft")
	bool IsShaftOpen() const { return Phase == EShaftPhase::Open; }

	UFUNCTION(BlueprintPure, Category = "Boss|Light Shaft")
	bool IsPointInsideShaft(const FVector& Point) const;

	UFUNCTION(BlueprintPure, Category = "Boss|Light Shaft")
	float GetShaftRadius() const { return Radius; }

	UFUNCTION(BlueprintPure, Category = "Boss|Light Shaft")
	FVector GetFloorLocation() const { return Floor; }

	virtual void OnPoolActivate_Implementation() override;

	virtual void OnPoolDeactivate_Implementation() override;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Light Shaft")
	void OnTelegraphStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Light Shaft")
	void OnShaftOpened();

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Light Shaft")
	void OnShaftClosed();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpotLightComponent> Light;

	// Columna visible (cilindro). Solo se muestra si BeamMaterial está asignado (ej. material translúcido aditivo).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BeamMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UDecalComponent> GroundDecal;

	// Polvo que cae del techo durante el aviso.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNiagaraComponent> DustEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Light Shaft|Timing", meta = (ClampMin = "0"))
	float TelegraphDuration = 1.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Light Shaft|Timing", meta = (ClampMin = "0.01"))
	float LightRampDuration = 0.35f;

	// 0 = el hueco queda abierto para siempre.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Light Shaft|Timing", meta = (ClampMin = "0"))
	float Lifetime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Light Shaft|Timing", meta = (ClampMin = "0.01"))
	float CloseDuration = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Light Shaft|Light", meta = (ClampMin = "0"))
	float LightIntensity = 60000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Light Shaft|Light")
	FLinearColor LightColor = FLinearColor(1.f, 0.93f, 0.78f);

	// Fracción de la intensidad final durante el aviso.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Light Shaft|Light", meta = (ClampMin = "0", ClampMax = "1"))
	float TelegraphIntensityFraction = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Light Shaft|Light")
	TObjectPtr<UMaterialInterface> BeamMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Light Shaft|Light")
	TObjectPtr<UMaterialInterface> GroundDecalMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Light Shaft|Damage", meta = (ClampMin = "0.05"))
	float DamageTickInterval = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Light Shaft|Damage")
	ELightShaftDamageMode PlayerDamageMode = ELightShaftDamageMode::SunBurningEvent;

	// Daño por segundo en modo DirectDamage (y para entidades que no son el jugador).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Light Shaft|Damage", meta = (ClampMin = "0"))
	float DirectDamagePerSecond = 12.f;

	// La luz también quema a otras entidades (ej. los esqueletos invocados), nunca al boss.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Light Shaft|Damage")
	bool bAffectNonPlayerEntities = false;

	// Si algo tapa la luz entre el hueco y el objetivo, no quema (cubrirse detrás de un pilar funciona).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Light Shaft|Damage")
	bool bRequireLineOfSight = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Light Shaft|Damage")
	TEnumAsByte<ECollisionChannel> LineOfSightChannel = ECC_Visibility;

	// Daño de los escombros al abrirse el hueco (se esquiva saliendo del área durante el aviso, o con el dodge).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Light Shaft|Impact", meta = (ClampMin = "0"))
	float ImpactDamage = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Light Shaft|Impact")
	FGameplayTag ImpactDamageTypeTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Light Shaft|Impact")
	TObjectPtr<UNiagaraSystem> ImpactEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Light Shaft|Impact")
	TObjectPtr<USoundBase> ImpactSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Light Shaft|Impact")
	TObjectPtr<USoundBase> TelegraphSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Light Shaft|Impact")
	TSubclassOf<UCameraShakeBase> ImpactCameraShake;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Light Shaft|Impact", meta = (ClampMin = "0"))
	float ImpactCameraShakeRadius = 2500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Light Shaft|Debug")
	bool bDrawDebug = false;

private:
	enum class EShaftPhase : uint8
	{
		Inactive,
		Telegraph,
		Open,
		Closing
	};

	void BeginOpenPhase();
	void FinishClose();
	void DamageTick();
	void ApplyImpact();
	void LayoutComponents();
	void SetLightAlpha(float Alpha);
	void ClearExposure();
	void GatherCandidates(TArray<AActor*>& OutCandidates) const;
	bool HasLineOfSight(const AActor* Target) const;

	EShaftPhase Phase = EShaftPhase::Inactive;
	float PhaseElapsed = 0.f;

	FVector Top = FVector::ZeroVector;
	FVector Floor = FVector::ZeroVector;
	float Radius = 200.f;

	TWeakObjectPtr<AActor> Boss;
	TWeakObjectPtr<ABossCeilingBreakPoint> BreakPoint;
	FBossDamageParams DamageParams;

	TArray<TWeakObjectPtr<AActor>> ExposedActors;

	FTimerHandle PhaseTimer;
	FTimerHandle DamageTimer;
};
