// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AgentSpawner.generated.h"


USTRUCT(BlueprintType)
struct FAgentSpawnerConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Config")
	TSubclassOf<AActor> AgentClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (MakeEditWidget = true), Category = "Spawner Config")
	TArray<FVector> SpawnPoints;

	UPROPERTY(EditAnywhere, Category = "Wave Rules")
	int32 TotalEnemiesToSpawn = 10;

	UPROPERTY(EditAnywhere, Category = "Wave Rules")
	int32 MaxEnemiesAliveAtOnce = 5;

	UPROPERTY(EditAnywhere, Category = "Pacing")
	float MinSpawnInterval = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Pacing")
	float MaxSpawnInterval = 3.0f;
    
	UPROPERTY(EditAnywhere, Category = "NavMesh")
	float NavMeshProjectionExtent = 200.0f;
};

UCLASS()
class TESISUE_API AAgentSpawner : public AActor
{
	GENERATED_BODY()

public:
	AAgentSpawner();

	UFUNCTION(BlueprintImplementableEvent, Category = "Spawner")
	AActor* GetAgentActor(const FVector& Location);
	
protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Properties")
	FAgentSpawnerConfig Config;
	
private:
    int32 EnemiesSpawnedSoFar;
    FTimerHandle SpawnTimerHandle;
    
    TArray<TWeakObjectPtr<AActor>> AliveEnemies;
	
	UFUNCTION(BlueprintCallable, Category = "Spawner", meta = (DisplayName = "Try Spawn Agent Safe"))
    void AttemptSpawn();

	UFUNCTION(BlueprintCallable, Category = "Spawner", meta = (DisplayName = "Force Next Agent Spawn")) 
    void ScheduleNextSpawn();

	UFUNCTION(BlueprintCallable, Category = "Spawner")
    void CleanUpAliveList();
	
    bool GetRandomNavigablePoint(FVector& OutLocation);
};
