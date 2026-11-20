#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_EnemyCombat.generated.h"

UCLASS()
class TESISUE_API UBTService_EnemyCombat : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_EnemyCombat();

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual FString GetStaticDescription() const override;

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector CombatStateKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	uint8 AttackStateValue = 0;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	uint8 RepositionStateValue = 1;

	UPROPERTY(EditAnywhere, Category = "Combat")
	bool bRanged = false;

	UPROPERTY(EditAnywhere, Category = "Combat", meta = (ClampMin = "0"))
	float LoseInterestRange = 3500.f;

	UPROPERTY(EditAnywhere, Category = "Combat|Melee", meta = (EditCondition = "!bRanged", ClampMin = "0"))
	float EngageRange = 900.f;

	UPROPERTY(EditAnywhere, Category = "Combat|Melee", meta = (EditCondition = "!bRanged", ClampMin = "0"))
	float AbortRange = 1600.f;

	UPROPERTY(EditAnywhere, Category = "Combat|Ranged", meta = (EditCondition = "bRanged", ClampMin = "0"))
	float MinCastRange = 600.f;

	UPROPERTY(EditAnywhere, Category = "Combat|Ranged", meta = (EditCondition = "bRanged", ClampMin = "0"))
	float MaxCastRange = 1900.f;

	UPROPERTY(EditAnywhere, Category = "Combat|Ranged", meta = (EditCondition = "bRanged"))
	bool bRequireLineOfSight = true;

private:
	static bool IsTargetAlive(const AActor* Target);
	bool HasLineOfSight(const APawn* Pawn, const AActor* Target) const;
	void ReleaseToken(UBehaviorTreeComponent& OwnerComp) const;
};
