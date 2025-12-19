#include "SceneEvents/VelmaraGameMode.h"

#include "Enemy/Enemy.h"
#include "SceneEvents/VelmaraGameInstance.h"


void AVelmaraGameMode::RegisterEnemy(AEnemy* Enemy)
{
	RegisteredEnemies.AddUnique(Enemy);
}

void AVelmaraGameMode::UnregisterEnemy(AEnemy* Enemy)
{
	RegisteredEnemies.Remove(Enemy);
}

void AVelmaraGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (UVelmaraGameInstance* GI = Cast<UVelmaraGameInstance>(GetGameInstance()))
	{
		GI->RestoreLoadedData();
	}
}

void AVelmaraGameMode::SetEnemiesAIEnabled(const bool bEnabled)
{
	for (AEnemy* Enemy : RegisteredEnemies)
	{
		if (Enemy && Enemy->IsAlive())
		{
			bEnabled ? Enemy->EnableAI() : Enemy->DisableAI();
		}
	}
}
