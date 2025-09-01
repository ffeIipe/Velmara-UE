// Fill out your copyright notice in the Description page of Project Settings.


#include "SceneEvents/NewGameModeBase.h"
#include "SceneEvents/NewGameInstance.h"
#include "Enemy/Enemy.h"
#include "Components/CharacterStateComponent.h"

TArray<AEnemy*> ANewGameModeBase::GetRegisteredEnemies()
{
	return RegisteredEnemies;
}

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
		if (Enemy && Enemy->IsAlive())
		{
			bEnabled ? Enemy->EnableAI() : Enemy->DisableAI();
		}
	}
}

void ANewGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	if (UNewGameInstance* NewGameInstance = Cast<UNewGameInstance>(GetGameInstance()))
	{
		NewGameInstance->ApplyPendingLoadedDataToWorld();
	}
}
