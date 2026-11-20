#pragma once

#include "CoreMinimal.h"
#include "AI/CustomAIController.h"
#include "Features/BossSystem/Data/BossTypes.h"
#include "BossAIController.generated.h"

class ABossEnemy;

UCLASS()
class TESISUE_API ABossAIController : public ACustomAIController
{
	GENERATED_BODY()

public:
	ABossAIController(const FObjectInitializer& ObjectInitializer);

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "Boss|AI")
	AActor* GetCombatTarget() const { return CombatTarget.Get(); }

	UFUNCTION(BlueprintPure, Category = "Boss|AI")
	EBossBrainState GetBrainState() const { return BrainState; }

	UFUNCTION(BlueprintCallable, Category = "Boss|AI")
	void SetCombatTarget(AActor* NewTarget) { ForcedTarget = NewTarget; }

	void NotifyBossDamaged(AActor* DamageInstigator);

	void HandleBossDeath();

protected:
	virtual void OnPossess(APawn* InPawn) override;

	virtual void OnUnPossess() override;

private:
	AActor* ResolveTarget() const;
	static bool IsTargetValid(const AActor* Target);

	void RunDecision(AActor* Target);
	bool TryEvade();
	void ChaseTarget(AActor* Target);
	void HandleActionFinished(bool bInterrupted);

	TWeakObjectPtr<ABossEnemy> Boss;
	TWeakObjectPtr<AActor> CombatTarget;
	TWeakObjectPtr<AActor> ForcedTarget;

	EBossBrainState BrainState = EBossBrainState::Inactive;

	double RecoverUntil = 0.0;
	double NextDecisionTime = 0.0;
	double NextEvadeTime = 0.0;

	TArray<double> RecentHitTimes;
	bool bWantsToEvade = false;

	FDelegateHandle ActionFinishedHandle;
};
