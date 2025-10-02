// Fill out your copyright notice in the Description page of Project Settings.


#include "SceneEvents/EnemySpawner.h"

#include "SceneEvents/EnemySpawner/SpawnPoint.h"


// Sets default values
AEnemySpawner::AEnemySpawner()
{
	PrimaryActorTick.bCanEverTick = false;

}

void AEnemySpawner::StartSpawning()
{
	//if (!GetWorld()->GetTimerManager().IsTimerActive(SpawnTimerHandle))
	{
		const float TimeBetweenSpawn = FMath::RandRange(TimeBetweenSpawnMin, TimeBetweenSpawnMax);
	
		GetWorld()->GetTimerManager().SetTimer(
			SpawnTimerHandle,
			this,
			&AEnemySpawner::SpawnEnemyFromSpawnPoint,
			TimeBetweenSpawn,
			true);
	}
}

void AEnemySpawner::SpawnEnemyFromSpawnPoint()
{
	if (EnemiesSpawned < EnemiesSpawnedLimit)
	{
		EnemiesSpawned++;
		const int32 RandomIndex = FMath::RandRange(0, SpawnPoints.Num() - 1);

		if (SpawnPoints.IsValidIndex(RandomIndex))
		{
			SpawnPoints[RandomIndex]->Spawn();
		}
		else if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "Invalid Random Index!");	
		}
	}
	//else EndSpawning();
}

void AEnemySpawner::EndSpawning()
{
	GetWorld()->GetTimerManager().ClearTimer(SpawnTimerHandle);
}

void AEnemySpawner::BeginPlay()
{
	Super::BeginPlay();

	for (ASpawnPoint* SpawnPoint : SpawnPoints)
	{
		SpawnPoint->OnEnemyDeactivated.BindUFunction(this, "DecreaseEnemiesSpawned");
	}
}

void AEnemySpawner::DecreaseEnemiesSpawned()
{
	EnemiesSpawned--;

	if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Emerald, "DecreaseEnemiesSpawned");
	
	//StartSpawning();
}


