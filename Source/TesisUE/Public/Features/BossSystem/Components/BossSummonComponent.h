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
	// Les aplica MortalDamage: hacen su muerte normal y vuelven solos al pool.
	Kill,
	ReturnToPool,
	Keep
};

/*
 * Invoca enemigos alrededor del boss usando el UActorPoolSubsystem. Los enemigos que implementan PoolableActor
 * (ej. CBP_Enemy) vuelven al pool al morir y se reutilizan en la próxima invocación.
 */
UCLASS(ClassGroup = (Boss), meta = (BlueprintSpawnableComponent))
class TESISUE_API UBossSummonComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBossSummonComponent();

	// Enemigos a invocar: por cada invocado se elige una clase al azar.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Summon")
	TArray<TSubclassOf<AActor>> MinionClasses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Summon", meta = (ClampMin = "1"))
	int32 MinionsPerSummon = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Summon", meta = (ClampMin = "1"))
	int32 MaxAliveMinions = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Summon", meta = (ClampMin = "0"))
	float SummonCooldown = 22.f;

	// Segundos desde que arranca la pelea hasta la primera invocación posible.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Summon", meta = (ClampMin = "0"))
	float FirstSummonDelay = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Summon|Placement", meta = (ClampMin = "0"))
	float MinSpawnRadius = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Summon|Placement", meta = (ClampMin = "0"))
	float MaxSpawnRadius = 900.f;

	// Instancias por clase que se crean en el pool al empezar. 0 = se crean a demanda.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Summon|Pool", meta = (ClampMin = "0"))
	int32 PrewarmPerClass = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Summon|Animation")
	TObjectPtr<UAnimMontage> SummonMontage;

	// Momento normalizado del montaje en que aparecen los avisos (o AnimNotify "Boss Action Event").
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Summon|Animation", meta = (ClampMin = "0", ClampMax = "1"))
	float SummonEventTime = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Summon|Animation", meta = (ClampMin = "0.1"))
	float SummonPlayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Summon|Animation", meta = (ClampMin = "0"))
	float RecoveryAfterSummon = 0.5f;

	// Tiempo entre el efecto en el piso y la aparición del enemigo.
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

	// Reproduce el montaje de invocación (a través del componente de combate) y en su evento aparecen los enemigos.
	UFUNCTION(BlueprintCallable, Category = "Boss|Summon")
	bool StartSummon(AActor* Target, int32 CountOverride = 0);

	// Invoca sin montaje ni cooldown (respeta MaxAliveMinions). Devuelve cuántos quedaron programados.
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
