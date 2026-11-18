#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FindKiteLocation.generated.h"

/*
 * Para enemigos a distancia: busca en el NavMesh un punto a PreferredDistance del objetivo y lo escribe en OutLocationKey.
 * Si el objetivo está más cerca que RetreatDistance se aleja en línea (abriéndose a los costados si hay pared); si no,
 * se desplaza de costado alrededor del objetivo para no quedarse quieto. Prefiere puntos con línea de visión al objetivo.
 */
UCLASS()
class TESISUE_API UBTTask_FindKiteLocation : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_FindKiteLocation();

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector OutLocationKey;

	UPROPERTY(EditAnywhere, Category = "Kite", meta = (ClampMin = "0"))
	float PreferredDistance = 1150.f;

	UPROPERTY(EditAnywhere, Category = "Kite", meta = (ClampMin = "0"))
	float RetreatDistance = 650.f;

	// Grados que se abre al costado cuando no tiene que huir.
	UPROPERTY(EditAnywhere, Category = "Kite")
	FVector2D StrafeAngle = FVector2D(25.f, 60.f);

	UPROPERTY(EditAnywhere, Category = "Kite")
	bool bPreferLineOfSight = true;

private:
	bool IsReachable(const APawn* Pawn, const FVector& Location) const;
	static bool HasLineOfSight(const UWorld* World, const FVector& From, const AActor* Target, const APawn* Pawn);
};
