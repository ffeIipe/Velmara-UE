#include "Features/SolarBoss/AI/BTService_SolarBossEvaluator.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Features/SolarBoss/Components/SolarBossBrainComponent.h"
#include "Features/SolarBoss/Data/SolarBossConfigDataAsset.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

UBTService_SolarBossEvaluator::UBTService_SolarBossEvaluator()
{
	NodeName = TEXT("Solar Boss Evaluator");

	INIT_SERVICE_NODE_NOTIFY_FLAGS();

	Interval = 0.1f;
	RandomDeviation = 0.f;
	bCallTickOnSearchStart = true;
}

void UBTService_SolarBossEvaluator::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	const AAIController* Controller = OwnerComp.GetAIOwner();
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	USolarBossBrainComponent* Brain = Pawn ? Pawn->FindComponentByClass<USolarBossBrainComponent>() : nullptr;
	if (!Blackboard || !Brain || !Brain->IsReady()) return;

	APawn* Target = UGameplayStatics::GetPlayerPawn(Pawn, 0);
	if (Target && (Target->IsHidden() || !USolarBossBrainComponent::IsActorAlive(Target)))
	{
		Target = nullptr;
	}

	const float Distance = Target ? FVector::Dist2D(Pawn->GetActorLocation(), Target->GetActorLocation()) : TNumericLimits<float>::Max();
	if (Distance <= Brain->Config->AggroRadius)
	{
		Brain->StartEncounter();
	}

	Brain->EvaluateState(Target);

	const bool bPanic = Brain->GetMode() == ESolarBossMode::EvadeWithMinions
		&& Distance <= Brain->Config->EvadePanicDodgeRadius && Brain->IsReactiveDodgeReady();

	Blackboard->SetValueAsObject(TEXT("TargetActor"), Target);
	Blackboard->SetValueAsEnum(TEXT("BossMode"), static_cast<uint8>(Brain->GetMode()));
	Blackboard->SetValueAsFloat(TEXT("DistanceToTarget"), Distance);
	Blackboard->SetValueAsInt(TEXT("AliveMinionCount"), Brain->GetAliveMinionCount());
	Blackboard->SetValueAsBool(TEXT("bEncounterStarted"), Brain->HasEncounterStarted());
	Blackboard->SetValueAsBool(TEXT("bPendingCycleBreak"), Brain->HasPendingCycleBreak());
	Blackboard->SetValueAsBool(TEXT("bNeedsDefensiveDodge"), Brain->NeedsDefensiveDodge());
	Blackboard->SetValueAsBool(TEXT("bNeedsEmergencySummon"), Brain->NeedsEmergencySummon());
	Blackboard->SetValueAsBool(TEXT("bInEvadePanicRange"), bPanic);
}
