// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpawnPoint.generated.h"

class AEnemy;

DECLARE_DYNAMIC_DELEGATE(FOnEnemyDeactivation);

UCLASS()
class TESISUE_API ASpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	ASpawnPoint();
	
	void Spawn() const;
	
	FOnEnemyDeactivation OnEnemyDeactivated;
	
protected:
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly)
	TSoftClassPtr<AEnemy> EnemyClass;

private:
	UFUNCTION()
	void DecreaseEnemiesSpawned(AEnemy* DeactivatedEnemy);
};