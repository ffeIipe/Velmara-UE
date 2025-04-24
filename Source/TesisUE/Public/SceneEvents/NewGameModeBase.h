// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NewGameModeBase.generated.h"

class AEnemy;

UCLASS()
class TESISUE_API ANewGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
	UPROPERTY()
	TArray<AEnemy*> RegisteredEnemies;

public:
	void RegisterEnemy(AEnemy* Enemy);

	void UnregisterEnemy(AEnemy* Enemy);
	
	void SetEnemiesAIEnabled(bool bEnabled);
};
