#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "BloodMarkSubsystem.generated.h"

class UMaterialInterface;
class UMeshComponent;

UCLASS()
class TESISUE_API UBloodMarkSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void MarkEnemiesInRange(const FVector& Origin, float Radius, float Duration, UMaterialInterface* OverlayMaterial);

private:
	struct FMark
	{
		double ExpireTime = 0.0;
		TWeakObjectPtr<UMeshComponent> Mesh;
		TWeakObjectPtr<UMaterialInterface> PreviousOverlay;
	};

	void UpdateMarks();
	void ClearMark(AActor* Actor, const FMark& Mark) const;
	bool IsEnemyOfPlayer(const AActor* Actor) const;

	TMap<TWeakObjectPtr<AActor>, FMark> Marks;
	FTimerHandle UpdateTimer;
};
