#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "Features/SolarBoss/AI/SolarBossActionPlayback.h"
#include "BTTask_SolarBossBreakAndSummon.generated.h"

struct FBTSolarBossBreakMemory
{
	FSolarBossActionPlayback Playback;
	bool bSummonPhase = false;
	bool bEventFired = false;
};

UCLASS()
class TESISUE_API UBTTask_SolarBossBreakAndSummon : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_SolarBossBreakAndSummon();

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FBTSolarBossBreakMemory); }

protected:
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

private:
	AActor* GetTarget(const UBehaviorTreeComponent& OwnerComp) const;
};
