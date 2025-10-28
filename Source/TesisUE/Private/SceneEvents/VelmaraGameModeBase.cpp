// Fill out your copyright notice in the Description page of Project Settings.


#include "SceneEvents/VelmaraGameModeBase.h"
#include "SceneEvents/VelmaraGameInstance.h"
#include "Enemy/Enemy.h"

TArray<AEnemy*> AVelmaraGameModeBase::GetRegisteredEnemies()
{
	return RegisteredEnemies;
}

void AVelmaraGameModeBase::RegisterEnemy(AEnemy* Enemy)
{
	RegisteredEnemies.AddUnique(Enemy);
}

void AVelmaraGameModeBase::UnregisterEnemy(AEnemy* Enemy)
{
	RegisteredEnemies.Remove(Enemy);
}

void AVelmaraGameModeBase::SetEnemiesAIEnabled(bool bEnabled)
{
	for (AEnemy* Enemy : RegisteredEnemies)
	{
		if (Enemy && Enemy->IsAlive())
		{
			bEnabled ? Enemy->EnableAI() : Enemy->DisableAI();
		}
	}
}

void AVelmaraGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	if (UVelmaraGameInstance* NewGameInstance = Cast<UVelmaraGameInstance>(GetGameInstance()))
	{
		NewGameInstance->ApplyPendingLoadedDataToWorld();
	}
}