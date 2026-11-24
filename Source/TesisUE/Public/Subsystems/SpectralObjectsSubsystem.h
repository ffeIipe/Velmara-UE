#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SpectralObjectsSubsystem.generated.h"

class ISpectral;

UCLASS()
class TESISUE_API USpectralObjectsSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()


public:
	UFUNCTION(BlueprintCallable)
	void AddSpectralObject(AActor* Actor);

	UFUNCTION(BlueprintCallable)
	void ActivateSpectralObjects();

	UFUNCTION(BlueprintCallable)
	void ActivateSpectralObjectsInRange(const FVector& Origin, float Radius, float Duration = 5.f);

	UFUNCTION(BlueprintCallable)
	void DeactivateSpectralObjects();

private:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	void UpdateRevealedObjects();
	bool IsPlayerUsing(const AActor* Actor) const;

	TArray<TScriptInterface<ISpectral>> SpectralObjects;

	TMap<TWeakObjectPtr<AActor>, double> RevealedUntil;
	FTimerHandle UpdateTimer;
};
