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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnBossAttackTelegraph, FName, AttackName, FVector, TelegraphLocation, float, TelegraphRadius, bool, bIsProjectile);

DECLARE_DELEGATE_OneParam(FBossActionFinished, bool);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnBossActionFinishedNative, bool);

UCLASS(ClassGroup = (Boss), meta = (BlueprintSpawnableComponent))
class TESISUE_API UBossCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBossCombatComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Attacks", meta = (TitleProperty = "AttackName"))
	TArray<FBossAttackDefinition> Attacks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Damage")
	FBossDamageParams DamageParams;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Damage", meta = (ClampMin = "0"))
	float DefaultNotifyDamage = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Stages", meta = (ClampMin = "0"))
	float DamageBonusPerStage = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Stages", meta = (ClampMin = "0"))
	float PlayRateBonusPerStage = 0.06f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Stages", meta = (ClampMin = "0", ClampMax = "0.9"))
	float RecoveryReductionPerStage = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Weapon", meta = (ClampMin = "1"))
	float WeaponTraceRadius = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Weapon", meta = (ClampMin = "1", ClampMax = "12"))
	int32 WeaponTraceSamples = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Weapon", meta = (ClampMin = "0"))
	float MinWeaponSweepSpeed = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Weapon")
	FName WeaponBaseSocket = TEXT("WeaponBase");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Weapon")
	FName WeaponTipSocket = TEXT("WeaponTip");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Weapon")
	FName FallbackWeaponSocket = TEXT("hand_r");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Weapon")
	TArray<TEnumAsByte<EObjectTypeQuery>> TargetObjectTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Projectile")
	TSubclassOf<ABossProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Projectile")
	FVector ProjectileSpawnOffset = FVector(120.f, 0.f, 20.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Projectile", meta = (ClampMin = "1"))
	float ProjectileSpeed = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Projectile", meta = (ClampMin = "0.1"))
	float ProjectileLifetime = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Warping", meta = (ClampMin = "0"))
	float WarpStopDistance = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Projectile", meta = (ClampMin = "0"))
	int32 ProjectilePoolPrewarm = 6;

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

	UPROPERTY(BlueprintAssignable, Category = "Boss|Events")
	FOnBossAttackTarget OnAttackEvaded;

	UPROPERTY(BlueprintAssignable, Category = "Boss Combat|Events")
	FOnBossAttackTelegraph OnAttackTelegraphStarted;

	FOnBossActionFinishedNative OnActionFinished;

	UFUNCTION(BlueprintCallable, Category = "Boss|Combat")
	void SetWeaponComponent(UPrimitiveComponent* InWeaponComponent);

	UFUNCTION(BlueprintCallable, Category = "Boss|Combat")
	int32 SelectAttack(const AActor* Target, int32 Stage) const;

	UFUNCTION(BlueprintCallable, Category = "Boss|Combat")
	bool StartAttack(int32 AttackIndex, AActor* Target);

	bool PlayActionMontage(UAnimMontage* Montage, float PlayRate, float EventTime, FSimpleDelegate OnEvent,
		FBossActionFinished OnFinished, AActor* TrackTarget, float RecoveryAfter = 0.3f);

	UFUNCTION(BlueprintCallable, Category = "Boss|Combat")
	void CancelCurrentAction();

	UFUNCTION(BlueprintPure, Category = "Boss|Combat")
	bool IsBusy() const { return CurrentAction.bActive; }

	UFUNCTION(BlueprintPure, Category = "Boss|Combat")
	bool IsAttacking() const { return CurrentAction.bActive && CurrentAction.AttackIndex != INDEX_NONE; }

	UFUNCTION(BlueprintPure, Category = "Boss|Combat")
	float GetLastRecoveryTime() const { return LastRecoveryTime; }

	UFUNCTION(BlueprintCallable, Category = "Boss|Stages")
	void SetStage(int32 NewStage) { CurrentStage = FMath::Max(0, NewStage); }

	UFUNCTION(BlueprintPure, Category = "Boss|Stages")
	int32 GetStage() const { return CurrentStage; }

	UFUNCTION(BlueprintPure, Category = "Boss|Combat")
	float GetMaxAttackDistance() const;

	void RotateOwnerTowards(const FVector& Location, float DeltaTime, float DegreesPerSecond) const;

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
