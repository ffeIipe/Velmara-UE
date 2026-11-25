#pragma once

#include "CoreMinimal.h"
#include "AI/CustomAIController.h"
#include "SolarBossAIController.generated.h"

class UBehaviorTree;

UCLASS()
class TESISUE_API ASolarBossAIController : public ACustomAIController
{
	GENERATED_BODY()

public:
	ASolarBossAIController(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void OnPossess(APawn* InPawn) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Solar Boss")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;
};
