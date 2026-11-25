#include "Features/SolarBoss/AI/SolarBossAIController.h"

#include "BehaviorTree/BehaviorTree.h"

ASolarBossAIController::ASolarBossAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ASolarBossAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (BehaviorTreeAsset)
	{
		RunBehaviorTree(BehaviorTreeAsset);
	}
}
