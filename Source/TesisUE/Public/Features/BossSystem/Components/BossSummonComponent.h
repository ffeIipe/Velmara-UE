#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BossSummonComponent.generated.h"

class AEntity;
class UAnimMontage;
class UNiagaraSystem;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBossMinionSpawned, AActor*, Minion);

UENUM(BlueprintType)
enum class EBossMinionCleanup : uint8
{
	Kill,
	ReturnToPool,
	Keep
};

UCLASS(ClassGroup = (Boss), meta = (BlueprintSpawnableComponent))
class TESISUE_API UBossSummonComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBossSummonComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Summon")
	TArray<TSubclassOf<AActor>> MinionClasses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Summon", meta = (ClampMin = "1"))
	int32 MinionsPerSummon = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Summon", meta = (ClampMin = "1"))
	int32 MaxAliveMinions = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Summon", meta = (ClampMin = "0"))
	float SummonCooldown = 22.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Summon", meta = (ClampMin = "0"))
	float FirstSummonDelay = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Summon|Placement", meta = (ClampMin = "0"))
	float MinSpawnRadius = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Summon|Placement", meta = (ClampMin = "0"))
	float MaxSpawnRadius = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Summon|Pool", meta = (ClampMin = "0"))
	int32 PrewarmPerClass = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Summon|Animation")
	TObjectPtr<UAnimMontage> SummonMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Summon|Animation", meta = (ClampMin = "0", ClampMax = "1"))
	float SummonEventTime = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Summon|Animation", meta = (ClampMin = "0.1"))
	float SummonPlayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Summon|Animation", meta = (ClampMin = "0"))
	float RecoveryAfterSummon = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Summon|Feedback", meta = (ClampMin = "0"))
	float SpawnDelay = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Summon|Feedback", meta = (ClampMin = "0"))
	float SpawnStagger = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Summon|Feedback")
	TObjectPtr<UNiagaraSystem> SpawnEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Summon|Feedback")
	TObjectPtr<USoundBase> SpawnSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Summon")
	EBossMinionCleanup CleanupOnBossDeath = EBossMinionCleanup::Kill;

	UPROPERTY(BlueprintAssignable, Category = "Boss|Summon")
	FOnBossMinionSpawned OnMinionSpawned;

	UFUNCTION(BlueprintPure, Category = "Boss|Summon")
	bool CanSummon() const;

	UFUNCTION(BlueprintCallable, Category = "Boss|Summon")
	bool StartSummon(AActor* Target, int32 CountOverride = 0);

	UFUNCTION(BlueprintCallable, Category = "Boss|Summon")
	int32 SpawnMinionsNow(AActor* Target, int32 Count = 0);

	UFUNCTION(BlueprintPure, Category = "Boss|Summon")
	int32 GetAliveMinionCount() const;

	UFUNCTION(BlueprintPure, Category = "Boss|Summon")
	TArray<AActor*> GetAliveMinions() const;

	UFUNCTION(BlueprintCallable, Category = "Boss|Summon")
	void CleanupMinions(EBossMinionCleanup Mode);

	void NotifyEncounterStarted();

	void OnBossDied();

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleMinionDead(AEntity* DeadEntity);

	void HandleSummonEvent(TWeakObjectPtr<AActor> Target, int32 Count);

	void SpawnMinion(TSubclassOf<AActor> MinionClass, FVector Location, TWeakObjectPtr<AActor> Target);

	bool FindSpawnLocation(const FVector& Center, const TArray<FVector>& UsedLocations, FVector& OutLocation) const;

	bool IsMinionAlive(const AActor* Minion) const;

	void PruneMinions();

	TArray<TWeakObjectPtr<AActor>> AliveMinions;

	TArray<FTimerHandle> PendingSpawnTimers;

	int32 PendingSpawns = 0;

	double NextSummonTime = 0.0;

	bool bEncounterStarted = false;

	bool bBossDead = false;
};
