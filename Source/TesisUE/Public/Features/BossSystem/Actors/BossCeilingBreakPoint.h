#pragma once

#include "CoreMinimal.h"
#include "Chaos/ChaosNotifyHandlerInterface.h"
#include "GameFramework/Actor.h"
#include "BossCeilingBreakPoint.generated.h"

class UArrowComponent;
class UGeometryCollectionComponent;

UCLASS()
class TESISUE_API ABossCeilingBreakPoint : public AActor
{
	GENERATED_BODY()

public:
	ABossCeilingBreakPoint();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling")
	int32 StageIndex = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling", meta = (ClampMin = "0"))
	float ShaftRadiusOverride = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling")
	TArray<TObjectPtr<AActor>> CeilingPiecesToHide;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Ceiling", meta = (ClampMin = "0.1"))
	float SettleTime = 4.f;

	UFUNCTION(BlueprintCallable, Category = "Boss|Ceiling")
	void Break();

	UFUNCTION(BlueprintCallable, Category = "Boss|Ceiling")
	void ResetPoint();

	UFUNCTION(BlueprintPure, Category = "Boss|Ceiling")
	bool IsBroken() const { return bBroken; }

	UFUNCTION(BlueprintPure, Category = "Boss|Ceiling")
	bool CanBreakAtStage(int32 Stage) const { return !bBroken && (StageIndex < 0 || StageIndex == Stage); }

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Ceiling")
	void OnCeilingBroken();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UArrowComponent> Arrow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UGeometryCollectionComponent> Rock;

private:
	UFUNCTION()
	void HandleRockCollision(const FChaosPhysicsCollisionInfo& CollisionInfo);

	void Shatter();
	void Settle();

	bool bBroken = false;
	bool bShattered = false;
	FTimerHandle SettleTimer;
};
