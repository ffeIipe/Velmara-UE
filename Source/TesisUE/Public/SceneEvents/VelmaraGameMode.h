#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "VelmaraGameMode.generated.h"

class UPlayerProgressSaveGame;
class AEnemy;

UCLASS()
class TESISUE_API AVelmaraGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	
	void SetEnemiesAIEnabled(bool bEnabled);
	
	void RegisterEnemy(AEnemy* Enemy);
	void UnregisterEnemy(AEnemy* Enemy);
	
private:
	UPROPERTY()
	TArray<AEnemy*> RegisteredEnemies;
};