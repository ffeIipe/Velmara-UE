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
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBossFullArenaCollapsed);

UCLASS(ClassGroup = (Boss), meta = (BlueprintSpawnableComponent))
class TESISUE_API UBossCeilingBreakComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBossCeilingBreakComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling")
	TArray<FBossCeilingStage> Stages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Animation")
	TObjectPtr<UAnimMontage> BreakMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Animation", meta = (ClampMin = "0", ClampMax = "1"))
	float BreakEventTime = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Animation", meta = (ClampMin = "0.1"))
	float BreakPlayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Animation", meta = (ClampMin = "0"))
	float RecoveryAfterBreak = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Shockwave", meta = (ClampMin = "0"))
	float ShockwaveRadius = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Shockwave", meta = (ClampMin = "0"))
	float ShockwaveDamage = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling")
	bool bInterruptAttacks = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Shafts")
	TSubclassOf<ABossLightShaft> LightShaftClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Shafts", meta = (ClampMin = "50"))
	float ShaftRadius = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Shafts", meta = (ClampMin = "0"))
	float ShaftStagger = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Shafts", meta = (ClampMin = "0"))
	float MinShaftSeparation = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Shafts", meta = (ClampMin = "0"))
	float MinDistanceFromBoss = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Shafts", meta = (ClampMin = "0"))
	int32 ShaftPoolPrewarm = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Shafts")
	bool bCloseShaftsOnBossDeath = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Placement")
	TArray<TObjectPtr<ABossCeilingBreakPoint>> BreakPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Placement")
	bool bAutoCollectBreakPoints = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Placement", meta = (ClampMin = "0"))
	float ArenaRadius = 1800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Placement")
	TObjectPtr<AActor> ArenaCenter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling|Placement", meta = (ClampMin = "100"))
	float MaxCeilingTraceHeight = 4000.f;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss Ceiling|Full Collapse", meta = (ClampMin = "0.0"))
	float FullCollapseShaftRadius = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss Ceiling|Full Collapse", meta = (ClampMin = "0.0"))
	float FullCollapseBreakPointStagger = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss Ceiling|Full Collapse")
	bool bClosePreviousShaftsOnFullCollapse = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss Ceiling|Full Collapse")
	TArray<TObjectPtr<AActor>> ExtraCeilingPiecesToHideOnFullCollapse;

	UPROPERTY(BlueprintAssignable, Category = "Boss|Ceiling")
	FOnBossStageChanged OnStageChanged;

	UPROPERTY(BlueprintAssignable, Category = "Boss Ceiling|Events")
	FOnBossFullArenaCollapsed OnFullArenaCollapsed;

	UFUNCTION(BlueprintCallable, Category = "Boss|Ceiling")
	void EvaluateHealth(float HealthPercent);

	UFUNCTION(BlueprintPure, Category = "Boss|Ceiling")
	bool HasPendingStage() const { return PendingStages.Num() > 0 && !bExecutingStage && !bBossDead; }

	UFUNCTION(BlueprintPure, Category = "Boss|Ceiling")
	bool IsExecutingStage() const { return bExecutingStage; }

	UFUNCTION(BlueprintCallable, Category = "Boss|Ceiling")
	bool ExecutePendingStage(AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "Boss|Ceiling")
	void BreakCeilingNow(int32 StageIndex, AActor* Target);

	UFUNCTION(BlueprintPure, Category = "Boss|Ceiling")
	int32 GetCompletedStages() const { return CompletedStages; }

	UFUNCTION(BlueprintPure, Category = "Boss|Ceiling")
	FVector GetArenaOrigin() const;

	UFUNCTION(BlueprintCallable, Category = "Boss|Ceiling")
	void CloseAllShafts();

	void OnBossDied();

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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
	void ExecuteFullArenaCollapse();
	void BreakPointDeferred(TWeakObjectPtr<ABossCeilingBreakPoint> Point);
	void ApplyShockwave() const;

	void BuildShaftRequests(int32 StageIndex, const AActor* Target, TArray<FShaftRequest>& OutRequests);
	bool ResolveShaftFromGround(const FVector& GroundPoint, const AActor* IgnoredActor, FShaftRequest& OutRequest) const;
	bool ResolveShaftFromCeiling(const FVector& CeilingPoint, FShaftRequest& OutRequest) const;
	bool FindRandomGroundPoint(const TArray<FShaftRequest>& Existing, FVector& OutPoint) const;
	bool IsFarFromExisting(const FVector& Point, const TArray<FShaftRequest>& Existing) const;
	FVector ResolveArenaCenter() const;
	FBossDamageParams GetShaftDamageParams() const;

	TArray<int32> PendingStages;
	TArray<TWeakObjectPtr<ABossLightShaft>> ActiveShafts;
	TArray<FTimerHandle> ShaftTimers;
	TArray<FTimerHandle> CollapseTimers;

	FVector SpawnOrigin = FVector::ZeroVector;
	int32 NextStageToQueue = 0;
	int32 CompletedStages = 0;
	int32 ExecutingStageIndex = INDEX_NONE;
	bool bStageEventFired = false;
	bool bExecutingStage = false;
	bool bBossDead = false;
};
