#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TotemSubsystem.generated.h"

class UTotemChargeComponent;

UCLASS()
class TESISUE_API UTotemSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void RegisterTotem(UTotemChargeComponent* Totem);

	void UnregisterTotem(UTotemChargeComponent* Totem);

	UFUNCTION(BlueprintCallable, Category = "Totem")
	void NotifyEntityDied(AActor* DeadActor);

private:
	TArray<TWeakObjectPtr<UTotemChargeComponent>> Totems;
};
