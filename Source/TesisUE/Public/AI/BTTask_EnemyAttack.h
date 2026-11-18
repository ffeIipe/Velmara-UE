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

/*
 * Ataca con la habilidad que escuche TriggerEventTag (la del arma equipada o la que dé el EntityData) y espera a que termine.
 * Con bChainCombo encadena golpes del árbol de combos del arma simulando pulsaciones cortas del input de ataque en cada
 * ventana de combo (State.ComboWindow), igual que haría el jugador. Al terminar (o abortarse) devuelve el turno del director.
 */
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

	// Evento que dispara la habilidad de ataque (GA_ComboBase_Generic, GA_ShamanStaffMagic...).
	UPROPERTY(EditAnywhere, Category = "Attack")
	FGameplayTag TriggerEventTag;

	// Tag que tiene el dueño mientras la habilidad está atacando.
	UPROPERTY(EditAnywhere, Category = "Attack")
	FGameplayTag AttackingTag;

	// Si la habilidad no arranca en este tiempo, la tarea falla (bloqueada por hit react, sin arma, etc.).
	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = "0.05"))
	float ActivationTimeout = 0.4f;

	// Corte de seguridad para que el árbol nunca quede colgado esperando una habilidad.
	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = "0.5"))
	float MaxDuration = 8.f;

	// Gira de golpe hacia el objetivo antes de atacar.
	UPROPERTY(EditAnywhere, Category = "Attack")
	bool bFaceTarget = true;

	UPROPERTY(EditAnywhere, Category = "Combo")
	bool bChainCombo = true;

	// Golpes totales por ataque (el primero incluido); se elige uno al azar entre Min y Max.
	UPROPERTY(EditAnywhere, Category = "Combo", meta = (EditCondition = "bChainCombo", ClampMin = "1"))
	int32 MinComboHits = 1;

	UPROPERTY(EditAnywhere, Category = "Combo", meta = (EditCondition = "bChainCombo", ClampMin = "1"))
	int32 MaxComboHits = 3;

	// No encadena el siguiente golpe si el objetivo se alejó más que esto.
	UPROPERTY(EditAnywhere, Category = "Combo", meta = (EditCondition = "bChainCombo"))
	float MaxChainDistance = 400.f;

	UPROPERTY(EditAnywhere, Category = "Combo", meta = (EditCondition = "bChainCombo"))
	FGameplayTag ComboWindowTag;

	// InputID (EVelmaraAbilityInputID) con el que se otorgó la habilidad de ataque.
	UPROPERTY(EditAnywhere, Category = "Combo", meta = (EditCondition = "bChainCombo"))
	int32 AttackInputID = 3;

	UPROPERTY(EditAnywhere, Category = "Director")
	bool bReturnAttackToken = true;

	// Tiempo que el enemigo espera antes de volver a pedir turno, para que ataquen otros.
	UPROPERTY(EditAnywhere, Category = "Director", meta = (EditCondition = "bReturnAttackToken"))
	FVector2D TokenCooldown = FVector2D(0.6f, 1.4f);

private:
	static UAbilitySystemComponent* GetPawnASC(const UBehaviorTreeComponent& OwnerComp);
	AActor* GetTarget(const UBehaviorTreeComponent& OwnerComp) const;
};
