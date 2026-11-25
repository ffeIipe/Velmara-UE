#include "Features/SolarBoss/AI/BTTask_SolarBossBreakAndSummon.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Features/SolarBoss/Components/SolarBossBrainComponent.h"
#include "Features/SolarBoss/Data/SolarBossConfigDataAsset.h"
#include "GameFramework/Character.h"

UBTTask_SolarBossBreakAndSummon::UBTTask_SolarBossBreakAndSummon()
{
	NodeName = TEXT("Solar Boss Break And Summon");
	bNotifyTick = true;
	bNotifyTaskFinished = true;

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, TargetActorKey), AActor::StaticClass());
	TargetActorKey.SelectedKeyName = TEXT("TargetActor");
}

void UBTTask_SolarBossBreakAndSummon::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetActorKey.ResolveSelectedKey(*BBAsset);
	}
}

AActor* UBTTask_SolarBossBreakAndSummon::GetTarget(const UBehaviorTreeComponent& OwnerComp) const
{
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	return Blackboard ? Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName)) : nullptr;
}

EBTNodeResult::Type UBTTask_SolarBossBreakAndSummon::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FBTSolarBossBreakMemory* Memory = CastInstanceNodeMemory<FBTSolarBossBreakMemory>(NodeMemory);
	*Memory = FBTSolarBossBreakMemory();

	AAIController* Controller = OwnerComp.GetAIOwner();
	ACharacter* Character = Controller ? Cast<ACharacter>(Controller->GetPawn()) : nullptr;
	USolarBossBrainComponent* Brain = Character ? Character->FindComponentByClass<USolarBossBrainComponent>() : nullptr;
	if (!Brain || !Brain->BeginCycleBreak()) return EBTNodeResult::Failed;

	Controller->StopMovement();
	Memory->Playback.Start(Character, Brain->Config->CeilingBreakMontage, Brain->Config->CeilingBreakPlayRate,
		Brain->Config->CeilingBreakFallbackDuration);

	return EBTNodeResult::InProgress;
}

void UBTTask_SolarBossBreakAndSummon::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FBTSolarBossBreakMemory* Memory = CastInstanceNodeMemory<FBTSolarBossBreakMemory>(NodeMemory);
	ACharacter* Character = OwnerComp.GetAIOwner() ? Cast<ACharacter>(OwnerComp.GetAIOwner()->GetPawn()) : nullptr;
	USolarBossBrainComponent* Brain = Character ? Character->FindComponentByClass<USolarBossBrainComponent>() : nullptr;
	if (!Brain || !Brain->IsReady())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	const USolarBossConfigDataAsset* Config = Brain->Config;
	const bool bRunning = Memory->Playback.Advance(Character, DeltaSeconds);
	const float EventTime = Memory->bSummonPhase ? Config->SummonEventTime : Config->CeilingBreakEventTime;

	if (!Memory->bEventFired && (!bRunning || Memory->Playback.Progress >= EventTime))
	{
		Memory->bEventFired = true;
		if (Memory->bSummonPhase)
		{
			Brain->SummonCycleMinions(GetTarget(OwnerComp));
		}
		else
		{
			Brain->ExecuteCeilingBreak(GetTarget(OwnerComp));
		}
	}

	if (bRunning) return;

	if (!Memory->bSummonPhase)
	{
		Memory->bSummonPhase = true;
		Memory->bEventFired = false;
		Memory->Playback.Start(Character, Config->SummonMontage, Config->SummonPlayRate, Config->SummonFallbackDuration);
		return;
	}

	Brain->CompleteCycleBreak();
	FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
}

EBTNodeResult::Type UBTTask_SolarBossBreakAndSummon::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	return EBTNodeResult::Aborted;
}

void UBTTask_SolarBossBreakAndSummon::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	if (TaskResult == EBTNodeResult::Aborted)
	{
		const FBTSolarBossBreakMemory* Memory = CastInstanceNodeMemory<FBTSolarBossBreakMemory>(NodeMemory);
		ACharacter* Character = OwnerComp.GetAIOwner() ? Cast<ACharacter>(OwnerComp.GetAIOwner()->GetPawn()) : nullptr;
		USolarBossBrainComponent* Brain = Character ? Character->FindComponentByClass<USolarBossBrainComponent>() : nullptr;
		if (Brain && Brain->IsReady())
		{
			Memory->Playback.Stop(Character);
			Brain->ExecuteCeilingBreak(GetTarget(OwnerComp));
			Brain->SummonCycleMinions(GetTarget(OwnerComp));
			Brain->CompleteCycleBreak();
		}
	}

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}
