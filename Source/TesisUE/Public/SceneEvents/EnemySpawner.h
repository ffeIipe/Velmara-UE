// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemySpawner.generated.h"

class ASpawnPoint;

UCLASS()
class TESISUE_API AEnemySpawner : public AActor
{
	GENERATED_BODY()

public:
	AEnemySpawner();

	UFUNCTION(BlueprintCallable)
	void StartSpawning();
	
	void SpawnEnemyFromSpawnPoint();
	void EndSpawning();
	
protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly)
	TArray<ASpawnPoint*> SpawnPoints;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly)
	int32 EnemiesSpawnedLimit = 5;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly)
	float TimeBetweenSpawnMin = 1.f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly)
	float TimeBetweenSpawnMax = 3.f;

	FTimerHandle SpawnTimerHandle;

private:
	int32 EnemiesSpawned = 0;

	UFUNCTION()
	void DecreaseEnemiesSpawned();
};
