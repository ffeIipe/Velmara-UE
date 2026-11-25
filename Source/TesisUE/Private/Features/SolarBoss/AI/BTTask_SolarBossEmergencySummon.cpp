#include "Features/SolarBoss/AI/BTTask_SolarBossEmergencySummon.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Features/SolarBoss/Components/SolarBossBrainComponent.h"
#include "Features/SolarBoss/Data/SolarBossConfigDataAsset.h"
#include "GameFramework/Character.h"

UBTTask_SolarBossEmergencySummon::UBTTask_SolarBossEmergencySummon()
{
	NodeName = TEXT("Solar Boss Emergency Summon");
	bNotifyTick = true;
	bNotifyTaskFinished = true;

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, TargetActorKey), AActor::StaticClass());
	TargetActorKey.SelectedKeyName = TEXT("TargetActor");
}

void UBTTask_SolarBossEmergencySummon::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetActorKey.ResolveSelectedKey(*BBAsset);
	}
}

EBTNodeResult::Type UBTTask_SolarBossEmergencySummon::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FBTSolarBossEmergencyMemory* Memory = CastInstanceNodeMemory<FBTSolarBossEmergencyMemory>(NodeMemory);
	*Memory = FBTSolarBossEmergencyMemory();

	AAIController* Controller = OwnerComp.GetAIOwner();
	ACharacter* Character = Controller ? Cast<ACharacter>(Controller->GetPawn()) : nullptr;
	const USolarBossBrainComponent* Brain = Character ? Character->FindComponentByClass<USolarBossBrainComponent>() : nullptr;
	if (!Brain || !Brain->IsReady()) return EBTNodeResult::Failed;

	Controller->StopMovement();
	Memory->Playback.Start(Character, Brain->Config->SummonMontage, Brain->Config->SummonPlayRate, Brain->Config->SummonFallbackDuration);

	return EBTNodeResult::InProgress;
}

void UBTTask_SolarBossEmergencySummon::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FBTSolarBossEmergencyMemory* Memory = CastInstanceNodeMemory<FBTSolarBossEmergencyMemory>(NodeMemory);
	ACharacter* Character = OwnerComp.GetAIOwner() ? Cast<ACharacter>(OwnerComp.GetAIOwner()->GetPawn()) : nullptr;
	USolarBossBrainComponent* Brain = Character ? Character->FindComponentByClass<USolarBossBrainComponent>() : nullptr;
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Brain || !Brain->IsReady() || !Blackboard)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	const bool bRunning = Memory->Playback.Advance(Character, DeltaSeconds);
	if (!Memory->bEventFired && (!bRunning || Memory->Playback.Progress >= Brain->Config->SummonEventTime))
	{
		Memory->bEventFired = true;
		Brain->SummonEmergencyMinions(Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName)));
	}

	if (!bRunning)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

EBTNodeResult::Type UBTTask_SolarBossEmergencySummon::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	return EBTNodeResult::Aborted;
}

void UBTTask_SolarBossEmergencySummon::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	const FBTSolarBossEmergencyMemory* Memory = CastInstanceNodeMemory<FBTSolarBossEmergencyMemory>(NodeMemory);
	if (TaskResult == EBTNodeResult::Aborted && OwnerComp.GetAIOwner())
	{
		Memory->Playback.Stop(Cast<ACharacter>(OwnerComp.GetAIOwner()->GetPawn()));
	}

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}
