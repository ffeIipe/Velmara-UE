#include "Features/BossSystem/Core/BossAIController.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Entities/Entity.h"
#include "Features/BossSystem/Components/BossCeilingBreakComponent.h"
#include "Features/BossSystem/Components/BossCombatComponent.h"
#include "Features/BossSystem/Components/BossSummonComponent.h"
#include "Features/BossSystem/Core/BossEnemy.h"
#include "GAS/VelmaraGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"

ABossAIController::ABossAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void ABossAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	Boss = Cast<ABossEnemy>(InPawn);
	BrainState = EBossBrainState::Inactive;
	StaggerUntil = 0.0;
	bStaggerTagApplied = false;

	if (const ABossEnemy* BossPawn = Boss.Get())
	{
		if (UBossCombatComponent* Combat = BossPawn->GetCombatComponent())
		{
			ActionFinishedHandle = Combat->OnActionFinished.AddUObject(this, &ABossAIController::HandleActionFinished);
		}
	}

	SetActorTickEnabled(Boss.IsValid());
}

void ABossAIController::OnUnPossess()
{
	ClearStaggerState();

	if (const ABossEnemy* BossPawn = Boss.Get())
	{
		if (UBossCombatComponent* Combat = BossPawn->GetCombatComponent())
		{
			Combat->OnActionFinished.Remove(ActionFinishedHandle);
		}
	}

	ActionFinishedHandle.Reset();
	Boss.Reset();

	Super::OnUnPossess();
}

void ABossAIController::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ABossEnemy* BossPawn = Boss.Get();
	if (!BossPawn || BrainState == EBossBrainState::Dead) return;

	if (BossPawn->IsDefeated() || !BossPawn->IsAlive())
	{
		HandleBossDeath();
		return;
	}

	const UBossCeilingBreakComponent* Ceiling = BossPawn->GetCeilingBreakComponent();
	const bool bStagePending = Ceiling && Ceiling->HasPendingStage();
	const double Now = GetWorld()->GetTimeSeconds();

	if (BrainState == EBossBrainState::Staggered)
	{
		if (bStagePending || Now >= StaggerUntil)
		{
			ClearStaggerState();
			RecoverUntil = 0.0;
			NextDecisionTime = 0.0;
			BrainState = EBossBrainState::Recovering;
		}
		else
		{
			return;
		}
	}

	UBossCombatComponent* Combat = BossPawn->GetCombatComponent();
	AActor* Target = ResolveTarget();
	CombatTarget = Target;

	if (!Target || !Combat)
	{
		if (BrainState == EBossBrainState::Chasing)
		{
			StopMovement();
		}
		BrainState = EBossBrainState::Inactive;
		return;
	}

	if (!BossPawn->IsEncounterStarted())
	{
		if (!BossPawn->bAutoStartEncounter || FVector::Dist(BossPawn->GetActorLocation(), Target->GetActorLocation()) > BossPawn->AggroRadius) return;

		BossPawn->StartEncounter();
	}

	if (bStagePending && Ceiling->bInterruptAttacks && Combat->IsAttacking())
	{
		Combat->CancelCurrentAction();
	}

	if (Combat->IsBusy()) return;

	if (Now < RecoverUntil && !bStagePending)
	{
		if (bWantsToEvade && TryEvade()) return;

		BrainState = EBossBrainState::Recovering;
		Combat->RotateOwnerTowards(Target->GetActorLocation(), DeltaSeconds, BossPawn->FacingSpeed);
		return;
	}

	if (BrainState == EBossBrainState::Chasing && GetMoveStatus() != EPathFollowingStatus::Moving)
	{
		Combat->RotateOwnerTowards(Target->GetActorLocation(), DeltaSeconds, BossPawn->FacingSpeed);
	}

	if (Now < NextDecisionTime) return;
	NextDecisionTime = Now + BossPawn->DecisionInterval;

	RunDecision(Target);
}

void ABossAIController::RunDecision(AActor* Target)
{
	ABossEnemy* BossPawn = Boss.Get();
	UBossCombatComponent* Combat = BossPawn->GetCombatComponent();

	if (UBossCeilingBreakComponent* Ceiling = BossPawn->GetCeilingBreakComponent(); Ceiling && Ceiling->HasPendingStage())
	{
		StopMovement();
		if (Ceiling->ExecutePendingStage(Target))
		{
			BrainState = EBossBrainState::BreakingCeiling;
			return;
		}
	}

	UBossSummonComponent* Summon = BossPawn->GetSummonComponent();
	if (Summon && Summon->NeedsEmergencySummon())
	{
		StopMovement();
		if (Summon->StartEmergencySummon(Target))
		{
			BrainState = EBossBrainState::Summoning;
			return;
		}
	}

	if (bWantsToEvade && TryEvade()) return;

	if (Summon && Summon->CanSummon())
	{
		StopMovement();
		if (Summon->StartSummon(Target))
		{
			BrainState = EBossBrainState::Summoning;
			return;
		}
	}

	const int32 AttackIndex = Combat->SelectAttack(Target, Combat->GetStage());
	if (AttackIndex != INDEX_NONE)
	{
		StopMovement();
		if (Combat->StartAttack(AttackIndex, Target))
		{
			BrainState = EBossBrainState::Attacking;
			return;
		}
	}

	ChaseTarget(Target);
}

void ABossAIController::ChaseTarget(AActor* Target)
{
	const ABossEnemy* BossPawn = Boss.Get();
	BrainState = EBossBrainState::Chasing;

	if (FVector::Dist2D(BossPawn->GetActorLocation(), Target->GetActorLocation()) <= BossPawn->ChaseAcceptanceRadius)
	{
		if (GetMoveStatus() == EPathFollowingStatus::Moving)
		{
			StopMovement();
		}
		return;
	}

	if (GetMoveStatus() == EPathFollowingStatus::Moving) return;

	MoveToActor(Target, BossPawn->ChaseAcceptanceRadius, true, true, false, nullptr, true);
}

bool ABossAIController::TryEvade()
{
	bWantsToEvade = false;

	ABossEnemy* BossPawn = Boss.Get();
	if (!BossPawn) return false;

	TArray<UAnimMontage*> ValidMontages;
	for (UAnimMontage* Montage : BossPawn->EvadeMontages)
	{
		if (Montage) ValidMontages.Add(Montage);
	}
	if (ValidMontages.Num() == 0) return false;

	UAnimMontage* Montage = ValidMontages[FMath::RandRange(0, ValidMontages.Num() - 1)];

	StopMovement();
	if (!BossPawn->GetCombatComponent()->PlayActionMontage(Montage, 1.f, -1.f, FSimpleDelegate(), FBossActionFinished(), nullptr, 0.15f))
	{
		return false;
	}

	BrainState = EBossBrainState::Evading;
	NextEvadeTime = GetWorld()->GetTimeSeconds() + BossPawn->EvadeCooldown;
	RecentHitTimes.Reset();
	return true;
}

void ABossAIController::TriggerStagger(const float Duration, UAnimMontage* OptionalMontage)
{
	ABossEnemy* BossPawn = Boss.Get();
	if (!BossPawn || BrainState == EBossBrainState::Dead || BrainState == EBossBrainState::BreakingCeiling || BossPawn->IsDefeated() || !BossPawn->IsAlive() || Duration <= 0.f) return;

	if (UBossCombatComponent* Combat = BossPawn->GetCombatComponent())
	{
		Combat->CancelCurrentAction();
	}

	StopMovement();
	bWantsToEvade = false;

	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	StaggerUntil = Now + Duration;
	BrainState = EBossBrainState::Staggered;

	if (!bStaggerTagApplied)
	{
		if (UAbilitySystemComponent* ASC = BossPawn->GetAbilitySystemComponent())
		{
			const FGameplayTag StunnedTag = FVelmaraGameplayTags::Get().State_Stunned;
			if (StunnedTag.IsValid())
			{
				ASC->AddLooseGameplayTag(StunnedTag);
				bStaggerTagApplied = true;
			}
		}
	}

	if (OptionalMontage)
	{
		if (UAnimInstance* AnimInstance = BossPawn->GetMesh() ? BossPawn->GetMesh()->GetAnimInstance() : nullptr)
		{
			AnimInstance->Montage_Play(OptionalMontage);
		}
	}
}

void ABossAIController::ClearStaggerState()
{
	if (bStaggerTagApplied)
	{
		if (ABossEnemy* BossPawn = Boss.Get())
		{
			if (UAbilitySystemComponent* ASC = BossPawn->GetAbilitySystemComponent())
			{
				const FGameplayTag StunnedTag = FVelmaraGameplayTags::Get().State_Stunned;
				if (StunnedTag.IsValid())
				{
					ASC->RemoveLooseGameplayTag(StunnedTag);
				}
			}
		}
	}

	bStaggerTagApplied = false;
	StaggerUntil = 0.0;
}

void ABossAIController::NotifyBossDamaged(AActor* DamageInstigator)
{
	const ABossEnemy* BossPawn = Boss.Get();
	if (!BossPawn || BrainState == EBossBrainState::Dead || BrainState == EBossBrainState::Staggered) return;

	const double Now = GetWorld()->GetTimeSeconds();
	const float HitWindow = BossPawn->EvadeHitWindow;

	RecentHitTimes.Add(Now);
	RecentHitTimes.RemoveAll([Now, HitWindow](const double HitTime) { return Now - HitTime > HitWindow; });

	if (RecentHitTimes.Num() < BossPawn->EvadeHitThreshold || Now < NextEvadeTime) return;

	if (FMath::FRand() < BossPawn->EvadeChance)
	{
		bWantsToEvade = true;
	}
	RecentHitTimes.Reset();
}

void ABossAIController::HandleActionFinished(const bool bInterrupted)
{
	if (BrainState == EBossBrainState::Dead || BrainState == EBossBrainState::Staggered) return;

	const ABossEnemy* BossPawn = Boss.Get();
	const UBossCombatComponent* Combat = BossPawn ? BossPawn->GetCombatComponent() : nullptr;

	RecoverUntil = GetWorld()->GetTimeSeconds() + (Combat ? Combat->GetLastRecoveryTime() : 0.f);
	NextDecisionTime = 0.0;
	BrainState = EBossBrainState::Recovering;
}

void ABossAIController::HandleBossDeath()
{
	if (BrainState == EBossBrainState::Dead) return;

	ClearStaggerState();
	BrainState = EBossBrainState::Dead;
	bWantsToEvade = false;

	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);
	SetActorTickEnabled(false);
}

AActor* ABossAIController::ResolveTarget() const
{
	if (AActor* Forced = ForcedTarget.Get(); IsTargetValid(Forced))
	{
		return Forced;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	return IsTargetValid(PlayerPawn) ? PlayerPawn : nullptr;
}

bool ABossAIController::IsTargetValid(const AActor* Target)
{
	if (!IsValid(Target) || Target->IsHidden()) return false;

	const AEntity* Entity = Cast<AEntity>(Target);
	return !Entity || Entity->IsAlive();
}
