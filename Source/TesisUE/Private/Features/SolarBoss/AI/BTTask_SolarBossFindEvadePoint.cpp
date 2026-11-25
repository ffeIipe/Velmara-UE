#include "Features/SolarBoss/AI/BTTask_SolarBossFindEvadePoint.h"

#include "AIController.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Features/SolarBoss/Components/SolarBossBrainComponent.h"
#include "Features/SolarBoss/Data/SolarBossConfigDataAsset.h"
#include "GameFramework/Pawn.h"

UBTTask_SolarBossFindEvadePoint::UBTTask_SolarBossFindEvadePoint()
{
	NodeName = TEXT("Solar Boss Find Evade Point");

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, TargetActorKey), AActor::StaticClass());
	TargetActorKey.SelectedKeyName = TEXT("TargetActor");
	OutLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, OutLocationKey));
	OutLocationKey.SelectedKeyName = TEXT("MoveLocation");
}

void UBTTask_SolarBossFindEvadePoint::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetActorKey.ResolveSelectedKey(*BBAsset);
		OutLocationKey.ResolveSelectedKey(*BBAsset);
	}
}

EBTNodeResult::Type UBTTask_SolarBossFindEvadePoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FBTSolarBossEvadeMemory* Memory = CastInstanceNodeMemory<FBTSolarBossEvadeMemory>(NodeMemory);
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	APawn* Pawn = OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr;
	const USolarBossBrainComponent* Brain = Pawn ? Pawn->FindComponentByClass<USolarBossBrainComponent>() : nullptr;
	const AActor* Target = Blackboard ? Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName)) : nullptr;
	UNavigationSystemV1* NavSystem = Pawn ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(Pawn->GetWorld()) : nullptr;
	if (!Brain || !Brain->IsReady() || !Target || !NavSystem) return EBTNodeResult::Failed;

	const double Now = Pawn->GetWorld()->GetTimeSeconds();
	if (Now < Memory->NextRepathTime && Blackboard->IsVectorValueSet(OutLocationKey.SelectedKeyName)) return EBTNodeResult::Succeeded;

	const USolarBossConfigDataAsset* Config = Brain->Config;
	const FVector TargetLocation = Target->GetActorLocation();
	const FVector ArenaOrigin = Brain->GetArenaOrigin();

	FVector Away = (Pawn->GetActorLocation() - TargetLocation).GetSafeNormal2D();
	if (Away.IsNearlyZero())
	{
		Away = -Target->GetActorForwardVector().GetSafeNormal2D();
	}

	FVector Best = FAISystem::InvalidLocation;
	float BestScore = -TNumericLimits<float>::Max();
	const float FirstSide = FMath::RandBool() ? 1.f : -1.f;

	for (int32 Sample = 0; Sample < Config->EvadeSearchSamples; ++Sample)
	{
		const float Side = Sample % 2 == 0 ? FirstSide : -FirstSide;
		const float WideningScale = Sample < Config->EvadeSearchSamples / 2 ? 1.f : 2.f;
		const float Angle = Side * WideningScale * FMath::FRandRange(Config->EvadeStrafeAngles.X, Config->EvadeStrafeAngles.Y);

		FVector Desired = TargetLocation + Away.RotateAngleAxis(Angle, FVector::UpVector)
			* (Config->EvadePreferredDistance * FMath::FRandRange(0.9f, 1.1f));

		if (FVector::Dist2D(Desired, ArenaOrigin) > Config->ArenaRadius)
		{
			const FVector Clamped = ArenaOrigin + (Desired - ArenaOrigin).GetSafeNormal2D() * Config->ArenaRadius;
			Desired.X = Clamped.X;
			Desired.Y = Clamped.Y;
		}

		FNavLocation Projected;
		if (!NavSystem->ProjectPointToNavigation(Desired, Projected, FVector(300.f, 300.f, 600.f))) continue;

		const UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(Pawn->GetWorld(), Pawn->GetActorLocation(),
			Projected.Location, Pawn);
		if (!Path || !Path->IsValid() || Path->IsPartial()) continue;

		const float Distance = FVector::Dist2D(Projected.Location, TargetLocation);
		const float Score = Distance >= Config->EvadeMinSafeDistance
			? -FMath::Abs(Distance - Config->EvadePreferredDistance)
			: Distance - 100000.f;

		if (Score > BestScore)
		{
			Best = Projected.Location;
			BestScore = Score;
		}
	}

	if (!FAISystem::IsValidLocation(Best)) return EBTNodeResult::Failed;

	Memory->NextRepathTime = Now + Config->EvadeRepathInterval;
	Blackboard->SetValueAsVector(OutLocationKey.SelectedKeyName, Best);
	return EBTNodeResult::Succeeded;
}
