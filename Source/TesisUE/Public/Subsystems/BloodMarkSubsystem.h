#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "BloodMarkSubsystem.generated.h"

class UMaterialInterface;
class UMeshComponent;
class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBloodMarkActorEvent, AActor*, MarkedActor);

UCLASS()
class TESISUE_API UBloodMarkSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void MarkEnemiesInRange(const FVector& Origin, float Radius, float Duration, UMaterialInterface* OverlayMaterial);

	UFUNCTION(BlueprintPure, Category = "BloodMark")
	bool IsActorBloodMarked(const AActor* Actor) const;

	UFUNCTION(BlueprintPure, Category = "BloodMark")
	float GetRemainingMarkTime(const AActor* Actor) const;

	UFUNCTION(BlueprintCallable, Category = "BloodMark")
	void ClearMarkForActor(AActor* Actor);

	UFUNCTION(BlueprintPure, Category = "BloodMark")
	int32 GetMarkedEnemyCount() const;

	void GetMarkedActors(TArray<AActor*>& OutActors) const;

	UPROPERTY(BlueprintAssignable, Category = "BloodMark")
	FOnBloodMarkActorEvent OnActorBloodMarked;

	UPROPERTY(BlueprintAssignable, Category = "BloodMark")
	FOnBloodMarkActorEvent OnActorBloodMarkCleared;

	virtual void Deinitialize() override;

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
