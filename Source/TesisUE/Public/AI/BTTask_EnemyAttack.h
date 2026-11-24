#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_EnemyAttack.generated.h"

class UAbilitySystemComponent;

struct FBTEnemyAttackMemory
{
	float Elapsed = 0.f;
	int32 HitsLeft = 0;
	bool bStarted = false;
	bool bWindowWasOpen = false;
	bool bInputHeld = false;
};

UCLASS()
class TESISUE_API UBTTask_EnemyAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_EnemyAttack();

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FBTEnemyAttackMemory); }
	virtual FString GetStaticDescription() const override;

protected:
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(EditAnywhere, Category = "Attack")
	FGameplayTag TriggerEventTag;

	UPROPERTY(EditAnywhere, Category = "Attack")
	FGameplayTag AttackingTag;

	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = "0.05"))
	float ActivationTimeout = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = "0.5"))
	float MaxDuration = 8.f;

	UPROPERTY(EditAnywhere, Category = "Attack")
	bool bFaceTarget = true;

	UPROPERTY(EditAnywhere, Category = "Combo")
	bool bChainCombo = true;

	UPROPERTY(EditAnywhere, Category = "Combo", meta = (EditCondition = "bChainCombo", ClampMin = "1"))
	int32 MinComboHits = 1;

	UPROPERTY(EditAnywhere, Category = "Combo", meta = (EditCondition = "bChainCombo", ClampMin = "1"))
	int32 MaxComboHits = 3;

	UPROPERTY(EditAnywhere, Category = "Combo", meta = (EditCondition = "bChainCombo"))
	float MaxChainDistance = 400.f;

	UPROPERTY(EditAnywhere, Category = "Combo", meta = (EditCondition = "bChainCombo"))
	FGameplayTag ComboWindowTag;

	UPROPERTY(EditAnywhere, Category = "Combo", meta = (EditCondition = "bChainCombo"))
	int32 AttackInputID = 3;

	UPROPERTY(EditAnywhere, Category = "Director")
	bool bReturnAttackToken = true;

	UPROPERTY(EditAnywhere, Category = "Director", meta = (EditCondition = "bReturnAttackToken"))
	FVector2D TokenCooldown = FVector2D(0.6f, 1.4f);

private:
	static UAbilitySystemComponent* GetPawnASC(const UBehaviorTreeComponent& OwnerComp);
	AActor* GetTarget(const UBehaviorTreeComponent& OwnerComp) const;
};
