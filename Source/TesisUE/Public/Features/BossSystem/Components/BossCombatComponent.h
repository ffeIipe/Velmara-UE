#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/HitResult.h"
#include "Features/BossSystem/Data/BossTypes.h"
#include "BossCombatComponent.generated.h"

class ABossProjectile;
class ACharacter;
class UAnimInstance;
class UAnimMontage;
class UDecalComponent;
class UMaterialInterface;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBossAttackStarted, FName, AttackName, float, TimeUntilFirstHit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBossAttackEnded, FName, AttackName, bool, bInterrupted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBossAttackTarget, AActor*, Target, FName, AttackName);

DECLARE_DELEGATE_OneParam(FBossActionFinished, bool /*bInterrupted*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnBossActionFinishedNative, bool /*bInterrupted*/);

/*
 * Ejecuta las acciones del boss sobre montajes: ataques con ventanas de golpe (barrido del arma, esfera o proyectiles)
 * y montajes genéricos (invocar, romper el techo, esquivar) con un evento en un tiempo normalizado.
 * Las ventanas salen de la tabla de ataques, salvo que el montaje tenga AnimNotifyStates "Boss Hit Window".
 */
UCLASS(ClassGroup = (Boss), meta = (BlueprintSpawnableComponent))
class TESISUE_API UBossCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBossCombatComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Attacks", meta = (TitleProperty = "AttackName"))
	TArray<FBossAttackDefinition> Attacks;

	// GE, tag SetByCaller, inmunidades y cue para todos los golpes. Damage se reemplaza por el de cada ataque.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Damage")
	FBossDamageParams DamageParams;

	// Daño usado por ventanas de AnimNotify en montajes que no son ataques de la tabla.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Damage", meta = (ClampMin = "0"))
	float DefaultNotifyDamage = 15.f;

	// Bonus por etapa (0.15 = +15% por cada techo roto).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Stages", meta = (ClampMin = "0"))
	float DamageBonusPerStage = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Stages", meta = (ClampMin = "0"))
	float PlayRateBonusPerStage = 0.05f;

	// Reducción del tiempo de recuperación por etapa (0.12 = -12% por etapa).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Stages", meta = (ClampMin = "0", ClampMax = "0.9"))
	float RecoveryReductionPerStage = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Weapon", meta = (ClampMin = "1"))
	float WeaponTraceRadius = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Weapon", meta = (ClampMin = "1", ClampMax = "12"))
	int32 WeaponTraceSamples = 4;

	// Velocidad mínima del arma para que el barrido cuente. Evita daño cuando el arma está quieta o en la preparación.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Weapon", meta = (ClampMin = "0"))
	float MinWeaponSweepSpeed = 350.f;

	// Sockets opcionales del static mesh del arma. Sin ellos se usa el eje más largo de su bounding box.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Weapon")
	FName WeaponBaseSocket = TEXT("WeaponBase");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Weapon")
	FName WeaponTipSocket = TEXT("WeaponTip");

	// Si no hay arma asignada, se barre una esfera en este socket del mesh del boss.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Weapon")
	FName FallbackWeaponSocket = TEXT("hand_r");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Weapon")
	TArray<TEnumAsByte<EObjectTypeQuery>> TargetObjectTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Projectile")
	TSubclassOf<ABossProjectile> ProjectileClass;

	// Offset local respecto al boss desde donde salen los proyectiles.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Projectile")
	FVector ProjectileSpawnOffset = FVector(120.f, 0.f, 20.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Projectile", meta = (ClampMin = "1"))
	float ProjectileSpeed = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Projectile", meta = (ClampMin = "0.1"))
	float ProjectileLifetime = 3.f;

	// Motion Warping: distancia al objetivo donde termina el acercamiento del ataque.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Warping", meta = (ClampMin = "0"))
	float WarpStopDistance = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Projectile", meta = (ClampMin = "0"))
	int32 ProjectilePoolPrewarm = 6;

	// Material de decal para marcar en el piso los ataques con bShowGroundTelegraph.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Telegraph")
	TObjectPtr<UMaterialInterface> GroundTelegraphMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Debug")
	bool bDrawDebug = false;

	UPROPERTY(BlueprintAssignable, Category = "Boss|Events")
	FOnBossAttackStarted OnAttackStarted;

	UPROPERTY(BlueprintAssignable, Category = "Boss|Events")
	FOnBossAttackEnded OnAttackEnded;

	UPROPERTY(BlueprintAssignable, Category = "Boss|Events")
	FOnBossAttackTarget OnTargetHit;

	// El objetivo esquivó el golpe (tenía un tag de inmunidad, ej. State.Dodging).
	UPROPERTY(BlueprintAssignable, Category = "Boss|Events")
	FOnBossAttackTarget OnAttackEvaded;

	// Se dispara al terminar cualquier acción (ataque o montaje genérico).
	FOnBossActionFinishedNative OnActionFinished;

	UFUNCTION(BlueprintCallable, Category = "Boss|Combat")
	void SetWeaponComponent(UPrimitiveComponent* InWeaponComponent);

	// Ataque válido por distancia, ángulo, etapa y cooldown (elección ponderada). INDEX_NONE si no hay.
	UFUNCTION(BlueprintCallable, Category = "Boss|Combat")
	int32 SelectAttack(const AActor* Target, int32 Stage) const;

	UFUNCTION(BlueprintCallable, Category = "Boss|Combat")
	bool StartAttack(int32 AttackIndex, AActor* Target);

	// Montaje genérico. OnEvent se dispara en EventTime (normalizado, < 0 = sin evento) o con un AnimNotify "Boss Action Event".
	bool PlayActionMontage(UAnimMontage* Montage, float PlayRate, float EventTime, FSimpleDelegate OnEvent,
		FBossActionFinished OnFinished, AActor* TrackTarget, float RecoveryAfter = 0.3f);

	UFUNCTION(BlueprintCallable, Category = "Boss|Combat")
	void CancelCurrentAction();

	UFUNCTION(BlueprintPure, Category = "Boss|Combat")
	bool IsBusy() const { return CurrentAction.bActive; }

	UFUNCTION(BlueprintPure, Category = "Boss|Combat")
	bool IsAttacking() const { return CurrentAction.bActive && CurrentAction.AttackIndex != INDEX_NONE; }

	// Recuperación que dejó la última acción terminada.
	UFUNCTION(BlueprintPure, Category = "Boss|Combat")
	float GetLastRecoveryTime() const { return LastRecoveryTime; }

	UFUNCTION(BlueprintCallable, Category = "Boss|Stages")
	void SetStage(int32 NewStage) { CurrentStage = FMath::Max(0, NewStage); }

	UFUNCTION(BlueprintPure, Category = "Boss|Stages")
	int32 GetStage() const { return CurrentStage; }

	UFUNCTION(BlueprintPure, Category = "Boss|Combat")
	float GetMaxAttackDistance() const;

	void RotateOwnerTowards(const FVector& Location, float DeltaTime, float DegreesPerSecond) const;

	// Llamados por AnimNotifyState_BossHitWindow / AnimNotify_BossActionEvent.
	void BeginNotifyWindow(const UObject* NotifyKey, const FBossHitWindow& Window);
	void EndNotifyWindow(const UObject* NotifyKey);
	void TriggerActionEventFromNotify();

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	struct FActiveHitWindow
	{
		FBossHitWindow Params;
		const UObject* NotifyKey = nullptr;
		int32 TimedIndex = INDEX_NONE;
		TMap<TWeakObjectPtr<AActor>, double> LastHitTimes;
	};

	struct FBossActionRuntime
	{
		int32 Id = 0;
		bool bActive = false;
		int32 AttackIndex = INDEX_NONE;
		TWeakObjectPtr<UAnimMontage> Montage;
		TWeakObjectPtr<AActor> Target;
		double StartTime = 0.0;
		float FallbackDuration = 0.f;
		float EventTime = -1.f;
		bool bEventFired = false;
		bool bUseNotifyEvent = false;
		bool bUseNotifyWindows = false;
		float TrackingEndTime = 0.f;
		float TrackingSpeed = 0.f;
		bool bWarping = false;
		bool bWarpTranslation = false;
		float RecoveryAfter = 0.f;
		FSimpleDelegate OnEvent;
		FBossActionFinished OnFinished;
		TArray<uint8> TimedWindowStates;
		TArray<FActiveHitWindow> Windows;
	};

	bool BeginAction(UAnimMontage* Montage, float PlayRate, AActor* Target, bool bAllowWithoutMontage);
	void FinishAction(bool bInterrupted);
	void HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted, int32 ActionId);
	void FireActionEvent();

	float GetActionProgress() const;
	void UpdateTimedWindows(float Progress);
	void ProcessActiveWindows(float DeltaTime);
	void TryHitTarget(FActiveHitWindow& Window, AActor* Target, const FHitResult& Hit);
	void FireProjectiles(const FBossHitWindow& Window);
	void UpdateWarpTarget(const AActor* Target) const;

	bool GetWeaponSegment(FVector& OutStart, FVector& OutEnd) const;
	void BuildWeaponSamples(TArray<FVector>& OutSamples) const;
	float GetCurrentBaseDamage() const;
	float GetStageDamageMultiplier() const { return 1.f + DamageBonusPerStage * CurrentStage; }
	FName GetCurrentAttackName() const;

	UAnimInstance* GetAnimInstance() const;

	UPROPERTY(Transient)
	TObjectPtr<ACharacter> OwnerCharacter;

	UPROPERTY(Transient)
	TWeakObjectPtr<UPrimitiveComponent> WeaponComponent;

	UPROPERTY(Transient)
	TObjectPtr<UDecalComponent> ActiveTelegraphDecal;

	FBossActionRuntime CurrentAction;
	int32 NextActionId = 1;

	TArray<double> AttackReadyTimes;

	TArray<FVector> PrevWeaponSamples;
	bool bHasPrevWeaponSamples = false;

	int32 CurrentStage = 0;
	float LastRecoveryTime = 0.f;
	bool bWarnedMissingProjectileClass = false;
};
