#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "Features/SolarBoss/AI/SolarBossActionPlayback.h"
#include "Features/SolarBoss/Data/SolarBossTypes.h"
#include "BTTask_SolarBossDodge.generated.h"

struct FBTSolarBossDodgeMemory
{
	FSolarBossActionPlayback Playback;
	FVector Offset = FVector::ZeroVector;
	float Elapsed = 0.f;
	float Duration = 0.f;
	float MovedAlpha = 0.f;
};

UCLASS()
class TESISUE_API UBTTask_SolarBossDodge : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_SolarBossDodge();

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FBTSolarBossDodgeMemory); }

protected:
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Dodge")
	ESolarBossDodgeMode DodgeMode = ESolarBossDodgeMode::EscapeCycle;

	UPROPERTY(EditAnywhere, Category = "Dodge")
	ESolarBossDodgeDirection Direction = ESolarBossDodgeDirection::AwayFromTarget;

	UPROPERTY(EditAnywhere, Category = "Dodge")
	bool bSkipIfTargetOutsidePanicRadius = false;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

private:
	FVector FindDestination(APawn* Pawn, const AActor* Target, float Distance) const;
};
