#include "SceneEvents/VelmaraGameMode.h"

#include "Enemy/Enemy.h"
#include "Features/SaveSystem/Subsystems/SaveGameSubsystem.h"
#include "SceneEvents/VelmaraGameInstance.h"


class USaveGameSubsystem;

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

	if (UGameInstance* GI = GetGameInstance())
	{
		if (USaveGameSubsystem* SaveSystem = GI->GetSubsystem<USaveGameSubsystem>())
		{
			SaveSystem->RestoreCurrentLevelState();
		}
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
