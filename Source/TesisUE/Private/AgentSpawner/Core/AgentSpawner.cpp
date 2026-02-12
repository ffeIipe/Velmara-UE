#include "AgentSpawner/Core/AgentSpawner.h"
#include "NavigationSystem.h"


AAgentSpawner::AAgentSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AAgentSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorld()->GetTimerManager().ClearTimer(SpawnTimerHandle);
    Super::EndPlay(EndPlayReason);
}

void AAgentSpawner::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

#if WITH_EDITOR
    for (const FVector& Point : Config.SpawnPoints)
    {
        FVector WorldPos = Transform.TransformPosition(Point);
        DrawDebugSphere(GetWorld(), WorldPos, 30.0f, 12, FColor::Green, false, 0.1f);
        
        DrawDebugLine(GetWorld(), Transform.GetLocation(), WorldPos, FColor::Green, false, 0.1f);
    }
#endif
}

void AAgentSpawner::ScheduleNextSpawn()
{
    if (EnemiesSpawnedSoFar >= Config.TotalEnemiesToSpawn) return;

    const float RandomDelay = FMath::RandRange(Config.MinSpawnInterval, Config.MaxSpawnInterval);
    GetWorld()->GetTimerManager().SetTimer(SpawnTimerHandle, this, &AAgentSpawner::AttemptSpawn, RandomDelay, false);
}

void AAgentSpawner::AttemptSpawn()
{
    if (!Config.AgentClass) return;

    CleanUpAliveList();

    if (AliveEnemies.Num() >= Config.MaxEnemiesAliveAtOnce)
    {
        ScheduleNextSpawn(); 
        return;
    }

    if (FVector SpawnLocation; GetRandomNavigablePoint(SpawnLocation))
    {
        if (AActor* NewEnemy = GetAgentActor(SpawnLocation))
        {
            AliveEnemies.Add(NewEnemy);
            EnemiesSpawnedSoFar++;
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Fallo al encontrar punto en NavMesh para %s"), *GetName());
    }

    ScheduleNextSpawn();
}

void AAgentSpawner::CleanUpAliveList()
{
    for (int32 i = AliveEnemies.Num() - 1; i >= 0; --i)
    {
        if (!AliveEnemies[i].IsValid() || AliveEnemies[i]->IsHidden())
        {
            AliveEnemies.RemoveAt(i);
        }
    }
}

bool AAgentSpawner::GetRandomNavigablePoint(FVector& OutLocation)
{
    if (Config.SpawnPoints.Num() == 0) return false;

    const int32 RandomIndex = FMath::RandRange(0, Config.SpawnPoints.Num() - 1);

    const FVector LocalPoint = Config.SpawnPoints[RandomIndex];
    const FVector WorldPoint = GetActorTransform().TransformPosition(LocalPoint);

    if (UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
    {
        FNavLocation ResultLocation;
        const bool bFound = NavSystem->ProjectPointToNavigation(
            WorldPoint, 
            ResultLocation, 
            FVector(Config.NavMeshProjectionExtent, Config.NavMeshProjectionExtent, Config.NavMeshProjectionExtent)
        );

        if (bFound)
        {
            OutLocation = ResultLocation.Location;
            OutLocation.Z += 50.0f;
            return true;
        }
    }

    return false;
}