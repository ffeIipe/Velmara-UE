#include "Subsystems/OrbsPoolManager.h"
#include "Items/Item.h"
#include <Interfaces/OrbPoolingInterface.h>

void UOrbsPoolManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UOrbsPoolManager::Deinitialize()
{
    for (auto& Pair : OrbsPools)
    {
        for (AActor* Item : Pair.Value.PooledItems)
        {
            if (IsValid(Item))
            {
                Item->SetActorHiddenInGame(true);
                Item->SetActorTickEnabled(false);
                Item->Destroy();
            }
        }
        Pair.Value.PooledItems.Empty();
    }
    OrbsPools.Empty();
    Super::Deinitialize();
}

void UOrbsPoolManager::EnsurePoolInitialized(TSubclassOf<AActor> OrbsClass, int32 InitialPoolSize)
{
    if (!OrbsClass)
    {
        return;
    }

    FOrbsPool* Pool = GetOrCreatePool(OrbsClass);
    if (Pool)
    {
        int32 OrbsToCreate = InitialPoolSize - Pool->PooledItems.Num();
        for (int32 i = 0; i < OrbsToCreate; ++i)
        {
            AActor* NewItem = SpawnNewOrbForPool(OrbsClass, FVector::ZeroVector, FRotator::ZeroRotator, nullptr, nullptr);
            if (NewItem)
            {
                if (NewItem->GetClass()->ImplementsInterface(UOrbPoolingInterface::StaticClass()))
                {
                    GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Green, FString("Succesful cast to OrbInterface"));
                    IOrbPoolingInterface::Execute_DeactivateOrb(NewItem);
                }
                else GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, FString("Failed to cast to OrbInterface"));

                Pool->PooledItems.Add(NewItem);
            }
        }
    }
}

AActor* UOrbsPoolManager::SpawnOrbFromPool(TSubclassOf<AActor> OrbsClass, const FVector& Location, const FRotator& Rotation, AActor* Owner, APawn* Instigator)
{
    if (!OrbsClass)
    {
        return nullptr;
    }

    FOrbsPool* Pool = GetOrCreatePool(OrbsClass);
    AActor* ItemToSpawn = nullptr;

    if (Pool && Pool->PooledItems.Num() > 0)
    {
        ItemToSpawn = Pool->PooledItems.Pop();
    }
    else
    {
        ItemToSpawn = SpawnNewOrbForPool(OrbsClass, Location, Rotation, Owner, Instigator);
        if (ItemToSpawn && Pool)
        {
            // Freshly spawned actor might have run its BeginPlay.
            // We ideally want to control this. For now, we assume it's okay and call Activate.
            // More robust solution: spawn deferred, then call custom Init, then FinishSpawning.
        }
    }

    if (ItemToSpawn)
    {
        ItemToSpawn->SetOwner(Owner);
        ItemToSpawn->SetInstigator(Instigator);

        if (ItemToSpawn->GetClass()->ImplementsInterface(UOrbPoolingInterface::StaticClass()))
        {
            IOrbPoolingInterface::Execute_ActivateOrb(ItemToSpawn, Location, Rotation);
        }
    }
    return ItemToSpawn;
}

void UOrbsPoolManager::ReturnOrbToPool(AActor* Orb)
{
    if (!Orb)
    {
        UE_LOG(LogTemp, Warning, TEXT("ReturnEnemyToPool: Enemy is null."));
        return;
    }

    TSubclassOf<AItem> EnemyClass = Orb->GetClass();
    FOrbsPool* Pool = GetOrCreatePool(EnemyClass);

    if (Pool)
    {
        if (Orb->GetClass()->ImplementsInterface(UOrbPoolingInterface::StaticClass()))
        {
            IOrbPoolingInterface::Execute_DeactivateOrb(Orb);
        }
        Pool->PooledItems.Add(Orb);
    }
    else
    {
        Orb->Destroy();
    }
}

FOrbsPool* UOrbsPoolManager::GetOrCreatePool(TSubclassOf<AActor> OrbsClass)
{
    if (!OrbsClass) return nullptr;

    FOrbsPool* FoundPool = OrbsPools.Find(OrbsClass);
    if (!FoundPool)
    {
        FOrbsPool NewPool;
        NewPool.ItemClass = OrbsClass;
        OrbsPools.Add(OrbsClass, NewPool);
        FoundPool = OrbsPools.Find(OrbsClass);
    }
    return FoundPool;
}

AActor* UOrbsPoolManager::SpawnNewOrbForPool(TSubclassOf<AActor> OrbsClass, const FVector& Location, const FRotator& Rotation, AActor* Owner, APawn* Instigator)
{
    UWorld* World = GetWorld();
    if (!World || !OrbsClass)
    {
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = Owner;
    SpawnParams.Instigator = Instigator;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AActor* NewItem = World->SpawnActor<AActor>(OrbsClass, Location, Rotation, SpawnParams);

    return NewItem;
}