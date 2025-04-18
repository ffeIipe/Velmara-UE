// Fill out your copyright notice in the Description page of Project Settings.


#include "SceneEvents/NewGameModeBase.h"
#include "Enemy/Enemy.h"

void ANewGameModeBase::RegisterEnemy(AEnemy* Enemy)
{
	RegisteredEnemies.AddUnique(Enemy);
}

void ANewGameModeBase::UnregisterEnemy(AEnemy* Enemy)
{
	RegisteredEnemies.Remove(Enemy);
}

void ANewGameModeBase::SetEnemiesAIEnabled(bool bEnabled)
{
	for (AEnemy* Enemy : RegisteredEnemies)
	{
		if (Enemy)
		{
			bEnabled ? Enemy->EnableAI() : Enemy->DisableAI();
		}
	}
}