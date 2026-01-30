#pragma once

#include "CoreMinimal.h"
#include "DetourCrowdAIController.h"
#include "CustomAIController.generated.h"

UCLASS()
class TESISUE_API ACustomAIController : public ADetourCrowdAIController
{
	GENERATED_BODY()

public:
	ACustomAIController(const FObjectInitializer& ObjectInitializer);

	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;
};
