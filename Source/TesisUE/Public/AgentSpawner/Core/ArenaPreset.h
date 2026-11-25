#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ArenaPreset.generated.h"

USTRUCT(BlueprintType)
struct FArenaAgentEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Config")
	TSubclassOf<AActor> AgentClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Config", meta = (ClampMin = "0"))
	int32 Weight = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Config", meta = (ClampMin = "0", ToolTip = "0 = unlimited"))
	int32 MaxAliveAtOnce = 0;
};

UCLASS(Blueprintable)
class TESISUE_API UArenaPreset : public UDataAsset
{
	GENERATED_BODY()

	public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner Config")
	TArray<FArenaAgentEntry> Agents;

	UFUNCTION(BlueprintCallable, Category = "Spawner Config")
	TSubclassOf<AActor> PickNextAgentClass(const TArray<TSubclassOf<AActor>>& SpawnedClasses, const TArray<AActor*>& AliveAgents) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave Rules")
	int32 TotalEnemiesToSpawn = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave Rules")
	int32 MaxEnemiesAliveAtOnce = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave Rules")
	bool bShouldSurvivePerTime = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave Rules", meta = (EditCondition = "bShouldSurvivePerTime", EditConditionHides))
	float TimeInSeconds = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pacing")
	float MinSpawnInterval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pacing")
	float MaxSpawnInterval = 3.0f;
    
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NavMesh")
	float NavMeshProjectionExtent = 200.0f;
};
