#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "Features/SolarBoss/AI/SolarBossActionPlayback.h"
#include "BTTask_SolarBossMeleeAttack.generated.h"

class ACharacter;
class USolarBossBrainComponent;

enum class ESolarBossMeleePhase : uint8
{
	WaitingHitReact,
	Ability,
	Montage,
	Recovery
};

struct FBTSolarBossMeleeMemory
{
	FSolarBossActionPlayback Playback;
	ESolarBossMeleePhase Phase = ESolarBossMeleePhase::WaitingHitReact;
	float PhaseElapsed = 0.f;
	int32 AttackIndex = INDEX_NONE;
	bool bAbilityStarted = false;
	bool bHitDone = false;
	bool bWindowClosed = false;
	bool bWeaponActive = false;
	bool bArmorGranted = false;
};

UCLASS()
class TESISUE_API UBTTask_SolarBossMeleeAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_SolarBossMeleeAttack();

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FBTSolarBossMeleeMemory); }

protected:
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = "0"))
	float MaxHitReactWait = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = "0.05"))
	float AbilityActivationTimeout = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = "0.5"))
	float AbilityMaxDuration = 6.f;

private:
	AActor* GetTarget(const UBehaviorTreeComponent& OwnerComp) const;
	bool StartAttack(UBehaviorTreeComponent& OwnerComp, FBTSolarBossMeleeMemory& Memory, ACharacter* Character, USolarBossBrainComponent* Brain) const;
	static void SetWeaponActive(FBTSolarBossMeleeMemory& Memory, ACharacter* Character, bool bActive);
	static void ReleaseAttack(FBTSolarBossMeleeMemory& Memory, ACharacter* Character, USolarBossBrainComponent* Brain);
};
