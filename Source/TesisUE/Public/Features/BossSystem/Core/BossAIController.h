#pragma once

#include "CoreMinimal.h"
#include "AI/CustomAIController.h"
#include "Features/BossSystem/Data/BossTypes.h"
#include "BossAIController.generated.h"

class ABossEnemy;

/*
 * Cerebro del boss. Cada decisión prioriza: romper el techo (si cruzó un umbral) > esquivar > invocar > atacar según
 * la distancia > perseguir. Toda la configuración vive en el ABossEnemy y sus componentes.
 */
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

	// Fuerza un objetivo. Sin objetivo forzado persigue al pawn del jugador 0 (Velmara o lo que esté poseyendo).
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
