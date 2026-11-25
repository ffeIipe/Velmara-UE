#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_SolarBossFindEvadePoint.generated.h"

struct FBTSolarBossEvadeMemory
{
	double NextRepathTime = 0.0;
};

UCLASS()
class TESISUE_API UBTTask_SolarBossFindEvadePoint : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_SolarBossFindEvadePoint();

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FBTSolarBossEvadeMemory); }

protected:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector OutLocationKey;
};
