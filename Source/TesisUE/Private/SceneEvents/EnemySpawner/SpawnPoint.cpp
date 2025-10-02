// Fill out your copyright notice in the Description page of Project Settings.


#include "SceneEvents/EnemySpawner/SpawnPoint.h"

#include "Enemy/Enemy.h"
#include "Subsystems/EnemyPoolManager.h"


// Sets default values
ASpawnPoint::ASpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ASpawnPoint::Spawn() const
{
	if (UEnemyPoolManager* EnemyPoolManager = GetWorld()->GetSubsystem<UEnemyPoolManager>())
	{
		const bool bCanLoadClass = !EnemyClass.IsNull();
		const bool bClassLoaded = EnemyClass.IsValid();
		
		if (bCanLoadClass && !bClassLoaded)
		{
			 EnemyClass.LoadSynchronous();
		}
		
		if (bClassLoaded)
		{
			AEnemy* TempEnemy = EnemyPoolManager->SpawnEnemyFromPool(EnemyClass.Get(), GetActorLocation(), GetActorRotation());
			TempEnemy->OnDeactivated.AddDynamic(this, &ASpawnPoint::DecreaseEnemiesSpawned);
		}
		else
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3, FColor::Red, "Fail to Load Class of Enemy from Spawn Point");
		}
	}
}

void ASpawnPoint::DecreaseEnemiesSpawned(AEnemy* DeactivatedEnemy)
{
	OnEnemyDeactivated.ExecuteIfBound();
}
