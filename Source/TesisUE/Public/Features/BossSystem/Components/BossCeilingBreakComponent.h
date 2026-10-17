#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Features/BossSystem/Data/BossTypes.h"
#include "BossCeilingBreakComponent.generated.h"

class ABossCeilingBreakPoint;
class ABossLightShaft;
class UAnimMontage;
class UCameraShakeBase;
class UNiagaraSystem;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBossStageChanged, int32, NewStage);

/*
 * Habilidad por etapas: al cruzar cada umbral de vida el boss golpea, el techo se rompe y se abren huecos de luz
 * (ABossLightShaft, desde el pool) que queman a Velmara. Cada etapa también sube la agresividad del combate.
 */
UCLASS(ClassGroup = (Boss), meta = (BlueprintSpawnableComponent))
class TESISUE_API UBossCeilingBreakComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBossCeilingBreakComponent();

	// Ordenadas de mayor a menor umbral de vida.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling")
	TArray<FBossCeilingStage> Stages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Animation")
	TObjectPtr<UAnimMontage> BreakMontage;

	// Momento normalizado del golpe que rompe el techo (o AnimNotify "Boss Action Event").
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Animation", meta = (ClampMin = "0", ClampMax = "1"))
	float BreakEventTime = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Animation", meta = (ClampMin = "0.1"))
	float BreakPlayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Animation", meta = (ClampMin = "0"))
	float RecoveryAfterBreak = 1.f;

	// Onda de choque del golpe alrededor del boss (se esquiva alejándose durante el salto o con el dodge).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Shockwave", meta = (ClampMin = "0"))
	float ShockwaveRadius = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Shockwave", meta = (ClampMin = "0"))
	float ShockwaveDamage = 15.f;

	// Al cruzar un umbral el boss corta el ataque que esté haciendo para romper el techo.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling")
	bool bInterruptAttacks = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Shafts")
	TSubclassOf<ABossLightShaft> LightShaftClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Shafts", meta = (ClampMin = "50"))
	float ShaftRadius = 230.f;

	// Segundos entre la apertura de cada hueco de una misma etapa.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Shafts", meta = (ClampMin = "0"))
	float ShaftStagger = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Shafts", meta = (ClampMin = "0"))
	float MinShaftSeparation = 500.f;

	// Los huecos aleatorios no se abren encima del boss.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Shafts", meta = (ClampMin = "0"))
	float MinDistanceFromBoss = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Shafts", meta = (ClampMin = "0"))
	int32 ShaftPoolPrewarm = 4;

	// Huecos deshabilitados al morir el boss. Si es false quedan como parte del escenario (y siguen quemando).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Shafts")
	bool bCloseShaftsOnBossDeath = false;

	// Puntos colocados en el nivel. Vacío + bAutoCollectBreakPoints = usa todos los ABossCeilingBreakPoint del nivel.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Placement")
	TArray<TObjectPtr<ABossCeilingBreakPoint>> BreakPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Placement")
	bool bAutoCollectBreakPoints = true;

	// Si faltan puntos, se generan huecos al azar en el navmesh dentro de este radio alrededor del centro de la arena.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Placement", meta = (ClampMin = "0"))
	float ArenaRadius = 1800.f;

	// Vacío = la posición inicial del boss.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Placement")
	TObjectPtr<AActor> ArenaCenter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Placement", meta = (ClampMin = "100"))
	float MaxCeilingTraceHeight = 4000.f;

	// Altura del hueco cuando no se encuentra techo (cielo abierto).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Placement", meta = (ClampMin = "100"))
	float OpenSkyShaftHeight = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Placement")
	TEnumAsByte<ECollisionChannel> CeilingTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Feedback")
	TSubclassOf<UCameraShakeBase> BreakCameraShake;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Feedback", meta = (ClampMin = "0"))
	float BreakCameraShakeRadius = 5000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Feedback")
	TObjectPtr<USoundBase> BreakSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Feedback")
	TObjectPtr<UNiagaraSystem> BreakEffect;

	UPROPERTY(BlueprintAssignable, Category = "Boss|Ceiling")
	FOnBossStageChanged OnStageChanged;

	// Encola las etapas cuyo umbral se cruzó (un golpe grande puede encolar varias).
	UFUNCTION(BlueprintCallable, Category = "Boss|Ceiling")
	void EvaluateHealth(float HealthPercent);

	UFUNCTION(BlueprintPure, Category = "Boss|Ceiling")
	bool HasPendingStage() const { return PendingStages.Num() > 0 && !bExecutingStage && !bBossDead; }

	UFUNCTION(BlueprintPure, Category = "Boss|Ceiling")
	bool IsExecutingStage() const { return bExecutingStage; }

	// Reproduce el montaje y, en su evento, rompe el techo de la próxima etapa pendiente.
	UFUNCTION(BlueprintCallable, Category = "Boss|Ceiling")
	bool ExecutePendingStage(AActor* Target);

	// Rompe el techo sin montaje (scripting / debug).
	UFUNCTION(BlueprintCallable, Category = "Boss|Ceiling")
	void BreakCeilingNow(int32 StageIndex, AActor* Target);

	// Cantidad de etapas ya ejecutadas.
	UFUNCTION(BlueprintPure, Category = "Boss|Ceiling")
	int32 GetCompletedStages() const { return CompletedStages; }

	UFUNCTION(BlueprintPure, Category = "Boss|Ceiling")
	FVector GetArenaOrigin() const;

	UFUNCTION(BlueprintCallable, Category = "Boss|Ceiling")
	void CloseAllShafts();

	void OnBossDied();

protected:
	virtual void BeginPlay() override;

private:
	struct FShaftRequest
	{
		FVector Top = FVector::ZeroVector;
		FVector Floor = FVector::ZeroVector;
		float Radius = 0.f;
		TWeakObjectPtr<ABossCeilingBreakPoint> Point;
	};

	void HandleBreakEvent(int32 StageIndex, TWeakObjectPtr<AActor> Target);
	void HandleBreakFinished(bool bInterrupted);
	void OpenShaft(FShaftRequest Request);
	void ApplyShockwave() const;

	void BuildShaftRequests(int32 StageIndex, const AActor* Target, TArray<FShaftRequest>& OutRequests);
	bool ResolveShaftFromGround(const FVector& GroundPoint, const AActor* IgnoredActor, FShaftRequest& OutRequest) const;
	bool ResolveShaftFromCeiling(const FVector& CeilingPoint, FShaftRequest& OutRequest) const;
	bool FindRandomGroundPoint(const TArray<FShaftRequest>& Existing, FVector& OutPoint) const;
	bool IsFarFromExisting(const FVector& Point, const TArray<FShaftRequest>& Existing) const;
	FBossDamageParams GetShaftDamageParams() const;

	TArray<int32> PendingStages;
	TArray<TWeakObjectPtr<ABossLightShaft>> ActiveShafts;
	TArray<FTimerHandle> ShaftTimers;

	FVector SpawnOrigin = FVector::ZeroVector;
	int32 NextStageToQueue = 0;
	int32 CompletedStages = 0;
	int32 ExecutingStageIndex = INDEX_NONE;
	bool bStageEventFired = false;
	bool bExecutingStage = false;
	bool bBossDead = false;
};
