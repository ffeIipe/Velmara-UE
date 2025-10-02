#include "Subsystems/EnemyPoolManager.h"
#include "Enemy/Enemy.h"
#include "Engine/World.h"

void UEnemyPoolManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UEnemyPoolManager::Deinitialize()
{
    for (auto& Pair : EnemyPools)
    {
        for (AEnemy* Enemy : Pair.Value.PooledEnemies)
        {
            if (IsValid(Enemy))
            {
                Enemy->SetActorHiddenInGame(true);
                Enemy->SetActorTickEnabled(false);
                Enemy->Destroy();
            }
        }
        Pair.Value.PooledEnemies.Empty();
    }
    EnemyPools.Empty();
    Super::Deinitialize();
}

void UEnemyPoolManager::EnsurePoolInitialized(const TSubclassOf<AEnemy> EnemyClass, const int32 InitialPoolSize)
{
    if (!EnemyClass)
    {
        return;
    }

    if (FEnemyPool* Pool = GetOrCreatePool(EnemyClass))
    {
        const int32 EnemiesToCreate = InitialPoolSize - Pool->PooledEnemies.Num();
        for (int32 i = 0; i < EnemiesToCreate; ++i)
        {
            if (AEnemy* NewEnemy = SpawnNewEnemyForPool(EnemyClass, FVector::ZeroVector, FRotator::ZeroRotator, nullptr, nullptr))
            {
                NewEnemy->DeactivateEnemy();
                Pool->PooledEnemies.Add(NewEnemy);
            }
        }
    }
}

AEnemy* UEnemyPoolManager::SpawnEnemyFromPool(const TSubclassOf<AEnemy> EnemyClass, const FVector& Location, const FRotator& Rotation, AActor* Owner, APawn* Instigator)
{
    if (!EnemyClass)
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3, FColor::Red, "Invalid Enemy class.");
        return nullptr;
    }

    FEnemyPool* Pool = GetOrCreatePool(EnemyClass);
    AEnemy* EnemyToSpawn;

    if (Pool && Pool->PooledEnemies.Num() > 0)
    {
        EnemyToSpawn = Pool->PooledEnemies.Pop();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Pool for %s is empty or does not exist. Spawning a new one."), *EnemyClass->GetName());
        EnemyToSpawn = SpawnNewEnemyForPool(EnemyClass, Location, Rotation, Owner, Instigator);
        if (EnemyToSpawn && Pool)
        {
            // Freshly spawned actor might have run its BeginPlay.
            // We ideally want to control this. For now, we assume it's okay and call Activate.
            // More robust solution: spawn deferred, then call custom Init, then FinishSpawning.
        }
    }

    if (EnemyToSpawn)
    {
        EnemyToSpawn->SetOwner(Owner);
        EnemyToSpawn->SetInstigator(Instigator);
        EnemyToSpawn->ActivateEnemy(Location, Rotation);
    }
    return EnemyToSpawn;
}

void UEnemyPoolManager::ReturnEnemyToPool(AEnemy* Enemy)
{
    if (!Enemy)
    {
        UE_LOG(LogTemp, Warning, TEXT("ReturnEnemyToPool: Enemy is null."));
        return;
    }

    const TSubclassOf<AEnemy> EnemyClass = Enemy->GetClass();

    if (FEnemyPool* Pool = GetOrCreatePool(EnemyClass))
    {
        Pool->PooledEnemies.Add(Enemy);
        
    }
    else
    {
        Enemy->Destroy();
    }
}

FEnemyPool* UEnemyPoolManager::GetOrCreatePool(const TSubclassOf<AEnemy>& EnemyClass)
{
    if (!EnemyClass) return nullptr;

    FEnemyPool* FoundPool = EnemyPools.Find(EnemyClass);
    if (!FoundPool)
    {
        FEnemyPool NewPool;
        NewPool.EnemyClass = EnemyClass;
        EnemyPools.Add(EnemyClass, NewPool);
        FoundPool = EnemyPools.Find(EnemyClass);
    }
    return FoundPool;
}

AEnemy* UEnemyPoolManager::SpawnNewEnemyForPool(const TSubclassOf<AEnemy>& EnemyClass, const FVector& Location, const FRotator& Rotation, AActor* Owner, APawn* Instigator) const
{
    UWorld* World = GetWorld();
    if (!World || !EnemyClass)
    {
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = Owner;
    SpawnParams.Instigator = Instigator;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AEnemy* NewEnemy = World->SpawnActor<AEnemy>(EnemyClass, Location, Rotation, SpawnParams);

    return NewEnemy;
}
