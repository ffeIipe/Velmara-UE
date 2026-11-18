#include "AI/BTTask_EnemyAttack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "AI/Core/CombatDirectorSubsystem.h"
#include "BehaviorTree/BlackboardComponent.h"

extern TAutoConsoleVariable<bool> CVarDebugEnemyCombat;

UBTTask_EnemyAttack::UBTTask_EnemyAttack()
{
	NodeName = TEXT("Enemy Attack");
	bNotifyTick = true;
	bNotifyTaskFinished = true;

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, TargetActorKey), AActor::StaticClass());
	TargetActorKey.SelectedKeyName = TEXT("TargetActor");

	TriggerEventTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Attack"), false);
	AttackingTag = FGameplayTag::RequestGameplayTag(TEXT("State.Attacking"), false);
	ComboWindowTag = FGameplayTag::RequestGameplayTag(TEXT("State.ComboWindow"), false);
}

void UBTTask_EnemyAttack::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetActorKey.ResolveSelectedKey(*BBAsset);
	}
}

FString UBTTask_EnemyAttack::GetStaticDescription() const
{
	return bChainCombo
		? FString::Printf(TEXT("%s\nCombo %d-%d hits"), *TriggerEventTag.ToString(), MinComboHits, MaxComboHits)
		: TriggerEventTag.ToString();
}

UAbilitySystemComponent* UBTTask_EnemyAttack::GetPawnASC(const UBehaviorTreeComponent& OwnerComp)
{
	const AAIController* Controller = OwnerComp.GetAIOwner();
	return Controller ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Controller->GetPawn()) : nullptr;
}

AActor* UBTTask_EnemyAttack::GetTarget(const UBehaviorTreeComponent& OwnerComp) const
{
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	return Blackboard ? Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName)) : nullptr;
}

EBTNodeResult::Type UBTTask_EnemyAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FBTEnemyAttackMemory* Memory = CastInstanceNodeMemory<FBTEnemyAttackMemory>(NodeMemory);
	*Memory = FBTEnemyAttackMemory();

	APawn* Pawn = OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr;
	if (!Pawn || !GetPawnASC(OwnerComp) || !TriggerEventTag.IsValid()) return EBTNodeResult::Failed;

	AActor* Target = GetTarget(OwnerComp);
	if (bFaceTarget && Target)
	{
		const FVector ToTarget = (Target->GetActorLocation() - Pawn->GetActorLocation()).GetSafeNormal2D();
		if (!ToTarget.IsNearlyZero())
		{
			Pawn->SetActorRotation(ToTarget.Rotation());
		}
	}

	Memory->HitsLeft = bChainCombo ? FMath::RandRange(MinComboHits, FMath::Max(MinComboHits, MaxComboHits)) - 1 : 0;

	FGameplayEventData Payload;
	Payload.Instigator = Pawn;
	Payload.Target = Target;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Pawn, TriggerEventTag, Payload);

	return EBTNodeResult::InProgress;
}

void UBTTask_EnemyAttack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FBTEnemyAttackMemory* Memory = CastInstanceNodeMemory<FBTEnemyAttackMemory>(NodeMemory);
	UAbilitySystemComponent* ASC = GetPawnASC(OwnerComp);
	if (!ASC)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	Memory->Elapsed += DeltaSeconds;

	// Un frame de input apretado alcanza para que el combo lo lea como "tap".
	if (Memory->bInputHeld)
	{
		ASC->AbilityLocalInputReleased(AttackInputID);
		Memory->bInputHeld = false;
	}

	const bool bAttacking = ASC->HasMatchingGameplayTag(AttackingTag);

	if (!Memory->bStarted)
	{
		if (bAttacking)
		{
			Memory->bStarted = true;
		}
		else if (Memory->Elapsed > ActivationTimeout)
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		}
		return;
	}

	if (!bAttacking || Memory->Elapsed > MaxDuration)
	{
		FinishLatentTask(OwnerComp, bAttacking ? EBTNodeResult::Failed : EBTNodeResult::Succeeded);
		return;
	}

	if (Memory->HitsLeft > 0)
	{
		const bool bWindowOpen = ASC->HasMatchingGameplayTag(ComboWindowTag);
		if (bWindowOpen && !Memory->bWindowWasOpen)
		{
			const AActor* Target = GetTarget(OwnerComp);
			const APawn* Pawn = OwnerComp.GetAIOwner()->GetPawn();
			if (Target && Pawn && FVector::Dist2D(Target->GetActorLocation(), Pawn->GetActorLocation()) <= MaxChainDistance)
			{
				ASC->AbilityLocalInputPressed(AttackInputID);
				Memory->bInputHeld = true;
				Memory->HitsLeft--;
			}
			else
			{
				Memory->HitsLeft = 0;
			}
		}
		Memory->bWindowWasOpen = bWindowOpen;
	}
}

EBTNodeResult::Type UBTTask_EnemyAttack::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// La habilidad termina sola (montage); cortar el árbol no la cancela, así el golpe no queda a medias.
	return EBTNodeResult::Aborted;
}

void UBTTask_EnemyAttack::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	FBTEnemyAttackMemory* Memory = CastInstanceNodeMemory<FBTEnemyAttackMemory>(NodeMemory);
	UE_CLOG(CVarDebugEnemyCombat.GetValueOnGameThread(), LogTemp, Log, TEXT("EnemyAttack %s: finished %s after %.2fs (started %d)"),
		*GetNameSafe(OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr),
		TaskResult == EBTNodeResult::Succeeded ? TEXT("OK") : TaskResult == EBTNodeResult::Aborted ? TEXT("ABORTED") : TEXT("FAILED"), Memory->Elapsed, Memory->bStarted);
	UAbilitySystemComponent* ASC = GetPawnASC(OwnerComp);
	if (Memory->bInputHeld && ASC)
	{
		ASC->AbilityLocalInputReleased(AttackInputID);
		Memory->bInputHeld = false;
	}

	if (bReturnAttackToken)
	{
		APawn* Pawn = OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr;
		UCombatDirectorSubsystem* Director = Pawn ? Pawn->GetWorld()->GetSubsystem<UCombatDirectorSubsystem>() : nullptr;
		if (Director)
		{
			Director->ReturnAllAttackTokens(Pawn);
			Director->ReturnAttackTokenWithCooldown(Pawn, GetTarget(OwnerComp), FMath::FRandRange(TokenCooldown.X, TokenCooldown.Y));
		}
	}

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}
