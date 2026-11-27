#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/TimerHandle.h"
#include "GameplayEffectTypes.h"
#include "Features/BossSystem/Data/BossTypes.h"
#include "Features/SolarBoss/Data/SolarBossTypes.h"
#include "SolarBossBrainComponent.generated.h"

class ABossCeilingBreakPoint;
class ABossLightShaft;
class AEntity;
class UAbilitySystemComponent;
class UPaladinBossHealthBar;
class USolarBossConfigDataAsset;
struct FGameplayEffectSpec;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSolarBossModeChanged, ESolarBossMode, NewMode, ESolarBossMode, OldMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSolarBossCycleTriggered, int32, CycleIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSolarBossHitByPossessedMinion, AActor*, Minion, float, Damage);

struct FSolarBossShaftRequest
{
	FVector Top = FVector::ZeroVector;
	FVector Floor = FVector::ZeroVector;
	float Radius = 0.f;
	TWeakObjectPtr<ABossCeilingBreakPoint> Point;
};

UCLASS(ClassGroup = (Boss), meta = (BlueprintSpawnableComponent))
class TESISUE_API USolarBossBrainComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USolarBossBrainComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Solar Boss")
	TObjectPtr<USolarBossConfigDataAsset> Config;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Solar Boss")
	TArray<TObjectPtr<AActor>> EncounterListeners;

	UPROPERTY(BlueprintAssignable, Category = "Solar Boss")
	FOnSolarBossModeChanged OnModeChanged;

	UPROPERTY(BlueprintAssignable, Category = "Solar Boss")
	FOnSolarBossCycleTriggered OnCycleTriggered;

	UPROPERTY(BlueprintAssignable, Category = "Solar Boss")
	FOnSolarBossHitByPossessedMinion OnHitByPossessedMinion;

	static bool IsActorAlive(const AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "Solar Boss")
	void StartEncounter();

	UFUNCTION(BlueprintCallable, Category = "Solar Boss")
	void SetTemporarySuperArmor(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Solar Boss")
	ESolarBossMode GetMode() const { return Mode; }

	UFUNCTION(BlueprintPure, Category = "Solar Boss")
	int32 GetAliveMinionCount() const;

	bool IsReady() const { return bInitialized && !bDead; }
	bool HasEncounterStarted() const { return bEncounterStarted; }
	bool HasPendingCycleBreak() const { return PendingCycles.Num() > 0 && bCycleBreakArmed; }
	bool NeedsDefensiveDodge() const { return bWantsDefensiveDodge; }
	bool NeedsEmergencySummon() const { return bNeedsEmergencySummon; }
	bool IsReactiveDodgeReady() const;
	FVector GetArenaOrigin() const { return ArenaOrigin; }

	void EvaluateState(AActor* Target);

	void GrantSuperArmor(float Duration);
	void NotifyDodge(ESolarBossDodgeMode DodgeMode);
	bool IsInHitReact(const UAnimMontage* IgnoredMontage = nullptr) const;
	void CancelHitReact();

	bool BeginCycleBreak();
	void ExecuteCeilingBreak(AActor* Target);
	void SummonCycleMinions(AActor* Target);
	void CompleteCycleBreak();
	void SummonEmergencyMinions(AActor* Target);

	int32 SelectMeleeAttack(const AActor* Target) const;
	void CommitMeleeAttack(int32 AttackIndex);
	bool TryMeleeSphereHit(int32 AttackIndex);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void InitializeBrain();
	void HandleEffectApplied(UAbilitySystemComponent* Source, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle);
	void HandleHealthChanged(const FOnAttributeChangeData& Data);
	void ProcessPendingHit();
	void HandleDeath();
	void SignalEncounterListeners(bool bActive) const;
	void SetMode(ESolarBossMode NewMode);
	void ApplyWalkSpeed();
	void TickAfterWalkSpeedWritersAndBeforeMovement();
	void UpdateModeFromMinions();
	void RefreshSuperArmor();
	void ExpireSuperArmor();
	void UpdateHealthBar() const;
	void HideHealthBar();
	void PruneMinions();
	bool IsMinionAlive(const AActor* Minion) const;
	bool CanActInWorld() const;
	const FSolarBossCycleConfig* GetCurrentCycle() const;
	void SummonMinions(AActor* Target, int32 Count);
	void SpawnMinion(TSubclassOf<AActor> MinionClass, FVector Location, TWeakObjectPtr<AActor> Target);
	bool FindSpawnLocation(const TArray<FVector>& UsedLocations, FVector& OutLocation) const;
	bool TryFindSpawnLocation(const FVector& Center, float MinRadius, const TArray<FVector>& UsedLocations, FVector& OutLocation) const;
	void CleanupMinions();
	void OpenPartialShafts(int32 CycleIndex, AActor* Target);
	void CollapseWholeCeiling();
	void OpenShaft(FSolarBossShaftRequest Request);
	ABossLightShaft* AcquireShaft(const FVector& Location) const;
	void TrackShaft(ABossLightShaft* Shaft);
	void CloseAllShafts();
	void BreakPointDeferred(TWeakObjectPtr<ABossCeilingBreakPoint> Point);
	void ApplyShockwave() const;
	bool ResolveShaftFromGround(const FVector& GroundPoint, const AActor* IgnoredActor, FSolarBossShaftRequest& OutRequest) const;
	bool ResolveShaftFromCeiling(const FVector& CeilingPoint, FSolarBossShaftRequest& OutRequest) const;
	bool IsFarFromShafts(const FVector& Point, const TArray<FSolarBossShaftRequest>& Requests) const;
	FBossDamageParams MakeDamageParams(float Damage, const FGameplayTag& DamageTypeTag) const;
	bool DamageSphere(const FVector& Center, float Radius, const FBossDamageParams& Params, bool bSingleTarget) const;

	UPROPERTY(Transient)
	TObjectPtr<AEntity> OwnerEntity;

	UPROPERTY(Transient)
	TObjectPtr<UPaladinBossHealthBar> HealthBarWidget;

	ESolarBossMode Mode = ESolarBossMode::Inactive;
	FVector ArenaOrigin = FVector::ZeroVector;

	TArray<TWeakObjectPtr<ABossCeilingBreakPoint>> BreakPoints;
	TArray<TWeakObjectPtr<ABossLightShaft>> ActiveShafts;
	TArray<TWeakObjectPtr<AActor>> Minions;
	TArray<int32> PendingCycles;
	TArray<double> AttackReadyTimes;
	TArray<double> RecentHitTimes;
	FGameplayTagContainer BlockedHitReactTags;
	TWeakObjectPtr<AActor> PendingHitAttacker;
	FTimerHandle SuperArmorTimer;

	float PendingHitLoss = 0.f;
	double NextDefensiveDodgeTime = 0.0;
	double NextReactiveDodgeTime = 0.0;
	double NextEmergencySummonTime = 0.0;
	double StarvationStartTime = -1.0;

	int32 NextCycleIndex = 0;
	int32 CompletedCycles = 0;
	int32 ActiveCycleIndex = INDEX_NONE;
	int32 PendingSpawns = 0;

	bool bInitialized = false;
	bool bDead = false;
	bool bEncounterStarted = false;
	bool bFullCollapseActive = false;
	bool bCycleBreakArmed = false;
	bool bCeilingBroken = false;
	bool bCycleMinionsSummoned = false;
	bool bWantsDefensiveDodge = false;
	bool bNeedsEmergencySummon = false;
	bool bApplyingBonusDamage = false;
	bool bHitScheduled = false;
	bool bManualSuperArmor = false;
	bool bTimedSuperArmor = false;
	bool bAddedDodgeTag = false;
};
