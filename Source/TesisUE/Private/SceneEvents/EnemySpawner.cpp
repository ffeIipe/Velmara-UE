// Fill out your copyright notice in the Description page of Project Settings.


#include "SceneEvents/EnemySpawner.h"

#include "SceneEvents/EnemySpawner/SpawnPoint.h"


AEnemySpawner::AEnemySpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	MaxEnemiesSpawn = 10;
}

void AEnemySpawner::StartSpawning()
{
	const float TimeBetweenSpawn = FMath::RandRange(TimeBetweenSpawnMin, TimeBetweenSpawnMax);
	
	GetWorld()->GetTimerManager().SetTimer(
		SpawnTimerHandle,
		this,
		&AEnemySpawner::SpawnEnemyFromSpawnPoint,
		TimeBetweenSpawn,
		true);
}

void AEnemySpawner::SpawnEnemyFromSpawnPoint()
{
	if (EnemiesSpawned < SpawnPoints.Num())
	{
		EnemiesSpawned++;
		
		if (bIsNumericalSpawn)
		{
			if (EnemiesSpawned >= MaxEnemiesSpawn)
			{
				EndSpawning();
				EnemiesSpawned = 0;
			}
		}
		
		if (const int32 RandomIndex = FMath::RandRange(0, SpawnPoints.Num() - 1); SpawnPoints.IsValidIndex(RandomIndex))
		{
			SpawnPoints[RandomIndex]->Spawn();
		}
		else if (GEngine)
		{
			//GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "Invalid Random Index!");	
		}
	}
}

void AEnemySpawner::EndSpawning()
{
	//GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "End of spawning!");
	
	GetWorld()->GetTimerManager().ClearTimer(SpawnTimerHandle);
}

void AEnemySpawner::BeginPlay()
{
	Super::BeginPlay();

	for (ASpawnPoint* SpawnPoint : SpawnPoints)
	{
		SpawnPoint->OnEnemyDeactivated.BindUFunction(this, "DecreaseEnemiesSpawned");

		SpawnPoint->SetOwner(this);
	}
}

void AEnemySpawner::DecreaseEnemiesSpawned()
{
	if (!bIsNumericalSpawn)
	{
		EnemiesSpawned--;
	}
}


