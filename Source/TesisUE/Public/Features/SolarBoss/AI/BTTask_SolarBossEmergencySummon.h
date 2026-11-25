#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "Features/SolarBoss/AI/SolarBossActionPlayback.h"
#include "BTTask_SolarBossEmergencySummon.generated.h"

struct FBTSolarBossEmergencyMemory
{
	FSolarBossActionPlayback Playback;
	bool bEventFired = false;
};

UCLASS()
class TESISUE_API UBTTask_SolarBossEmergencySummon : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_SolarBossEmergencySummon();

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FBTSolarBossEmergencyMemory); }

protected:
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;
};
