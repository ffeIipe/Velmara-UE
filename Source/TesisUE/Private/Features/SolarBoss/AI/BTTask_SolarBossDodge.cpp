#include "Features/SolarBoss/AI/BTTask_SolarBossDodge.h"

#include "AIController.h"
#include "NavigationSystem.h"
#include "Animation/AnimMontage.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Features/SolarBoss/Components/SolarBossBrainComponent.h"
#include "Features/SolarBoss/Data/SolarBossConfigDataAsset.h"
#include "GameFramework/Character.h"

UBTTask_SolarBossDodge::UBTTask_SolarBossDodge()
{
	NodeName = TEXT("Solar Boss Dodge");
	bNotifyTick = true;
	bNotifyTaskFinished = true;

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, TargetActorKey), AActor::StaticClass());
	TargetActorKey.SelectedKeyName = TEXT("TargetActor");
}

void UBTTask_SolarBossDodge::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetActorKey.ResolveSelectedKey(*BBAsset);
	}
}

EBTNodeResult::Type UBTTask_SolarBossDodge::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FBTSolarBossDodgeMemory* Memory = CastInstanceNodeMemory<FBTSolarBossDodgeMemory>(NodeMemory);
	*Memory = FBTSolarBossDodgeMemory();

	AAIController* Controller = OwnerComp.GetAIOwner();
	ACharacter* Character = Controller ? Cast<ACharacter>(Controller->GetPawn()) : nullptr;
	USolarBossBrainComponent* Brain = Character ? Character->FindComponentByClass<USolarBossBrainComponent>() : nullptr;
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Brain || !Brain->IsReady() || !Blackboard) return EBTNodeResult::Failed;

	const USolarBossConfigDataAsset* Config = Brain->Config;
	const AActor* Target = Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName));

	if (bSkipIfTargetOutsidePanicRadius
		&& (!Target || FVector::Dist2D(Target->GetActorLocation(), Character->GetActorLocation()) > Config->EvadePanicDodgeRadius))
	{
		return EBTNodeResult::Succeeded;
	}

	Brain->NotifyDodge(DodgeMode);
	Controller->StopMovement();

	if (Target)
	{
		const FVector ToTarget = (Target->GetActorLocation() - Character->GetActorLocation()).GetSafeNormal2D();
		if (!ToTarget.IsNearlyZero())
		{
			Character->SetActorRotation(ToTarget.Rotation());
		}
	}

	const FVector Offset = (FindDestination(Character, Target, Config->DodgeDistance) - Character->GetNavAgentLocation()) * FVector(1.f, 1.f, 0.f);
	const FVector LocalDirection = Character->GetActorTransform().InverseTransformVectorNoScale(Offset);

	UAnimMontage* Montage = Config->DodgeBackMontage;
	if (LocalDirection.X > 0.f || FMath::Abs(LocalDirection.Y) > FMath::Abs(LocalDirection.X))
	{
		Montage = LocalDirection.Y < 0.f ? Config->DodgeLeftMontage : Config->DodgeRightMontage;
	}

	Memory->Playback.Start(Character, Montage, Config->DodgePlayRate, Config->DodgeDurationFallback);

	const UAnimMontage* Playing = Memory->Playback.Montage.Get();
	Memory->Duration = Playing
		? Playing->GetPlayLength() / FMath::Max(Config->DodgePlayRate * Playing->RateScale, 0.1f)
		: Config->DodgeDurationFallback;
	Memory->Offset = Playing && Playing->HasRootMotion() ? FVector::ZeroVector : Offset;

	return EBTNodeResult::InProgress;
}

FVector UBTTask_SolarBossDodge::FindDestination(APawn* Pawn, const AActor* Target, const float Distance) const
{
	const FVector Start = Pawn->GetNavAgentLocation();

	FVector Away = Target ? (Pawn->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D() : FVector::ZeroVector;
	if (Away.IsNearlyZero())
	{
		Away = -Pawn->GetActorForwardVector().GetSafeNormal2D();
	}

	const FVector Right = (-Away).RotateAngleAxis(90.f, FVector::UpVector);

	TArray<FVector, TInlineAllocator<5>> Directions;
	switch (Direction)
	{
	case ESolarBossDodgeDirection::StrafeLeft:
		Directions.Add(-Right);
		break;
	case ESolarBossDodgeDirection::StrafeRight:
		Directions.Add(Right);
		break;
	case ESolarBossDodgeDirection::BestNavigable:
		Directions.Add(Away);
		Directions.Add(Away.RotateAngleAxis(45.f, FVector::UpVector));
		Directions.Add(Away.RotateAngleAxis(-45.f, FVector::UpVector));
		Directions.Add(-Right);
		Directions.Add(Right);
		break;
	default:
		Directions.Add(Away);
		break;
	}

	UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Pawn->GetWorld());
	if (!NavSystem) return Start + Directions[0] * Distance;

	const FVector Reference = Target ? Target->GetActorLocation() : Start;
	FVector Best = Start;
	float BestDistance = -1.f;

	for (const FVector& Candidate : Directions)
	{
		const FVector End = Start + Candidate * Distance;
		FVector HitLocation;
		const bool bBlocked = UNavigationSystemV1::NavigationRaycast(Pawn, Start, End, HitLocation, nullptr, Pawn->GetController());

		FNavLocation Projected;
		if (!NavSystem->ProjectPointToNavigation(bBlocked ? HitLocation : End, Projected, FVector(100.f, 100.f, 250.f))) continue;

		const float FromTarget = FVector::Dist2D(Projected.Location, Reference);
		if (FromTarget > BestDistance)
		{
			Best = Projected.Location;
			BestDistance = FromTarget;
		}
	}

	return Best;
}

void UBTTask_SolarBossDodge::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FBTSolarBossDodgeMemory* Memory = CastInstanceNodeMemory<FBTSolarBossDodgeMemory>(NodeMemory);
	ACharacter* Character = OwnerComp.GetAIOwner() ? Cast<ACharacter>(OwnerComp.GetAIOwner()->GetPawn()) : nullptr;
	if (!Character)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	Memory->Elapsed += DeltaSeconds;

	const float Alpha = Memory->Duration > 0.f ? FMath::Min(Memory->Elapsed / Memory->Duration, 1.f) : 1.f;
	const float Eased = FMath::InterpEaseOut(0.f, 1.f, Alpha, 2.f);
	Character->SetActorLocation(Character->GetActorLocation() + Memory->Offset * (Eased - Memory->MovedAlpha), true);
	Memory->MovedAlpha = Eased;

	if (Alpha >= 1.f)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

EBTNodeResult::Type UBTTask_SolarBossDodge::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	return EBTNodeResult::Aborted;
}

void UBTTask_SolarBossDodge::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	const FBTSolarBossDodgeMemory* Memory = CastInstanceNodeMemory<FBTSolarBossDodgeMemory>(NodeMemory);
	if (TaskResult == EBTNodeResult::Aborted && OwnerComp.GetAIOwner())
	{
		Memory->Playback.Stop(Cast<ACharacter>(OwnerComp.GetAIOwner()->GetPawn()));
	}

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}
