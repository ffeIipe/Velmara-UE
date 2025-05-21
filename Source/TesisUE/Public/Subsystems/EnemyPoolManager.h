#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EnemyPoolManager.generated.h"

class AEnemy;

USTRUCT(BlueprintType)
struct FEnemyPool
{
    GENERATED_BODY()

public:
    UPROPERTY()
    TArray<AEnemy*> PooledEnemies;

    TSubclassOf<AEnemy> EnemyClass;

    UPROPERTY()
    TArray<AEnemy*> ActiveEnemies;
};

UCLASS()
class TESISUE_API UEnemyPoolManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Enemy Pooling")
    void EnsurePoolInitialized(TSubclassOf<AEnemy> EnemyClass, int32 InitialPoolSize = 5);

    UFUNCTION(BlueprintCallable, Category = "Enemy Pooling")
    AEnemy* SpawnEnemyFromPool(TSubclassOf<AEnemy> EnemyClass, const FVector& Location, const FRotator& Rotation, AActor* Owner = nullptr, APawn* Instigator = nullptr);

    UFUNCTION(BlueprintCallable, Category = "Enemy Pooling")
    void ReturnEnemyToPool(AEnemy* Enemy);

private:
    UPROPERTY()
    TMap<TSubclassOf<AEnemy>, FEnemyPool> EnemyPools;

    FEnemyPool* GetOrCreatePool(TSubclassOf<AEnemy> EnemyClass);

    AEnemy* SpawnNewEnemyForPool(TSubclassOf<AEnemy> EnemyClass, const FVector& Location, const FRotator& Rotation, AActor* Owner, APawn* Instigator);
};