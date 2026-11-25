#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_SolarBossEvaluator.generated.h"

UCLASS()
class TESISUE_API UBTService_SolarBossEvaluator : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_SolarBossEvaluator();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
