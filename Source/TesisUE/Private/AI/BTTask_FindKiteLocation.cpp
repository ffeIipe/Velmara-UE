#include "AI/BTTask_FindKiteLocation.h"

#include "AIController.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_FindKiteLocation::UBTTask_FindKiteLocation()
{
	NodeName = TEXT("Find Kite Location");

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, TargetActorKey), AActor::StaticClass());
	TargetActorKey.SelectedKeyName = TEXT("TargetActor");
	OutLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, OutLocationKey));
	OutLocationKey.SelectedKeyName = TEXT("TargetLocation");
}

void UBTTask_FindKiteLocation::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetActorKey.ResolveSelectedKey(*BBAsset);
		OutLocationKey.ResolveSelectedKey(*BBAsset);
	}
}

FString UBTTask_FindKiteLocation::GetStaticDescription() const
{
	return FString::Printf(TEXT("Keep %.0f from %s (retreat under %.0f) -> %s"), PreferredDistance, *TargetActorKey.SelectedKeyName.ToString(),
		RetreatDistance, *OutLocationKey.SelectedKeyName.ToString());
}

EBTNodeResult::Type UBTTask_FindKiteLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	const APawn* Pawn = OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr;
	const AActor* Target = Blackboard ? Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName)) : nullptr;
	UNavigationSystemV1* NavSys = Pawn ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(Pawn->GetWorld()) : nullptr;
	if (!Pawn || !Target || !NavSys) return EBTNodeResult::Failed;

	const FVector TargetLocation = Target->GetActorLocation();
	FVector Away = (Pawn->GetActorLocation() - TargetLocation).GetSafeNormal2D();
	if (Away.IsNearlyZero())
	{
		Away = -Target->GetActorForwardVector().GetSafeNormal2D();
	}

	// Ángulos (grados, alrededor del objetivo) en orden de preferencia.
	TArray<float> Angles;
	const bool bRetreat = FVector::Dist2D(Pawn->GetActorLocation(), TargetLocation) < RetreatDistance;
	if (bRetreat)
	{
		Angles = {0.f, 25.f, -25.f, 50.f, -50.f, 80.f, -80.f, 110.f, -110.f};
	}
	else
	{
		const float Side = FMath::RandBool() ? 1.f : -1.f;
		const float Angle = FMath::FRandRange(StrafeAngle.X, StrafeAngle.Y);
		Angles = {Side * Angle, -Side * Angle, Side * Angle * 0.5f, -Side * Angle * 0.5f, 0.f};
	}

	FVector Fallback = FAISystem::InvalidLocation;
	for (const float Angle : Angles)
	{
		const float Distance = PreferredDistance * FMath::FRandRange(0.9f, 1.1f);
		const FVector Desired = TargetLocation + Away.RotateAngleAxis(Angle, FVector::UpVector) * Distance;

		FNavLocation Projected;
		if (!NavSys->ProjectPointToNavigation(Desired, Projected, FVector(300.f, 300.f, 600.f))) continue;

		// Si al proyectar quedó otra vez pegado al objetivo no sirve para alejarse.
		if (FVector::Dist2D(Projected.Location, TargetLocation) < RetreatDistance * 0.9f) continue;
		if (!IsReachable(Pawn, Projected.Location)) continue;

		if (!bPreferLineOfSight || HasLineOfSight(Pawn->GetWorld(), Projected.Location, Target, Pawn))
		{
			Blackboard->SetValueAsVector(OutLocationKey.SelectedKeyName, Projected.Location);
			return EBTNodeResult::Succeeded;
		}
		if (!FAISystem::IsValidLocation(Fallback))
		{
			Fallback = Projected.Location;
		}
	}

	if (FAISystem::IsValidLocation(Fallback))
	{
		Blackboard->SetValueAsVector(OutLocationKey.SelectedKeyName, Fallback);
		return EBTNodeResult::Succeeded;
	}
	return EBTNodeResult::Failed;
}

bool UBTTask_FindKiteLocation::IsReachable(const APawn* Pawn, const FVector& Location) const
{
	const UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(Pawn->GetWorld(), Pawn->GetActorLocation(), Location,
		const_cast<APawn*>(Pawn));
	return Path && Path->IsValid() && !Path->IsPartial();
}

bool UBTTask_FindKiteLocation::HasLineOfSight(const UWorld* World, const FVector& From, const AActor* Target, const APawn* Pawn)
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(KiteLOS), false, Pawn);
	Params.AddIgnoredActor(Target);
	return !World->LineTraceTestByChannel(From + FVector(0.f, 0.f, 80.f), Target->GetActorLocation() + FVector(0.f, 0.f, 50.f),
		ECC_Visibility, Params);
}
