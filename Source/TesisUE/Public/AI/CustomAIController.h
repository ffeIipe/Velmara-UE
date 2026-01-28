#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "CustomAIController.generated.h"

UCLASS()
class TESISUE_API ACustomAIController : public AAIController
{
	GENERATED_BODY()

public:
	ACustomAIController(const FObjectInitializer& ObjectInitializer);

	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;
};
