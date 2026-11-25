#include "Features/SolarBoss/AI/BTTask_SolarBossMeleeAttack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Entities/Entity.h"
#include "Features/InventorySystem/Interfaces/Pickable.h"
#include "Features/SolarBoss/Components/SolarBossBrainComponent.h"
#include "Features/SolarBoss/Data/SolarBossConfigDataAsset.h"
#include "GameFramework/Character.h"
#include "GAS/VelmaraGameplayTags.h"

UBTTask_SolarBossMeleeAttack::UBTTask_SolarBossMeleeAttack()
{
	NodeName = TEXT("Solar Boss Melee Attack");
	bNotifyTick = true;
	bNotifyTaskFinished = true;

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, TargetActorKey), AActor::StaticClass());
	TargetActorKey.SelectedKeyName = TEXT("TargetActor");
}

void UBTTask_SolarBossMeleeAttack::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetActorKey.ResolveSelectedKey(*BBAsset);
	}
}

AActor* UBTTask_SolarBossMeleeAttack::GetTarget(const UBehaviorTreeComponent& OwnerComp) const
{
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	return Blackboard ? Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName)) : nullptr;
}

EBTNodeResult::Type UBTTask_SolarBossMeleeAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FBTSolarBossMeleeMemory* Memory = CastInstanceNodeMemory<FBTSolarBossMeleeMemory>(NodeMemory);
	*Memory = FBTSolarBossMeleeMemory();

	ACharacter* Character = OwnerComp.GetAIOwner() ? Cast<ACharacter>(OwnerComp.GetAIOwner()->GetPawn()) : nullptr;
	USolarBossBrainComponent* Brain = Character ? Character->FindComponentByClass<USolarBossBrainComponent>() : nullptr;
	if (!Brain || !Brain->IsReady() || !GetTarget(OwnerComp)) return EBTNodeResult::Failed;

	if (Brain->IsInHitReact()) return EBTNodeResult::InProgress;

	return StartAttack(OwnerComp, *Memory, Character, Brain) ? EBTNodeResult::InProgress : EBTNodeResult::Failed;
}

bool UBTTask_SolarBossMeleeAttack::StartAttack(UBehaviorTreeComponent& OwnerComp, FBTSolarBossMeleeMemory& Memory, ACharacter* Character,
	USolarBossBrainComponent* Brain) const
{
	AActor* Target = GetTarget(OwnerComp);
	const int32 AttackIndex = Brain->SelectMeleeAttack(Target);
	if (AttackIndex == INDEX_NONE) return false;

	const FSolarBossAttackConfig& Attack = Brain->Config->MeleeAttacks[AttackIndex];
	Memory.AttackIndex = AttackIndex;
	Memory.PhaseElapsed = 0.f;
	Brain->CommitMeleeAttack(AttackIndex);
	OwnerComp.GetAIOwner()->StopMovement();

	const FVector ToTarget = (Target->GetActorLocation() - Character->GetActorLocation()).GetSafeNormal2D();
	if (!ToTarget.IsNearlyZero())
	{
		Character->SetActorRotation(ToTarget.Rotation());
	}

	if (Attack.bGrantSuperArmorDuringAttack)
	{
		Memory.bArmorGranted = true;
		Brain->SetTemporarySuperArmor(true);
	}

	if (Attack.AbilityTriggerTag.IsValid())
	{
		Memory.Phase = ESolarBossMeleePhase::Ability;

		FGameplayEventData Payload;
		Payload.Instigator = Character;
		Payload.Target = Target;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Character, Attack.AbilityTriggerTag, Payload);
	}
	else
	{
		Memory.Phase = ESolarBossMeleePhase::Montage;
		Memory.Playback.Start(Character, Attack.Montage, Attack.PlayRate, Attack.FallbackDuration);
	}

	return true;
}

void UBTTask_SolarBossMeleeAttack::SetWeaponActive(FBTSolarBossMeleeMemory& Memory, ACharacter* Character, const bool bActive)
{
	AEntity* Entity = Cast<AEntity>(Character);
	if (!Entity || Memory.bWeaponActive == bActive) return;

	Memory.bWeaponActive = bActive;
	Entity->SetWeaponCollisionEnabled(bActive ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

void UBTTask_SolarBossMeleeAttack::ReleaseAttack(FBTSolarBossMeleeMemory& Memory, ACharacter* Character, USolarBossBrainComponent* Brain)
{
	SetWeaponActive(Memory, Character, false);

	if (Memory.bArmorGranted && Brain)
	{
		Memory.bArmorGranted = false;
		Brain->SetTemporarySuperArmor(false);
	}
}

void UBTTask_SolarBossMeleeAttack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FBTSolarBossMeleeMemory* Memory = CastInstanceNodeMemory<FBTSolarBossMeleeMemory>(NodeMemory);
	ACharacter* Character = OwnerComp.GetAIOwner() ? Cast<ACharacter>(OwnerComp.GetAIOwner()->GetPawn()) : nullptr;
	USolarBossBrainComponent* Brain = Character ? Character->FindComponentByClass<USolarBossBrainComponent>() : nullptr;
	if (!Brain || !Brain->IsReady())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	Memory->PhaseElapsed += DeltaSeconds;

	if (Memory->Phase == ESolarBossMeleePhase::WaitingHitReact)
	{
		if (!Brain->IsInHitReact())
		{
			if (!StartAttack(OwnerComp, *Memory, Character, Brain))
			{
				FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
			}
		}
		else if (Memory->PhaseElapsed > MaxHitReactWait)
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		}
		return;
	}

	if (!Brain->Config->MeleeAttacks.IsValidIndex(Memory->AttackIndex))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	const FSolarBossAttackConfig& Attack = Brain->Config->MeleeAttacks[Memory->AttackIndex];

	if (Memory->Phase == ESolarBossMeleePhase::Recovery)
	{
		if (Memory->PhaseElapsed >= Attack.RecoveryTime)
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		}
		return;
	}

	if (Memory->Phase == ESolarBossMeleePhase::Ability)
	{
		const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Character);
		const bool bAttacking = ASC && ASC->HasMatchingGameplayTag(FVelmaraGameplayTags::Get().State_Attacking);

		if (bAttacking)
		{
			Memory->bAbilityStarted = true;
			if (Memory->PhaseElapsed > AbilityMaxDuration)
			{
				FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
			}
			return;
		}

		const bool bTimedOut = !Memory->bAbilityStarted && Memory->PhaseElapsed > AbilityActivationTimeout;
		if (bTimedOut || (Memory->bAbilityStarted && Brain->IsInHitReact()))
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
			return;
		}

		if (!Memory->bAbilityStarted) return;
	}
	else
	{
		const bool bRunning = Memory->Playback.Advance(Character, DeltaSeconds);
		const float Progress = Memory->Playback.Progress;

		if (Brain->IsInHitReact(Memory->Playback.Montage.Get()))
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
			return;
		}

		const AActor* Target = GetTarget(OwnerComp);
		if (Target && Progress < Attack.TrackingEndFraction)
		{
			const FRotator Desired(0.f, (Target->GetActorLocation() - Character->GetActorLocation()).Rotation().Yaw, 0.f);
			Character->SetActorRotation(FMath::RInterpConstantTo(Character->GetActorRotation(), Desired, DeltaSeconds, Attack.TrackingSpeed));
		}

		if (!Memory->bWindowClosed && Progress >= Attack.HitStartFraction)
		{
			const AEntity* Entity = Cast<AEntity>(Character);
			if (Attack.bUseEquippedWeaponHitbox && Entity && Entity->GetCurrentItem().GetObject())
			{
				SetWeaponActive(*Memory, Character, true);
			}
			else if (!Memory->bHitDone)
			{
				Memory->bHitDone = Brain->TryMeleeSphereHit(Memory->AttackIndex);
			}

			if (Progress >= Attack.HitEndFraction)
			{
				Memory->bWindowClosed = true;
				SetWeaponActive(*Memory, Character, false);
			}
		}

		if (bRunning) return;
	}

	ReleaseAttack(*Memory, Character, Brain);
	Memory->Phase = ESolarBossMeleePhase::Recovery;
	Memory->PhaseElapsed = 0.f;
}

EBTNodeResult::Type UBTTask_SolarBossMeleeAttack::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	return EBTNodeResult::Aborted;
}

void UBTTask_SolarBossMeleeAttack::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	FBTSolarBossMeleeMemory* Memory = CastInstanceNodeMemory<FBTSolarBossMeleeMemory>(NodeMemory);
	ACharacter* Character = OwnerComp.GetAIOwner() ? Cast<ACharacter>(OwnerComp.GetAIOwner()->GetPawn()) : nullptr;
	USolarBossBrainComponent* Brain = Character ? Character->FindComponentByClass<USolarBossBrainComponent>() : nullptr;

	ReleaseAttack(*Memory, Character, Brain);

	if (TaskResult != EBTNodeResult::Succeeded && Brain && Brain->IsReady())
	{
		Memory->Playback.Stop(Character);
	}

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}
